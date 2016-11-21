#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>

int main(int argc, char** argv)
{
	if (argc!=2)
	{
		printf("Wrong number of arguments\n");
		return -1;
	}
	
	if(parse_url(argv[1])!=0)
		printf("Wrong url input.\n");
	return 0;
}

int parse_url(char* url)
{
	int pos_arroba=-1;
	unsigned int i=0;
	int anonymous;
	int first_bar=-1;
	int second_bar=-1;
	int path_bar=-1;
	int first_colon=-1;
	int second_colon=-1;
	int colon_count=0;
	int third_bar=-1;
	int fourth_bar=-1;
	
	//First cycle to determine if url has a valid syntax;
	for(i=0;i<strlen(url);i++)
	{
		if (url[i]=='@')
		{
			anonymous=0;
			pos_arroba=i;
		}
		if(url[i]=='/')
		{
				if(first_bar ==-1)
					first_bar=i;
				else if( second_bar ==-1)
				{
					if(first_bar==(i-1))
						second_bar=i;
					else
						return -1;
				}
				else if (third_bar==-1)
				{
					third_bar=i;
				}
				else if (fourth_bar==-1)
				{
					fourth_bar=i;
				}
				else
					return -2;
		}
		if(url[i]==':' && first_bar!=-1 && second_bar!=-1)
			if(colon_count>1)
				return -3;
			else
			{
				colon_count++;
			}
	}
	
	

	
	
	
	for(i=0;i<strlen(url);i++)
	{
		if(anonymous==0)
			{
				for (i=0;i<strlen(url);i++)
				{
					if(url[i]==':'&& i>second_bar )
					{
						if(first_colon==-1)
						{
							first_colon=i;
						}
						else
							second_colon=i;
					}
					
				}
			}
	
		if(anonymous==1 && colon_count>0)
		{
			if(url[i]==':'&& i>second_bar)
			{
				second_colon=i;
			}
		}
		
		
	}
	
	printf("anonymous = %d\n",anonymous);
	printf("first_bar = %d\n",first_bar);
	printf("second_bar = %d\n",second_bar);
	printf("third_bar = %d\n",third_bar);
	printf("fourth_bar = %d\n",fourth_bar);
	printf("first_colon = %d\n",first_colon);
	printf("second_colon = %d\n",second_colon);
	printf("pos_arroba = %d\n",pos_arroba);
	
	return 0;
}
