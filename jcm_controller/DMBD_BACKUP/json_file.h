#include "dmbd_ioctl.h"
#ifndef __json_file__
#define __json_file__
typedef struct
{
	const char *ip;
	const char *hb_port;
	const char *controller_port;
	const char *gateway;
	const char *teng_ip;
	clock_t hb_time;
	int disconnect;

}server_node;
typedef struct
{
	char ip[Ip_len];
	char dev[dev_len];
	 char nbd_export[32];
	 char nbd_port[16];
}DMBD_INFORMATION;  
typedef struct{ 
	char server_ip[16]; 
	char server_dev[32]; 
	char server_export[32]; 
	char server_port[16]; 
}NBD_SERVER_CONFIG; 
typedef struct
{
     char ip[Ip_len];
     DMBD_INFORMATION *replica_info;
}DMBD_CONFIG_REPLICATION;
typedef struct
{
	int replica_length;
	int replica_info_length;
	char local_ip[16];
	DMBD_CONFIG_REPLICATION *replica_config;
}DMBD_REPLICA_NODE;
extern DMBD_CONFIG_REMOTE *dmbd_config_remote;
extern int remote_dmbd_config_len;

void ReadNBDReplication(char *filepath,char* my_ip,DMBD_CONFIG_REPLICATION *my_replica,int replica_info_len,int replica_len);
server_node *ReadServerConfig(char *filepath,int *len);
DMBD_CONFIG_REMOTE* ReadDMBDRemote(char *filepath,int *len);
//DMBD_CONFIG_REPLICA* ReadDMBDReplica(char *filepath,char *my_ip,int *len, int *info_len);
DMBD_CONFIG_REPLICATION* ReadDMBDReplication(char *filepath,char *my_ip,int *len, int *info_len);
#endif
