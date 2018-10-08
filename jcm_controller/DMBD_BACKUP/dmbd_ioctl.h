#include<linux/ioctl.h>
 #ifndef __DMBD_IOCTL__
 #define __DMBD_IOCTL__

#define DMBD_NAME "dmbd"
#define DMBD_IOCTL_NAME "dmbd_ioctl"
#define DISCONNECT 1
#define CONNECT    0
#define Ip_len 16
#define dev_len 32
typedef struct 
{
	char ip[16];
	char dev_name[32];
	unsigned long dev_size;
	char nbd_export[32];
	char nbd_port[16];
}DMBD_CONFIG_REMOTE;
typedef struct 
{
	char ip[16];
	char replica_ip[16];
	char replica_dev[32];
}DMBD_CONFIG_REPLICA;
typedef struct
{
	char ip[16];
	unsigned int state;
}DMBD_CONNECT_STATE;
typedef struct
{
	unsigned long write_mbps;
	unsigned long read_mbps;
}DMBD_MBPS;
/*typedef struct
{
     char ip[Ip_len];
     char dev[dev_len];
}DMBD_INFORMATION;
typedef struct
{
      char ip[Ip_len];
      DMBD_INFORMATION *replica_info;
}DMBD_CONFIG_REPLICATION;
*/
#define DMBD_IOCTL_MAGIC 'B'

#define DMBD_IO_CTL_HB_DOWN 				_IOW(DMBD_IOCTL_MAGIC,0,char*)
#define DMBD_IO_CTL_REMOTE_CONFIG 	_IOW(DMBD_IOCTL_MAGIC,1,DMBD_CONFIG_REMOTE*)
#define DMBD_IO_CTL_REPLICA_CONFIG 	_IOW(DMBD_IOCTL_MAGIC,2,DMBD_CONFIG_REPLICA*)
#define DMBD_IO_CTL_CONFIG_LOAD			_IO(DMBD_IOCTL_MAGIC,4)
#define DMBD_IO_CTL_CONNECT_STATE		_IOR(DMBD_IOCTL_MAGIC,5,DMBD_CONNECT_STATE*)
#define DMBD_IO_CTL_TOTAL_SIZE			_IOR(DMBD_IOCTL_MAGIC,6,unsigned long*)
#define DMBD_IO_CTL_EACH_SIZE				_IOR(DMBD_IOCTL_MAGIC,7,unsigned long*)
#define DMBD_IO_CTL_MBPS						_IOR(DMBD_IOCTL_MAGIC,8,unsigned long*)
#define DMBD_IO_CTL_BACKUP					_IOR(DMBD_IOCTL_MAGIC,9,unsigned int*)
#endif
