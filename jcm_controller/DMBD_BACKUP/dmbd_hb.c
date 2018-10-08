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
#include <sys/ioctl.h>
#include "dmbd_hb.h"
#include "json_file.h"
#include "dmbd_ioctl.h"
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <syslog.h>

extern server_node *my_node;
extern server_node *server_config;
extern serv_n *serv_node;
extern char config_cmd[1024];
FILE *hb_state;
json_object *new, *node, *array,*connect_ip, *connect_state;
struct sockaddr_in recv_addr;
struct sockaddr_in sender_addr;
static pthread_t        hb_receiver_tid;
static pthread_t        hb_sender_tid;
double bounds = 1000;
static int hb_alive = 0;
int recv_socket = -1;
int recv_ret = 0;
char *port;
char *ip;
char *gateway;
int ioctl_fd;
char* recv_data = NULL;
FILE *hb_state;
int in_cksum (u_short *addr, unsigned int len)
{   
	size_t              nleft = len;
	u_short*    w = addr;
	int                 sum = 0; 
	u_short             answer = 0;

	/*
	 *      * The IP checksum algorithm is simple: using a 32 bit accumulator (sum)
	 *           * add sequential 16 bit words to it, and at the end, folding back all
	 *                * the carry bits from the top 16 bits into the lower 16 bits.
	 *                     */
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	/* Mop up an odd byte, if necessary */
	if (nleft == 1) {
		sum += *(u_char*)w;
	}

	/* Add back carry bits from top 16 bits to low 16 bits */

	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16);         /* add carry */
	answer = ~sum;              /* truncate to 16 bits */

	return answer;
}



int send_icmp(int fd, int dst, int id, int seqn, char *data, int size)
{
	struct sockaddr_in   to;
	struct icmp         *icmp = NULL;
	char                 buff[1024];

	memset(buff, 0x00, 1024);
	icmp             = (struct icmp *)buff;
	icmp->icmp_type  = ICMP_ECHO;
	icmp->icmp_code  = 0;
	icmp->icmp_id    = htons(id);
	icmp->icmp_seq   = htons(seqn);
	icmp->icmp_cksum = 0;
	if (size) bcopy(data, buff+sizeof(struct icmp), size);
	icmp->icmp_cksum = in_cksum((unsigned short *)buff, sizeof(struct icmp)+size);

	bzero(&to, sizeof(struct sockaddr_in));
	to.sin_family      = AF_INET;
	to.sin_port        = htons(0);
	to.sin_addr.s_addr = dst;

	return (sendto(fd, buff, sizeof(struct icmp) + size, 0,
				(struct sockaddr *)&to, sizeof(struct sockaddr_in)));
}

int tsdiff_in_bounds(struct timespec ts1, struct timespec ts2, unsigned long bounds, int *diff)
{
	*diff = (ts2.tv_sec - ts1.tv_sec) * 1000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000000;

	if (*diff < 0) *diff = -*diff;
	if (*diff < bounds) return (1);

	else return 0;
}


int nu_select(int pr_fd, int pr_sec, int pr_usec)
{
	fd_set         read_fs;
	int            ret;
	struct timeval to = {0, 0};


	to.tv_sec   = pr_sec;
	to.tv_usec  = pr_usec;

	FD_ZERO(&read_fs);
	FD_SET(pr_fd, &read_fs);

	return select(pr_fd+1, &read_fs, NULL, NULL, &to);
}
int recv_icmp(int fd, int dst, int id, int seqn, int *count, int wait)
{
	struct sockaddr_in  from;
	char                buff[1024];
	struct msghdr       msg;
	struct iovec        iov;
	struct ip          *ip   = NULL;
	struct icmp        *icmp = NULL;
	int                 diff = 0;
	char                ipstr[17];
	struct timespec     start, now, *d_tv;
	int                 msize = sizeof(struct ip) + sizeof(struct icmp) + sizeof(struct timeval);
	int                 err;

	clock_gettime(CLOCK_REALTIME, &start);

	while (1)
	{
		clock_gettime(CLOCK_REALTIME, &now);

		if( !(tsdiff_in_bounds(start, now, wait, &diff)) )
		{
			return (-1);
		}

		memset(&msg, 0, sizeof(struct msghdr) );
		memset(&buff, 0, 1024);

		msg.msg_namelen = sizeof(struct sockaddr_in);
		msg.msg_name    = (void *)&from;
		iov.iov_base    = buff;
		iov.iov_len     = 1024;
		msg.msg_iov     = &iov;
		msg.msg_iovlen  = 1;
#if   defined( __HPUX__ )
		msg.msg_control     = NULL;
		msg.msg_controllen  = 0;
#endif

#ifdef __SUNOS__
		err = nu_select(fd, MDP_NET_TIMEOUT_SEC, MDP_NET_TIMEOUT_USEC + 1000);
		if( err == 0 )                  //timeout
			continue;
		else if( err == -1 )    //socket fail
			return (-1);
#endif
		if (0 > (err = recvmsg(fd, &msg, 0)))
		{       
			continue;
		}

		if (err >= msize)
		{
			ip    = (struct ip *)buff;
			icmp  = (struct icmp *)(buff + (ip->ip_hl << 2));
			d_tv  = (struct timespec *)(buff + sizeof(struct ip) + sizeof(struct icmp));

			if( icmp->icmp_type == ICMP_ECHOREPLY )
			{
#if   defined( __HPUX__ ) // for PARISK( LP64 )
				struct ip     tmp_ip;


				memcpy( &tmp_ip, buff, sizeof(struct ip) );
#endif
#if   defined( __HPUX__ ) // for PARISK( LP64 )
				if (tmp_ip.ip_src.s_addr != dst)
				{
					continue;
				}
#else
				if (from.sin_addr.s_addr != dst) continue;
#endif
				if (icmp->icmp_seq != htons(seqn))
				{
					continue;
				}
				if (icmp->icmp_id != htons(id))
				{
					continue;
				}

				// debug message
				inet_ntop(AF_INET, &dst, ipstr, 16);
				tsdiff_in_bounds(*d_tv, now, 0, &diff);

				// end
				*count = *count + 1;
				return (0);
			}
		}
	}

	return (-1);
}


int nu_pinger(const char *address, int count, int pr_wait, int pr_interval)
{
	int                  ping_sock  = 0;
	int                  wait       = 1000;
	int                  interval   = 1000;
	int                  i          = 0;
	int                  recv_count = 0;
	int                  ret        = 0;
	struct sockaddr_in   dst;
	struct timespec      tv, tsv;


	if( address == NULL )   return -1;

	if( pr_wait > 0 )     wait     = pr_wait * 1000;
	if( pr_interval > 0)  interval = pr_interval * 1000;

	if( 0 > (ping_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)))
	{       
		return 2;
	}

	dst.sin_family       = AF_INET;
	dst.sin_port         = htons(0);
	dst.sin_addr.s_addr  = inet_addr(address);

	tsv.tv_sec   = interval / 1000;
	tsv.tv_nsec  = (interval % 1000) * 100000;
	setsockopt(ping_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tsv, sizeof(tsv));
	setsockopt(ping_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tsv, sizeof(tsv));
	for( i = 0; (i < count ) && (recv_count == 0 ); i++ )
	{
		clock_gettime(CLOCK_REALTIME, &tv);
		ret = send_icmp(ping_sock, dst.sin_addr.s_addr, getpid(), getpid()+i, (char *)&tv, sizeof(struct timeval));
		if( ret == -1 )
		{
			printf("send ping failed!\n");
			continue;
		}

		ret = recv_icmp(ping_sock, dst.sin_addr.s_addr, getpid(), getpid()+i, &recv_count, wait);
		if( ret == -1 )
		{
			printf("recv ping failed!\n");
			continue;
		}
	}

	if( ret == -1 )printf("host ping test fail\n");

	return ret;
}

int hb_send(server_node *con_arr, int con_len)
{
	struct sockaddr_in server_addr;

	char* data = NULL;
	char* tmp = NULL;
	int ret = 0;
	int p_socket = -1;
	int ip_length[con_len];
	unsigned int len = sizeof(struct sockaddr_in);
	if(0> (p_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	{
		printf("socket failed\n");
		return (-1);
	}
	data = (char *)malloc((sizeof(char))*BUF_SIZE);
	memset(data,'\0',BUF_SIZE);
	if(data!=NULL)
	{
		bzero(&server_addr, sizeof(server_addr));
		for(int node_num =0;node_num<con_len;node_num++)
		{
			if(strcmp(con_arr[node_num].ip, my_node->ip)==0)
			{
				continue;
			}
			ip_length[node_num]= strlen(con_arr[node_num].ip);
			server_addr.sin_port = htons(atoi(con_arr[node_num].hb_port));
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = inet_addr(con_arr[node_num].ip);
			strcpy(data, my_node->ip);
	//		data[ip_length[node_num]] = '\0';
			ret = sendto(p_socket, data , strlen(data), MSG_DONTWAIT,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
//			printf("send : %s, return %d \n",data,ret);
			if(ret == -1)
			{
				printf("HB_SEND error\n %d,%s\n",ret,strerror(errno));	
			}
		}
	}
	close(p_socket);
	return (ret);

}

int hb_receive(clock_t hb_c,server_node *con_arr, int con_len)
{
	int  sender_size = sizeof(sender_addr);
	int disconnect_flag=DISCONNECT;
	int array_length;
	char hb_state_config_cmd[1024];
	sprintf(hb_state_config_cmd,"%s/hb_state",config_cmd);
	connect_ip = json_object_new_object();
	node = json_object_from_file(hb_state_config_cmd);
	recv_data = (char *)malloc((sizeof(char))*BUF_SIZE);
	memset(recv_data,'\0',BUF_SIZE);
	recv_ret = recvfrom(recv_socket, recv_data , IP_SIZE,0, (struct sockaddr *)&sender_addr, &sender_size);
	//printf("recv data : %s\n", recv_data);
	if(recv_ret < 0)
	{
	//	printf("error %s ,my ip : %s\n",strerror(errno),my_node->ip);
	}
	int node_num;
	for(node_num=0; node_num<con_len;node_num++) 
	{
		if(strcmp(con_arr[node_num].ip,my_node->ip)==0)
		{
			continue;
		}
		if(disconnect_flag == CONNECT)
		{
			disconnect_flag =DISCONNECT;
			continue;
		} 
		if(strcmp(con_arr[node_num].ip,recv_data)==0)
		{
			con_arr[node_num].hb_time = clock();
		}
	}
	//if my network is down ,process some
	
	for(int node_num=0; node_num<con_len;node_num++)
	{
		if(strcmp(con_arr[node_num].ip,my_node->ip)==0)
		{
			continue;
		}
		else
		{
			double diff = abs(((double)(hb_c-con_arr[node_num].hb_time))/1000.0);
	//		new = json_object_array_get_idx(array, con_len);
			if(diff>((bounds)/100))
			{
				if(0 > nu_pinger(my_node->gateway,10,0,0))
				{
					disconnect_flag= CONNECT;	
					break;
				}

				con_arr[node_num].disconnect=DISCONNECT;
				json_object_object_get_ex(node,con_arr[node_num].ip,&connect_ip);
				if(!strcmp(json_object_get_string(connect_ip),"connect"))
				{
					json_object_object_del(node,con_arr[node_num].ip);	
					json_object_object_add(node,con_arr[node_num].ip,json_object_new_string("disconnect"));
					char buff[128];
					sprintf(buff,"/dev/%s",DMBD_IOCTL_NAME);
					ioctl_fd=open (buff,O_RDWR);
					if(strcmp(con_arr[node_num].teng_ip,"NULL")==0)
					{
						ioctl(ioctl_fd,DMBD_IO_CTL_HB_DOWN,con_arr[node_num].ip);
					}
					else
					{
						ioctl(ioctl_fd,DMBD_IO_CTL_HB_DOWN,con_arr[node_num].teng_ip);
					}
					close(ioctl_fd);
					hb_state = fopen(hb_state_config_cmd,"w");
					fprintf(hb_state,json_object_to_json_string(node));
					fclose(hb_state);
				}
			}
			else
			{
				if(con_arr[node_num].disconnect == DISCONNECT)
				{
					con_arr[node_num].disconnect = CONNECT;
					json_object_object_get_ex(node,con_arr[node_num].ip,&connect_ip);
					if(!strcmp(json_object_get_string(connect_ip),"disconnect"))
					{
					json_object_object_del(node,con_arr[node_num].ip);
					json_object_object_add(node,con_arr[node_num].ip,json_object_new_string("connect"));
						hb_state = fopen(hb_state_config_cmd,"w");
						fprintf(hb_state,json_object_to_json_string(node));
						fclose(hb_state);
					}
				}
			}
		}
	}
	free(recv_data);
//	recv_data = NULL;
	return 1;
}
int hb_recv_init(void)
{	
	struct timeval  tv_timeo = { 0, 50000 };
	int rtv;
	if(0> (recv_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	{
		printf("socket failed\n");
	}
	setsockopt(recv_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv_timeo, sizeof(struct timeval) );
	memset(&recv_addr,0,sizeof(recv_addr));
	recv_addr.sin_port = htons(atoi(my_node->hb_port)); //controller init 에서 가져옴
	recv_addr.sin_family =AF_INET;

	recv_addr.sin_addr.s_addr = inet_addr(my_node->ip);//controller init에서 gateway도가져옴
	if( 0 >(rtv = bind(recv_socket,(struct sockaddr *)&recv_addr,sizeof(recv_addr))))
	{
		printf("bind error %s\n",strerror(errno));
	}

	return rtv;
}


int create_sender(serv_n *server_node)
{
	pthread_attr_t	attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int rtv = pthread_create(&hb_sender_tid, &attr, &hb_sender, (void *)serv_node);
	if(0> rtv)
	{
		
		pthread_attr_destroy(&attr);
		return (-1);
	}
	return rtv;
}
int create_receiver(serv_n *serv_node)
{

	pthread_attr_t	attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(0> hb_recv_init())
	{
		printf("recv init error  : %s \n",strerror(errno));
	}
	int rtv= pthread_create(&hb_receiver_tid, &attr, &hb_receiver, (void *)serv_node);
	if(0> rtv)
	{
		
		pthread_attr_destroy(&attr);
		return (-1);
	}
	return rtv;
}
int hb_start(void)
{
	int return_attr = 0;
	int con_len = serv_node->len;
	char hb_state_config_cmd[1024];
	connect_ip = json_object_new_object();
	sprintf(hb_state_config_cmd,"%s/hb_state",config_cmd);
	for(int node_num=0; node_num<con_len;node_num++)
	{
		/*	if(!strcmp(serv_node->serv_con[node_num].ip,my_node->ip))
			{
				continue;
			}*/
			hb_state = fopen(hb_state_config_cmd,"w");
			json_object_object_add(connect_ip,serv_node->serv_con[node_num].ip,json_object_new_string("connect"));
			fprintf(hb_state,json_object_to_json_string(connect_ip));
			fclose(hb_state);

	}


	hb_alive = TRUE;
	return 1;

}

int hb_exit(void){

	if(hb_alive == TRUE)
		hb_alive = FALSE;
}


void *hb_sender(void *serv_node)
{
	serv_n *my_serv_node = (serv_n *)serv_node;
	while(hb_alive == TRUE)
	{
		
		hb_send(my_serv_node->serv_con,my_serv_node->len);
		sleep(1);
	}
}
void *hb_receiver(void *serv_node)
{
	serv_n *my_serv_node = (serv_n *)serv_node;
	while(hb_alive == TRUE)
	{
		clock_t hb_clock = clock();
		hb_receive(hb_clock,my_serv_node->serv_con,my_serv_node->len);
	}

}
