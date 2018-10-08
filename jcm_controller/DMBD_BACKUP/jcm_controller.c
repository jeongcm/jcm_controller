#include <stdio.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include "jcm_controller.h"
#include "json_file.h"
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define JCM_DEV "bestioa"
server_node *my_node;
serv_n *serv_node;
server_node *server_config;
char config_cmd[1024];
int count =0;
int nbd_count = 0;
int success_flag = 0;
int unsuccess_flag=0;
int server_len;
const char *dmbd_mode;
const char *dmbd_chunk_size;
int msg_send(server_node *con_arr, int con_len, char *buf)
{
	int client_sock;
	char buffer[1024];
	if(0>(client_sock= socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)))
	{
		printf("socket error\n");
	}
	struct sockaddr_in send_client_addr;

	int ret = 0;
	unsigned int len = sizeof(struct sockaddr_in);
	if(buf!=NULL)
	{
		bzero(&send_client_addr, sizeof(send_client_addr));
		for(int node_num =0;node_num<con_len;node_num++)
		{
			if(strcmp(con_arr[node_num].ip, my_node->ip)==0)
			{
				continue;
			}
			send_client_addr.sin_port = htons(atoi(con_arr[node_num].controller_port));
			send_client_addr.sin_family = AF_INET;
			send_client_addr.sin_addr.s_addr = inet_addr(con_arr[node_num].ip);
			int send_len = sizeof(send_client_addr);

			if(0>connect(client_sock, (struct sockaddr *)&send_client_addr, send_len))
			{
				printf("connect error, %s \n ",strerror(errno));
				return 0;
			}

			ret = write(client_sock, buf,strlen(buf));//sizeof(buf));

			int rtv = read(client_sock,buffer,sizeof(buffer));
			buffer[rtv]=0;
			if(!(strcmp(buffer,"CONTROL_SET_ON")))
			{
				int f;
				char config_index[4096];
				char msg_dmbd_config_cmd[1024];
				sprintf(msg_dmbd_config_cmd,"%s/dmbd_config",config_cmd);
				f= open(msg_dmbd_config_cmd,O_RDWR,0644);
				int rtv	=read(f,config_index,4096);
				config_index[rtv] = 0;
				int w_rtv = write(client_sock,config_index,strlen(config_index)+1);
			}

			rtv = read(client_sock,buffer,sizeof(buffer));
			buffer[rtv]=0;
			if(!strcmp(buffer,"success"))
			{
				count++;
			}
			else if(!strcmp(buffer,"unsuccess"))
			{
				unsuccess_flag=1;
			}
			if(0>ret )
			{
				printf("msg_send error\n %d,%s\n",ret,strerror(errno));
			}
		}
	}
	//	close(client_sock);
	return ret;

}
int mount_msg_send(server_node *con_arr, int con_len, char *buf)
{
	int client_sock;
	char buffer[1024];
	if(0>(client_sock= socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)))
	{
		printf("socket error\n");
	}
	struct sockaddr_in send_client_addr;

	int ret = 0;
	unsigned int len = sizeof(struct sockaddr_in);
	if(buf!=NULL)
	{
		bzero(&send_client_addr, sizeof(send_client_addr));
		for(int node_num =0;node_num<con_len;node_num++)
		{
			if(strcmp(con_arr[node_num].ip, my_node->ip)==0)
			{
				continue;
			}
			send_client_addr.sin_port = htons(atoi(con_arr[node_num].controller_port));
			send_client_addr.sin_family = AF_INET;
			send_client_addr.sin_addr.s_addr = inet_addr(con_arr[node_num].ip);
			int send_len = sizeof(send_client_addr);

			if(0>connect(client_sock, (struct sockaddr *)&send_client_addr, send_len))
			{
				printf("connect error, %s \n ",strerror(errno));
				return 0;
			}


			ret = write(client_sock, buf,strlen(buf));
			if(0>ret )
			{
				printf("msg_send error\n %d,%s\n",ret,strerror(errno));
			}


		}
	}
	//	close(client_sock);
	return ret;

}
int nbd_msg_send(server_node *con_arr, int con_len, char *buf)
{
	int client_sock;
	char buffer[1024];
	if(0>(client_sock= socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)))
	{
		printf("socket error\n");
	}
	struct sockaddr_in send_client_addr;

	int ret = 0;
	unsigned int len = sizeof(struct sockaddr_in);
	if(buf!=NULL)
	{
		bzero(&send_client_addr, sizeof(send_client_addr));
		for(int node_num =0;node_num<con_len;node_num++)
		{
			if(strcmp(con_arr[node_num].ip, my_node->ip)==0)
			{
				continue;
			}
			send_client_addr.sin_port = htons(atoi(con_arr[node_num].controller_port));
			send_client_addr.sin_family = AF_INET;
			send_client_addr.sin_addr.s_addr = inet_addr(con_arr[node_num].ip);
			int send_len = sizeof(send_client_addr);

			if(0>connect(client_sock, (struct sockaddr *)&send_client_addr, send_len))
			{
				printf("connect error, %s \n ",strerror(errno));
				return 0;
			}

			ret = write(client_sock, buf,strlen(buf));//sizeof(buf));

			int rtv = read(client_sock,buffer,sizeof(buffer));

			if(!strcmp(buffer,"NBD_CLIENT_LOAD"))
			{
				nbd_count++;
			}
			if(0>ret )
			{
				printf("msg_send error\n %d,%s\n",ret,strerror(errno));
			}
		}
	}
	//	close(client_sock);
	return ret;

}
int cm_accept(int sd, struct sockaddr_in address)
{
	int  client_socket = -1;
	int length       = sizeof(struct sockaddr_in);
	//	struct sockaddr_in address;

	client_socket = accept(sd, (struct sockaddr *)&address, (socklen_t *)&length);


	return client_socket;
}
static int reuseSocket(int socket)
{
	int enable = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,&enable, sizeof(int)) < 0)
	{
		perror ("reuse");
		return -1;
	}
	return 0;
}
server_node *check_my_node(server_node *serv,char *my_ip,int length)
{
	for(int i=0; i<length;i++)
	{
		if(!strcmp(serv[i].ip,my_ip))
		{
			return serv+i;
		}
	}
}

int main (int agrc, char* argv[])
{
	pid_t pid;
	int remote_dmbd_config_len;
	DMBD_CONFIG_REMOTE *dmbd_config_remote;
	DMBD_CONFIG_REMOTE *my_dmbd_config_remote;
	NBD_SERVER_CONFIG nbd_server_config;
	
	char *ip;
	char teng_ip[16];
	char con_port[16];
	ip = argv[1];//에러 처리 필요
	int serv_sock=0;
	int client_sock=0;
	struct sockaddr_in server_addr, client_addr;
	unsigned int cli_len;
	int fd;
	char buffer[4096];
	char module_cmd[1024];
	char bestio_cmd[1024];
	char server_config_cmd[1024];
	char dmbd_config_cmd[1024];
	char nbd_config_cmd[1024];
	char nbd_server_cmd[1024];
	char mount_config_cmd[1024];
	char mount_cmd[1024];
	char mount_msg[1024];
	char mount_path_cmd[1024];
	struct timeval to;
	char ip_buf[21];
	
	const char *dmbd_mount_ip;
	const char *dmbd_mount_path;
	const char *dmbd_mount_fs;
	FILE *mount_file;
	const char *cmd_command;
	const char *dmbd_send_mode;
	const char *dmbd_send_chunk_size;
	serv_node = (serv_n *)malloc(sizeof(serv_n));
	fd_set fs_status, copy_fs;
	json_object *myobj,*remote,*myobj2,*remote_dev,*new, *temp,*cmd_type, *mode, *chunk_size, *mount_ip,*mount_path,*mount_fs,*nbd_config,*nbd_generic, *nbd_ip, *nbd_port, *nbd_export, *nbd_exportdev;
	if (NULL == argv[1])
	{
		printf("cmd line not have ip!\n");	
		return 0;
	}
	if (NULL == argv[2])
	{
		printf("cmd line not have config!\n");	
		return 0;
	}
	/*if( 0> (pid = daemon(0,0)))
	{
		printf("daemonize fail!\n");
		return -1;
	}*/
	strcpy(config_cmd,argv[2]);
	sprintf(server_config_cmd,"%s/server_config",config_cmd);
	server_config=ReadServerConfig(server_config_cmd,&server_len);
	serv_node->serv_con = server_config;
	serv_node->len = server_len;
	my_node =check_my_node(server_config,ip,server_len);
	strcpy(con_port,my_node->controller_port);
	printf("con_port : %s\n",con_port);

	sprintf(dmbd_config_cmd,"%s/dmbd_config",config_cmd);
	sprintf(nbd_config_cmd,"%s/nbd_config",config_cmd);
	sprintf(nbd_server_cmd,"nbd-server -C %s",nbd_config_cmd);
	sprintf(mount_config_cmd,"%s/mount_file",config_cmd);


	serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	reuseSocket(serv_sock);
	if(0>serv_sock)
	{
		printf("server_sock is not created!!\n");
		return 0;
	}
	memset(&server_addr,0,sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(my_node->ip);
	server_addr.sin_port = htons(atoi(con_port)); 

	fd = bind(serv_sock,(struct sockaddr *) &server_addr, sizeof(server_addr));
	if(0> fd)
	{
		printf("bind error\n");
		close(serv_sock);
		return 0;
	}
	//	sleep(2);

	fd = listen(serv_sock,SOMAXCONN);

	if(0> fd)
	{
		printf("listen fail\n");
		close(serv_sock);
	}

	int	maxfd = serv_sock;
	FD_ZERO(&fs_status);
	FD_SET(serv_sock, &fs_status);
	while(1)
	{
		to.tv_sec = 2;
		to.tv_usec = 0;
		copy_fs = fs_status;
		int result = select( maxfd+1, &copy_fs, 0, 0,&to);
		if(0>result)
		{
			printf("select error\n");
			continue;
		}
		if(result ==0)
		{
			printf("time over!\n");
			continue;
		}

		for(int i =0;i <maxfd+1;i++)
		{	
			if(FD_ISSET(i, &copy_fs))
			{
				if(i== serv_sock)
				{
					client_sock = cm_accept(i,client_addr);
					if(0> client_sock)
					{
						printf("accept failed");
						continue;
					}
					FD_SET(client_sock, &fs_status);
					if(maxfd<client_sock)
					{	
						maxfd = client_sock;
					}
					//get_ip(client_sock, ip_buf);
				}
				else
				{
					int rtv = read(i,buffer,sizeof(buffer));
					buffer[rtv]=0;

					if(rtv == 0)
					{
						close(i);
						FD_CLR(i, &fs_status);
					}
					else
					{
						myobj = json_tokener_parse(buffer);
						json_object_object_get_ex(myobj,"cmd_type",&cmd_type);
						cmd_command = json_object_get_string(cmd_type);
						if(cmd_command == NULL)
						{
							if(success_flag == 1)
							{
								continue;
							}
							int config;
							config = open(dmbd_config_cmd,O_RDWR|O_CREAT|O_TRUNC,0644);
							write(config,buffer,strlen(buffer));
							dmbd_config_remote = ReadDMBDRemote(dmbd_config_cmd,&remote_dmbd_config_len);
							for(int i =0; i<remote_dmbd_config_len;i++)
							{
									if(!strcmp(dmbd_config_remote[i].ip,my_node->ip))
									{
										sprintf(bestio_cmd,"insmod %s/bestio/go.ko memory_size=%lu back_major=8 back_minor=32",config_cmd,dmbd_config_remote[i].dev_size);
									}

							}
							int module_return = system(bestio_cmd);
							if(module_return != 0)
							{
								continue;
							}

							if(module_return == 0)
							{

								for(int i =0;i<remote_dmbd_config_len;i++)
								{
										if(!strcmp(dmbd_config_remote[i].ip,my_node->ip))
										{

											FILE *nbd_config;
											char nbd_msg[1024];
											nbd_config = fopen(nbd_config_cmd,"w+");
											sprintf(nbd_msg,"[generic]\n    listenaddr=%s\n    port=%s\n[%s]\n    exportname = /dev/%s",dmbd_config_remote[i].ip,dmbd_config_remote[i].nbd_port,dmbd_config_remote[i].nbd_export,JCM_DEV);
											fprintf(nbd_config,nbd_msg);
											fclose(nbd_config);
											system(nbd_server_cmd);
										}

								}
								char success[1024];
								memset(success, '\0', strlen(success));
								strcpy(success, "success");
								int ytv= write(i,success, strlen(success)+1);

								if(ytv<0)
								{
									printf("write success_ error\n");
								}
							}
							else
							{
								char unsuccess[1024];
								memset(unsuccess, '\0', strlen(unsuccess));
								strcpy(unsuccess, "unsuccess");
								int ytv= write(i,unsuccess, strlen(unsuccess)+1);

							}


						}
						else if(!strcmp(cmd_command,"USER_SET_CONFIG"))
						{

							//json_object_object_get_ex(myobj,"mode",&mode);
							//json_object_object_get_ex(myobj,"chunk_size",&chunk_size);
							//dmbd_send_mode = json_object_get_string(mode);
							//dmbd_send_chunk_size = json_object_get_string(chunk_size);
							char *controller_cmd = (char *)malloc(1024);

							//sprintf(controller_cmd,"{ \"cmd_type\": \"CONTROLLER_SET_CONFIG\", \"mode\": \"%s\", \"chunk_size\" : \"%s\"}",dmbd_send_mode,dmbd_send_chunk_size);
							sprintf(controller_cmd,"{ \"cmd_type\": \"CONTROLLER_SET_CONFIG\"}");

							char str[1024];
							strcpy(str, controller_cmd);
							if(server_len >1)
							{
								if( 0> msg_send(server_config,server_len,str))
								{
									printf("msg_send error\n");
								}
							}
						}
						else if(!strcmp(cmd_command,"CONTROLLER_SET_CONFIG"))
						{

							//json_object_object_get_ex(myobj,"mode",&mode);
							//json_object_object_get_ex(myobj,"chunk_size",&chunk_size);
							//dmbd_mode = json_object_get_string(mode);
							//dmbd_chunk_size = json_object_get_string(chunk_size);
							char *reply_cmd = (char *)malloc(1024);
							reply_cmd = "CONTROL_SET_ON";
							char str[1024];
							strcpy(str, reply_cmd);
							int rtv = write(i,str,strlen(str)+1);
							if( 0> rtv)
							{
								printf("msg_send error\n");
							}
						}
						else if(!strcmp(cmd_command,"mount"))
						{
							int ret;
							json_object_object_get_ex(myobj,"mount_ip", &mount_ip);
							json_object_object_get_ex(myobj,"mount_path", &mount_path);
							json_object_object_get_ex(myobj,"mount_fs", &mount_fs);
							dmbd_mount_ip = json_object_get_string(mount_ip);
							dmbd_mount_path = json_object_get_string(mount_path);
							dmbd_mount_fs = json_object_get_string(mount_fs);
							if(access(dmbd_mount_path,F_OK)!=0)
							{
								ret = mkdir(dmbd_mount_path,0776);
								if(ret== -1 && errno != EEXIST)
								{
									printf("mkdir mount error %s\n",strerror(errno));
								}			
							}

							sprintf(mount_path_cmd,"/dev/%s",JCM_DEV);
							int mount_rtv = mount(mount_path_cmd,dmbd_mount_path,dmbd_mount_fs,0,NULL);
							if(mount_rtv < 0)
							{
								printf("mount error %s \n",strerror(errno));
								continue;
							}

							mount_file = fopen(mount_config_cmd,"w");
							sprintf(mount_msg,"mount_ip is = %s\n",my_node->ip);
							fprintf(mount_file,mount_msg);
							fclose(mount_file);
							sprintf(mount_cmd,"{ \"cmd_type\": \"MOUNT_SET_ON\", \"mount_ip\": \"%s\"}",my_node->ip);

							char str[1024];
							strcpy(str, mount_cmd);
							if( 0> mount_msg_send(server_config,server_len,str))
							{
								printf("msg_send error\n");
							}
						}
						else if(!strcmp(cmd_command,"MOUNT_SET_ON"))
						{
							printf("dmbd mount file stored in here\n");
							json_object_object_get_ex(myobj,"mount_ip", &mount_ip);

							dmbd_mount_ip = json_object_get_string(mount_ip);
							printf("dmbd_mount_ip: %s\n",dmbd_mount_ip);
							mount_file = fopen(mount_config_cmd,"w");
							sprintf(mount_msg,"mount_ip is = %s\n",dmbd_mount_ip);
							fprintf(mount_file,mount_msg);
							fclose(mount_file);

						}
						else if(!strcmp(cmd_command,"NBD_SET_ON"))
						{
							dmbd_config_remote = ReadDMBDRemote(dmbd_config_cmd,&remote_dmbd_config_len);
						/*	for(int i =0;i<remote_dmbd_config_len;i++)
							{
									if(strcmp(dmbd_config_remote[i].ip,ip)!=0)
									{

										char nbd_msg[1024];
										sprintf(nbd_msg,"nbd-client %s %s %s -p -N %s",dmbd_config_remote[i].ip,dmbd_config_remote[i].nbd_port, dmbd_config_remote[i].dev_name,dmbd_config_remote[i].nbd_export);
										system(nbd_msg);
									}

							}*/
							char nbd_client_success[1024];
							strcpy(nbd_client_success, "NBD_CLIENT_LOAD");
							int ytv= write(i,nbd_client_success, strlen(nbd_client_success)+1);

							if(ytv<0)
							{
								printf("nbd_client write error\n");
							}
						}
						if(count == server_len-1)
						{
							//자기 모듈 내리기위해 file 불러와서 처리
							my_dmbd_config_remote = ReadDMBDRemote(dmbd_config_cmd,&remote_dmbd_config_len);
							for(int i =0; i<remote_dmbd_config_len;i++)
							{
									if(!strcmp(my_dmbd_config_remote[i].ip,my_node->ip))
									{
										sprintf(bestio_cmd,"insmod %s/bestio/go.ko memory_size=%lu back_major=8 back_minor=32",config_cmd,my_dmbd_config_remote[i].dev_size);
										system(bestio_cmd);
										
									}
									if(strcmp(my_dmbd_config_remote[i].ip,my_node->ip))
									{	
										char nbd_msg[1024];
										sprintf(nbd_msg,"nbd-client %s %s -p %s -N %s",my_dmbd_config_remote[i].ip,my_dmbd_config_remote[i].nbd_port,my_dmbd_config_remote[i].dev_name,my_dmbd_config_remote[i].nbd_export);
										printf("%s\n",nbd_msg);
										if(system(nbd_msg)!=0)
										{
											printf("nbd-client error\n");
										}
									}

							}
							sprintf(module_cmd, "insmod /root/jcm_controller/vrd_2018_04_18.ko");
								//sprintf(module_cmd, "insmod %s/dmbd_module/dmbd.ko mode=%s disk_total=%d chunk_size=%s local_ip=%s local_dev=/dev/bestioa",config_cmd,dmbd_send_mode,server_len,dmbd_send_chunk_size,ip);
							int module_return = system(module_cmd);
							if(module_return == 0)
							{
							/*	for(int i =0;i<remote_dmbd_config_len;i++)
								{
										if(!strcmp(my_dmbd_config_remote[i].ip,my_node->ip))
										{

											FILE *nbd_config;
											char nbd_msg[1024];
											nbd_config = fopen(nbd_config_cmd,"w+");
											sprintf(nbd_msg,"[generic]\n    listenaddr=%s\n    port=%s\n[%s]\n    exportname = /dev/%s",my_dmbd_config_remote[i].ip,my_dmbd_config_remote[i].nbd_port,my_dmbd_config_remote[i].nbd_export,JCM_DEV);
											fprintf(nbd_config,nbd_msg);
											fclose(nbd_config);
											int sys_ret =system(nbd_server_cmd);
											if(sys_ret!=0)
											{
												printf("system error\n");
											}
										}

								}*/
								char nbd_client_msg[1024];
								sprintf(nbd_client_msg,"{ \"cmd_type\": \"NBD_SET_ON\"}");
								if(count != 0)
								{
									if( 0> nbd_msg_send(server_config,server_len,nbd_client_msg))
									{
										printf("msg_send error\n");
									}
								}
							}
							count =0;
						}
						if(nbd_count == server_len-1)
						{
							//my_nbd_client 실행
							my_dmbd_config_remote = ReadDMBDRemote(dmbd_config_cmd,&remote_dmbd_config_len);
							for(int i =0;i<remote_dmbd_config_len;i++)
							{
									if(strcmp(my_dmbd_config_remote[i].ip,ip)!=0)
									{

									}

							}
							success_flag = 1;
							write(client_sock,"success",strlen("success"));
							nbd_count=0;
						}

						if(unsuccess_flag ==1)
						{
							int rtv = write(client_sock,"unsuccess",strlen("unsuccess")+1);
							unsuccess_flag =0;
						}
						else
						{
							continue;
						}
					}
				}

			}

		}

	}
}
