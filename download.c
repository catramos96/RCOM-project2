#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>

int parse_url(char* url, char* user, char* password, char* hostname, char* port, char* filepath, char* filename);

int main(int argc, char** argv)
{
	if (argc!=2)
	{
		printf("Wrong number of arguments\n");
		return -1;
	}
	
	char user[256]; 
	char password[256]; 
	char hostname[256];
	char port[256];
	char filepath[256];
	char filename[256];
	
	if(parse_url(argv[1], user, password, hostname, port, filepath, filename)!=0)
		printf("Wrong url input.\n");
		
	printf("user = %s\n", user);
	printf("password = %s\n", password);
	printf("hostname = %s\n", hostname);
	printf("port = %s\n", port);
	printf("filepath = %s\n", filepath);
	printf("filename = %s\n", filename);	

	struct addrinfo hints;
	struct addrinfo *result;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0; //Any protocol
    
    int r = getaddrinfo(hostname, port, &hints, &result);
	if (r != 0) {
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(r));
		exit(EXIT_FAILURE);
	} 

	int sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	
	if(sd == -1){
		perror("couldn't open socket, aborting:");
		exit(-1);
	}	
	 
	if(connect(sd, result->ai_addr, result->ai_addrlen) == -1){
		perror("couldn't connect to server, aborting:");
		exit(-1);
	}
	
	freeaddrinfo(result);
	
	
	/*	
	char buf[1024];
	
	send(sd, buf, 1024, 0);
	
	recv(sd, buf, 1024, 0);
	*/
		
		
	return 0;
}

int parse_url(char* url, char* user, char* password, char* hostname, char* port, char* filepath, char* filename)
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
		memcpy(user, url + second_bar + 1, first_colon - second_bar - 1);
		user[first_colon - second_bar] = '\0';
		
		memcpy(password, url + first_colon + 1, pos_arroba - first_colon - 1);
		password[pos_arroba - first_colon - 1] = '\0';
		
		hostname_begin = pos_arroba;
	}
	else
	{
		strcpy(user, "anonymous");
		strcpy(password, "-");
		
		hostname_begin = second_bar;
	}
	
	if (second_colon != -1) //custom port
	{
		memcpy(hostname, url + hostname_begin + 1, second_colon - hostname_begin - 1);
		hostname[second_colon - hostname_begin - 1] = '\0';
	
		memcpy(port, url + second_colon + 1, third_bar - second_colon - 1);
		port[third_bar - second_colon - 1] = '\0';	
	}
	else
	{
		memcpy(hostname, url + hostname_begin + 1, third_bar - hostname_begin - 1);
		hostname[third_bar - hostname_begin - 1] = '\0';
	
		strcpy(port, "21");
	}	

	
	char *last_barPtr = strrchr(url, '/');
	int last_bar = last_barPtr - url;
	if(last_bar > third_bar){
		memcpy(filepath, url + third_bar + 1, last_bar - third_bar - 1);
		filepath[strlen(url) - third_bar - 1] = '\0';
	}
	else
		filepath[0] = '\0';
	
	memcpy(filename, last_barPtr + 1, strlen(url) - last_bar - 1);
	filename[strlen(url) - last_bar - 1] = '\0';
		
	return 0;
}

