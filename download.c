#include "download.h"

int main(int argc, char** argv)
{
	if (argc!=2)
	{
		printf("Wrong number of arguments\n");
		return -1;
	}
	
	connectionInfo cInfo;
	
	if( parseUrl(argv[1], &cInfo) != 0 )
		printf("Wrong url input.\n");
		
	if (DEBUG){
		printf("\n<PARSE URL>\n");
		printf("user = %s\n", cInfo.user);
		printf("password = %s\n", cInfo.password);
		printf("hostname = %s\n", cInfo.hostname);
		printf("port = %s\n", cInfo.port);
		printf("filepath = %s\n", cInfo.filepath);
		printf("filename = %s\n\n", cInfo.filename);
	}

	ftpClient(cInfo);
	
	return 0;
}

void ftpClient(connectionInfo cInfo)
{
    //--- GET IP ---
    
	struct addrinfo hints;
	struct addrinfo *result;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0; //Any protocol
    
	int r = getaddrinfo(cInfo.hostname, cInfo.port, &hints, &result);
	if (r != 0) {
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(r));
		exit(-1);
	} 

	//--- SOCKET ---

	int sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	
	if(sd == -1){
		perror("Error opening socket");
		exit(-1);
	}
	
	//--- CONNECT ---
	 
	if(connect(sd, result->ai_addr, result->ai_addrlen) == -1){
		perror("Error connecting to server");
		exit(-1);
	}
	
	char response[RESP_BUFFER_SIZE];
	
	int received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from connect");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<CONNECT RESPONSE>\n%s\n", response);
	
	if(strncmp("220", response, 3) != 0){
		printf("Connection failed\n");
		exit(-1);
	}
	
	freeaddrinfo(result);

	//--- LOGIN ---
	
	char userCmd[128];
	char passCmd[128];
	
	sprintf(userCmd, "USER %s\r\n", cInfo.user);
	if (DEBUG) printf("<USER CMD>\n%s\n", userCmd);
		
	if(send(sd, userCmd, strlen(userCmd), 0) == -1){
		printf("Error sending USER cmd\n");
		exit(-1);
	}
		
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from USER");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<USER RESPONSE>\n%s\n", response);

	if(strncmp("331", response, 3) != 0){
		printf("Error in USER\n");
		exit(-1);
	}

	sprintf(passCmd, "PASS %s\r\n", cInfo.password);
	if (DEBUG) printf("<PASS CMD>\n%s\n", passCmd);
	
	if(send(sd, passCmd, strlen(passCmd), 0) == -1){
		printf("Error sending PASS cmd\n");
		exit(-1);
	}
			
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from PASS");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<PASS RESPONSE>\n%s\n", response);

	if(strncmp("530", response, 3) == 0){
		printf("Login incorrect\n");
		exit(-1);
	}
	else if(strncmp("230", response, 3) != 0)	{
		printf("Login failed\n");
		exit(-1);
	}
	
	char response2[128];
	while(strstr(response, "230 ") == NULL)
	{
		received = recv(sd, response2, 128, 0);
		if(received == -1){
			perror("Error receiving response from PASS");
			exit(1);
		}
		response2[received] = 0;
		strcat(response, response2);
	}
	//if (DEBUG) printf("<PASS FULL RESPONSE>\n%s\n", response);
	
	if (!DEBUG) printf("Login successful\n");

	//--- PASV ---
	
	if (DEBUG) printf("<PASV CMD>\nPASV\n\n");
	if(send(sd, "PASV\r\n", 6, 0) == -1){
		printf("Error sending PASV cmd\n");
		exit(-1);
	}
	
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from PASV");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<PASV RESPONSE>\n%s\n", response);
	
	if(strncmp(response, "227", 3) != 0){
		printf("PASV request not accepted\n");
		exit(-1);
	}	
	
	//--- 2ND SOCKET ---
	
	int i = 0;
	char *token;
	int parsedIP[6];
	token = strtok(response, "(,)");
	while( (token = strtok(NULL, "(,)")) != NULL && i < 6)
	{
		parsedIP[i] = atoi(token);
		//printf("Parsed value %d : %d\n", i, parsedIP[i]);
		i++;
	}
	if(i < 6){
		printf("Error parsing Pasv IP address.\n");
		exit(-1);
	}
	
	char pasvIpAddr[20];
	sprintf(pasvIpAddr, "%d.%d.%d.%d", parsedIP[0], parsedIP[1], parsedIP[2], parsedIP[3]);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( (parsedIP[4] << 8) + parsedIP[5] );
	inet_aton(pasvIpAddr, (struct in_addr*) &addr.sin_addr.s_addr);

	if (DEBUG) printf("Parsed Pasv IP address - %s:%d\n\n", pasvIpAddr, addr.sin_port);
	
	int sd2 = socket(AF_INET, SOCK_STREAM, 0);
	if( (connect(sd2, (const struct sockaddr*) &addr, sizeof(addr)) ) == -1){
		printf("Data connection not established.\n");
		exit(-1);
	}
	
	//--- TYPE ---
	
	if (DEBUG) printf("<TYPE CMD>\nTYPE I\n\n");
	if(send(sd, "TYPE I\r\n", 8, 0) == -1){
		printf("Error sending TYPE cmd\n");
		exit(-1);
	}
	
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from TYPE");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<TYPE RESPONSE>\n%s\n", response);
	
	if(strncmp(response, "200", 3) != 0){
		printf("TYPE request not accepted\n");
		exit(-1);
	}
	
	//--- SIZE ---
	
	char sizeCmd[512];
	if(strlen(cInfo.filepath) > 0)
		sprintf(sizeCmd, "SIZE %s/%s\r\n", cInfo.filepath, cInfo.filename);	
	else
		sprintf(sizeCmd, "SIZE %s\r\n", cInfo.filename);
			
	if (DEBUG) printf("<SIZE CMD>\nSIZE\n\n");
	if(send(sd, sizeCmd, strlen(sizeCmd), 0) == -1){
		printf("Error sending SIZE cmd\n");
		exit(-1);
	}
	
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from SIZE");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<SIZE RESPONSE>\n%s\n", response);
	
	if(strncmp("550", response, 3) == 0){
		printf("No such file or directory.\n");
		exit(-1);
	}
	else if(strncmp("213", response, 3) != 0){
		printf("No such file or directory.\n");
		exit(-1);
	}
	
	int filesize = atoi(response + 4);
	
	//--- RETR ---
	
	char retrCmd[512];
	if(strlen(cInfo.filepath) > 0)
		sprintf(retrCmd, "RETR %s/%s\r\n", cInfo.filepath, cInfo.filename);	
	else
		sprintf(retrCmd, "RETR %s\r\n", cInfo.filename);	
	
	if (DEBUG) printf("<RETR CMD>\n%s\n", retrCmd);
	if(send(sd, retrCmd, strlen(retrCmd), 0) == -1){
		printf("Error sending RETR cmd\n");
		exit(-1);
	}
	
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	if(received == -1){
		perror("Error receiving response from RETR");
		exit(1);
	}
	response[received] = 0;
	if (DEBUG) printf("<RETR RESPONSE>\n%s\n", response);
	
	if(strncmp("150", response, 3) != 0){
		printf("File is unavailable/not found.\n");
		exit(-1);
	}
	
	if (DEBUG) printf("Transfering file (bytes received)\n");
	else printf("Transfering file...\n");	

	//--- Transfer file  ---
	
	dataConnection(sd2, cInfo.filename, filesize);
	
	close(sd2);
	
	//--- END ---
	
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	if (DEBUG) printf("\n<RETR FINAL RESPONSE>\n%s\n", response);
	
	if(strncmp(response, "226", 3) != 0){
		printf("Transfer failed.\n");
		exit(-1);
	}
	else{
		printf("Transfer complete!\n");
	}
	
	close(sd);
}

void dataConnection(int sd2, char * filename, int filesize)
{
	char buffer[FILE_BUFFER_SIZE];
	int bytesReceived;
	int totalBytes = 0;
	
	int fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0777);
	
	while( (bytesReceived = recv(sd2, buffer, FILE_BUFFER_SIZE, 0)) != 0 )
	{		
		if(bytesReceived == -1){
			perror("Error receiving file data");
		}
		else{
			write(fd, buffer, bytesReceived);
			totalBytes += bytesReceived;			
		}
		if (DEBUG) printf("(%d)", bytesReceived);
		else loadingBar(totalBytes, filesize);
	}
	printf("\n");

	close(fd);
	
	if (totalBytes != filesize){
		printf("File size mismatch: %d (received) / %d (expected)\n", totalBytes, filesize);
		//TODO delete file
		exit(1);
	}
}

int parseUrl(char* url, connectionInfo *cInfo)
{
	int pos_arroba = -1;
	unsigned int i = 0;
	int anonymous = 1;
	int first_bar = -1;
	int second_bar = -1;
	int first_colon = -1;
	int second_colon = -1;
	int colon_count = 0;
	int third_bar = -1;
	
	//First cycle to determine if url has a valid syntax;
	for(i=0;i<strlen(url);i++){
		if (url[i] == '@'){
			anonymous = 0;
			pos_arroba = i;
		}
		if(url[i]=='/'){
			if(first_bar == -1)
				first_bar=i;
			else if(second_bar == -1){
				if(first_bar == i-1)
					second_bar = i;
				else
					return -1;
			}
			else if (third_bar == -1)
				third_bar = i;
		}
		if(url[i] == ':' && first_bar != -1 && second_bar != -1){
			if(colon_count > 1)
				return -3;
			else
				colon_count++;
		}
	}
	
	for(i=0; i < strlen(url); i++){
		if(anonymous==0){
			for (i=0; i < strlen(url); i++){
				if(url[i] == ':'&& i > second_bar ){
					if(first_colon == -1)					
						first_colon = i;
					else
						second_colon = i;
				}				
			}
		}
	
		if(anonymous ==1 && colon_count > 0){
			if(url[i] == ':' && i > second_bar)
				second_colon = i;
		}		
	}	
	
	int hostname_begin = 0;
	if (anonymous == 0)
	{
		memcpy(cInfo->user, url + second_bar + 1, first_colon - second_bar - 1);
		cInfo->user[first_colon - second_bar - 1] = '\0';
		
		memcpy(cInfo->password, url + first_colon + 1, pos_arroba - first_colon - 1);
		cInfo->password[pos_arroba - first_colon - 1] = '\0';
		
		hostname_begin = pos_arroba;
	}
	else //anonymous login => default user and pass
	{
		strcpy(cInfo->user, "ftp");
		strcpy(cInfo->password, "eu@");
		
		hostname_begin = second_bar;
	}
	
	if (second_colon != -1) //custom port
	{
		memcpy(cInfo->hostname, url + hostname_begin + 1, second_colon - hostname_begin - 1);
		cInfo->hostname[second_colon - hostname_begin - 1] = '\0';
	
		memcpy(cInfo->port, url + second_colon + 1, third_bar - second_colon - 1);
		cInfo->port[third_bar - second_colon - 1] = '\0';	
	}
	else //default port = 21
	{
		memcpy(cInfo->hostname, url + hostname_begin + 1, third_bar - hostname_begin - 1);
		cInfo->hostname[third_bar - hostname_begin - 1] = '\0';
	
		strcpy(cInfo->port, "21");
	}
	
	char *last_barPtr = strrchr(url, '/');
	int last_bar = last_barPtr - url;
	if(last_bar > third_bar){
		memcpy(cInfo->filepath, url + third_bar + 1, last_bar - third_bar - 1);
		cInfo->filepath[last_bar - third_bar - 1] = '\0';
	}
	else
		cInfo->filepath[0] = '\0';
	
	memcpy(cInfo->filename, last_barPtr + 1, strlen(url) - last_bar - 1);
	cInfo->filename[strlen(url) - last_bar - 1] = '\0';
		
	return 0;
}

void loadingBar(float file_size_processed, float file_total_size)
{
	float percentage = 100.0 * file_size_processed / file_total_size;
	printf("\rStatus: %6.2f%% [", percentage);	//display progress from 0 to 100%
	
	int i, len = 50;
	int pos = percentage * len / 100.0;
	
	//displays visual progress
	for (i = 0; i < len; i++){
		if(i <= pos)
			printf("=");
		else
			printf(" ");
		}

	printf("]");
	
	fflush(stdout);
}

