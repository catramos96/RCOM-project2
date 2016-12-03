#include "download.h"

int main(int argc, char** argv)
{
	if (argc!=2)
	{
		printf("Wrong number of arguments\n");
		return -1;
	}
	
	connectionInfo cInfo;
	
	if( parse_url(argv[1], &cInfo) != 0 )
		printf("Wrong url input.\n");
		
	/*printf("user = %s\n", cInfo.user);
	printf("password = %s\n", cInfo.password);
	printf("hostname = %s\n", cInfo.hostname);
	printf("port = %s\n", cInfo.port);
	printf("filepath = %s\n", cInfo.filepath);
	printf("filename = %s\n", cInfo.filename);*/	

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
		perror("couldn't open socket, aborting:");
		exit(-1);
	}
	
	//--- CONNECT ---
	 
	if(connect(sd, result->ai_addr, result->ai_addrlen) == -1){
		perror("couldn't connect to server, aborting:");
		exit(-1);
	}
	
	char response[RESP_BUFFER_SIZE];
	
	int received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<CONNECT RESPONSE>\n%s\n", response);
	
	if(strncmp("220", response, 3)){
		printf("No 220 code received");
		exit(-1);
	}
	
	freeaddrinfo(result);

	//--- LOGIN ---
	
	char userCmd[128];
	char passCmd[128];
	
	sprintf(userCmd, "USER %s\r\n", cInfo.user);
	int sent = send(sd, userCmd, strlen(userCmd), 0);	
	if(sent <= 0){
		printf("Error\n");
		exit(-1);
	}
		
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<USER RESPONSE>\n%s\n", response);

	if(strncmp("331", response, 3) != 0){
		printf("Error\n");
		exit(-1);
	}

	sprintf(passCmd, "PASS %s\r\n", cInfo.password);	
	sent = send(sd, passCmd, strlen(passCmd), 0);
	if(sent <= 0){
		printf("Error\n");
		exit(-1);
	}
			
	sleep(1);
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<PASS RESPONSE>\n%s\n", response);

	if(strncmp("530", response, 3) == 0){
		printf("Login incorrect\n");
		exit(-2);
	}
	else if(strncmp("230", response, 3) != 0)
	{
		printf("Error\n");
		exit(-1);
	}
	
	printf("Login successful\n\n");

	//--- PASV ---
	
	send(sd, "PASV\r\n", 6, 0);
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<PASV RESPONSE>\n%s\n", response);
	
	if(strncmp(response, "227", 3) != 0){
		printf("PASV request not accepted\n");
		exit(-1);
	}	
	
	//--- 2ND SOCKET ---
	
	int i = 0;
	char *dump;
	char port[16][14]; //stack smashing prevention 
	dump = strtok(response, "(,)");
	while((dump = strtok(NULL, "(,)")) != NULL)
	{		
		strcpy(port[i++], dump);
		//puts(dump);
	}
	if(i < 6){
		printf("Address parsing failed. Exiting.\n");
	}
	
	int parsedIP[6];
	for(i = 0; i < 6; i++){
		parsedIP[i] = atoi(port[i]);
		//printf("%d - %d\n", i, parsedIP[i]);
	}
	
	char ipaddr[21];
	sprintf(ipaddr,  "%d.%d.%d.%d", parsedIP[0], parsedIP[1], parsedIP[2], parsedIP[3]);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( (parsedIP[4] << 8) + parsedIP[5]);
	inet_aton(ipaddr, (struct in_addr*) &addr.sin_addr.s_addr);

	//printf("%s:%d\n", ipaddr, addr.sin_port);
	
	int sd2 = socket(AF_INET, SOCK_STREAM, 0);
	if( ( connect(sd2,(const struct sockaddr*) &addr, sizeof(addr)) ) == -1){
		puts("Passive connection not made");
	}
	
	//--- TYPE ---
	
	send(sd, "TYPE I\r\n", 8, 0);
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<TYPE RESPONSE>\n%s\n", response);
	
	if(strncmp(response, "200", 3) != 0){
		printf("TYPE request not accepted\n");
		exit(-1);
	}
	
	//--- RETR ---
	
	char retrCmd[512];
	int retrCmdLength;
	if(strlen(cInfo.filepath) != 0)
		retrCmdLength = snprintf(retrCmd, 512, "RETR %s/%s\r\n", cInfo.filepath, cInfo.filename);	
	else
		retrCmdLength = snprintf(retrCmd, 512, "RETR %s\r\n", cInfo.filename);	
	
	send(sd, retrCmd, retrCmdLength, 0);
	received = recv(sd, response, RESP_BUFFER_SIZE, 0);
	response[received] = 0;
	//printf("<RETR RESPONSE>\n%s\n", response);
	
	if(strncmp("150", response, 3)){
		puts("file is unavailable at this time");
		return -1;
	}
	
	printf("Downloading file...\n");

	char buffer[1024];
	int bytesread;
	
	int fd = open(cInfo.filename, O_CREAT|O_TRUNC|O_WRONLY, 0777);
	
	while( bytesread = recv(sd2, buffer, 1024, 0) )
	{
		//printf("%d ", bytesread);
		write(fd, buffer, bytesread);
	}
	
	printf("File downloaded!\n");
	
	close(sd);
	
	return 0;
}

int parse_url(char* url, connectionInfo *cInfo)
{
	int pos_arroba = -1;
	unsigned int i = 0;
	int anonymous = 1;
	int first_bar = -1;
	int second_bar = -1;
	//int path_bar = -1;
	int first_colon = -1;
	int second_colon = -1;
	int colon_count = 0;
	int third_bar = -1;
	//int fourth_bar = -1;
	
	//First cycle to determine if url has a valid syntax;
	for(i=0;i<strlen(url);i++)
	{
		if (url[i] == '@')
		{
			anonymous = 0;
			pos_arroba = i;
		}
		if(url[i]=='/')
		{
				if(first_bar == -1)
					first_bar=i;
				else if(second_bar == -1)
				{
					if(first_bar == i-1)
						second_bar = i;
					else
						return -1;
				}
				else if (third_bar == -1)
				{
					third_bar = i;
				}
				//else if (fourth_bar==-1)
				//{
				//	fourth_bar=i;
				//}
				//else
				//	return -2;
		}
		if(url[i] == ':' && first_bar != -1 && second_bar != -1)
		{
			if(colon_count > 1)
				return -3;
			else
			{
				colon_count++;
			}
		}
	}	
	
	
	for(i=0; i < strlen(url); i++)
	{
		if(anonymous==0)
			{
				for (i=0; i < strlen(url); i++)
				{
					if(url[i] == ':'&& i > second_bar )
					{
						if(first_colon == -1)
						{
							first_colon = i;
						}
						else
							second_colon = i;
					}
					
				}
			}
	
		if(anonymous ==1 && colon_count > 0)
		{
			if(url[i] == ':' && i > second_bar)
			{
				second_colon = i;
			}
		}		
	}
	
	/*
	printf("anonymous = %d\n",anonymous);
	printf("first_bar = %d\n",first_bar);
	printf("second_bar = %d\n",second_bar);
	printf("third_bar = %d\n",third_bar);
	//printf("fourth_bar = %d\n",fourth_bar);
	printf("first_colon = %d\n",first_colon);
	printf("second_colon = %d\n",second_colon);
	printf("pos_arroba = %d\n\n",pos_arroba);
	*/
	
	int hostname_begin = 0;
	if (anonymous == 0)
	{
		memcpy(cInfo->user, url + second_bar + 1, first_colon - second_bar - 1);
		cInfo->user[first_colon - second_bar - 1] = '\0';
		
		memcpy(cInfo->password, url + first_colon + 1, pos_arroba - first_colon - 1);
		cInfo->password[pos_arroba - first_colon - 1] = '\0';
		
		hostname_begin = pos_arroba;
	}
	else
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
	else
	{
		memcpy(cInfo->hostname, url + hostname_begin + 1, third_bar - hostname_begin - 1);
		cInfo->hostname[third_bar - hostname_begin - 1] = '\0';
	
		strcpy(cInfo->port, "21");
	}	

	
	char *last_barPtr = strrchr(url, '/');
	int last_bar = last_barPtr - url;
	if(last_bar > third_bar){
		memcpy(cInfo->filepath, url + third_bar + 1, last_bar - third_bar - 1);
		cInfo->filepath[strlen(url) - third_bar - 1] = '\0';
	}
	else
		cInfo->filepath[0] = '\0';
	
	memcpy(cInfo->filename, last_barPtr + 1, strlen(url) - last_bar - 1);
	cInfo->filename[strlen(url) - last_bar - 1] = '\0';
		
	return 0;
}

