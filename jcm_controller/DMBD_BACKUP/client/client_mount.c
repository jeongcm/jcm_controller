#include <stdio.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/times.h>
#include <pthread.h>
#include <time.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define BUF_SIZE 1024
#define msg_flag 0
int msg_send(char *ip, const char *port, const char *path, const char *fs);
int main(int argc, char **argv)
{
	int   param_opt;
	char ip[16];
	char port[16];
	char mount[32];
	char path[32];
	char fs[32];

	opterr   = 0;
	while( -1 !=( param_opt = getopt( argc, argv, "hi:n:p:f:s")))
	{
		switch( param_opt)
		{
			case  'h'   :  printf( "help\n -i : ip \n -n : port \n -p : path \n -f : fs \n -s : msg_send \n");
						   break;
			case  'i'   :  printf( "check i\n");
							strcpy(ip, argv[2]);
							printf("%s\n",ip);
						   break;
			case  'n'   :  printf( "check n\n");
							strcpy(port, argv[4]);
							printf("%s\n",port);
						   break;
			case  'p'   :  printf( "check p\n");
							strcpy(path, argv[6]);
							printf("%s\n",path);
						   break;
			case  'f'   :  printf( "check f\n");
							strcpy(fs, argv[8]);
							printf("%s\n",fs);
						   break;
			case  's'   :  printf( "check s\n");
							msg_send(ip,port,path,fs);
						   break;
			case  '?'   :  printf( "알 수 없는 옵션: %c\n", optopt);
						   break;
		}
	}
	return 0;

}
int msg_send(char *ip, const char *port ,const char *path, const char *fs)
{
	int client_sock;
	int client_len;
	char *buffer = (char *)malloc(BUF_SIZE);
	struct sockaddr_in clientaddr;
	char buf[1024];
	client_sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(ip);
	clientaddr.sin_port = htons(atoi(port));
	client_len = sizeof(clientaddr);

	if(0>connect(client_sock, (struct sockaddr *)&clientaddr, client_len))
	{
		printf("connect error\n");
		return 0;
	}

		sprintf(buffer,"{ \"cmd_type\": \"mount\", \"mount_path\": \"%s\", \"mount_fs\": \"%s\"}", path,fs);
		printf("%s\n",buffer);
		write(client_sock, buffer, BUF_SIZE);
/*		do
		{
			read(client_sock,buf,sizeof(buf));
			if(!strcmp(buf,"success"))
			{
				printf("module load success\n");
				break;
			}
			if(!strcmp(buf,"unsuccess"))
			{
				printf("module load fail\n");
				break;
			}
		}while(1);*/
//
		
		
	close(client_sock);

}
