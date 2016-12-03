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
#include <sys/stat.h>
#include <fcntl.h>

#define RESP_BUFFER_SIZE 1024

typedef struct connectionInfo{
	char user[128];
	char password[128];
	char hostname[128];
	char port[8];
	char filepath[256];
	char filename[128];

}connectionInfo;

int parse_url(char* url, connectionInfo* connInfo);
