#ifndef  __DMBD__HB__
#define  __DMBD__HB__
#include "json_file.h"
#define BUF_SIZE 1024
#define IP_SIZE 16
#define NW_COUNT 2
//#define TRUE 1
//#define FALSE 0
typedef struct{
     server_node *serv_con;
     int len;
}serv_n;
int hb_start(void);

int hb_exit(void); 
int hb_recv_init(void);
int create_sender(serv_n *server_node);
int create_receiver(serv_n *server_node);
void *hb_receiver(void *serv_node); 
void *hb_sender(void *serv_node); 
#endif


