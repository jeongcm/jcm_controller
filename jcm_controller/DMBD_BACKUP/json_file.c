#include <stdio.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include "json_file.h"

server_node* ReadServerConfig(char *filepath,int *len)
{
	int i=0,length;
	server_node* server_config;
	json_object *file,*array,*new,*ip,*hb_port,*c_port,*gateway,*teng;

	file = json_object_from_file(filepath);
	json_object_object_get_ex(file,"server",&array);
	length=json_object_array_length(array);
	server_config = (server_node*)malloc(sizeof(server_node)*length);

	for(i=0 ; i < length ; i++)
	{
	  new = json_object_array_get_idx(array, i);

	  json_object_object_get_ex(new, "ip",&ip);
	  server_config[i].ip = json_object_get_string(ip);

	  json_object_object_get_ex(new, "hb_port",&hb_port);
	  server_config[i].hb_port = json_object_get_string(hb_port);

	  json_object_object_get_ex(new, "controller_port",&c_port);
	  server_config[i].controller_port = json_object_get_string(c_port);

	  json_object_object_get_ex(new, "gateway",&gateway);
	  server_config[i].gateway = json_object_get_string(gateway);


	}
	*len = length;
	return server_config;
}

DMBD_CONFIG_REMOTE* ReadDMBDRemote(char *filepath,int *len)
{

	int i,remote_length;
	json_object *file,*array,*new,*ip,*dev_name,*dev_size, *nbd_export, *nbd_port;
	DMBD_CONFIG_REMOTE *remote_config;

	file = json_object_from_file(filepath);
	json_object_object_get_ex(file,"remote",&array);
	remote_length=json_object_array_length(array);
	remote_config = (DMBD_CONFIG_REMOTE*)calloc(remote_length,sizeof(DMBD_CONFIG_REMOTE));

	for(i=0 ; i < remote_length ; i++)
	{
		const char *r_ip;
		const char *r_dev_name;
		const char *r_nbd_export;
		const char *r_nbd_port;
		int64_t dev_size;

	  new = json_object_array_get_idx(array, i);
	  json_object_object_get_ex(new, "ip",&ip);
	  r_ip = json_object_get_string(ip);
	  memcpy(remote_config[i].ip,r_ip,16);

	  json_object_object_get_ex(new, "remote_dev",&dev_name);
	  r_dev_name = json_object_get_string(dev_name);
	  memcpy(remote_config[i].dev_name,r_dev_name,32);

	  json_object_object_get_ex(new, "dev_size",&dev_name);
	  dev_size = json_object_get_int64(dev_name);
	  remote_config[i].dev_size=dev_size;

	  json_object_object_get_ex(new, "nbd_export",&nbd_export);
	  r_nbd_export = json_object_get_string(nbd_export);
	  memcpy(remote_config[i].nbd_export,r_nbd_export,32);

	  json_object_object_get_ex(new,"dmbd_nbd_port",&nbd_port);
	  r_nbd_port = json_object_get_string(nbd_port);
	  memcpy(remote_config[i].nbd_port,r_nbd_port,16);

	}
	*len = remote_length;
	return remote_config;
}
DMBD_CONFIG_REPLICATION* ReadDMBDReplication(char *filepath,char *my_ip,int *len, int *info_len)
{
	int i,k,j,length,replica_length,replica_info_length;
	DMBD_CONFIG_REPLICATION *replica_config;
	json_object *file,*array,*new,*ip,*dev,*replica,*replica_attr,*my_attr, *replica_info_attr,*repli_ip, *repli_dev, *nbd_export, *nbd_port;

	file = json_object_from_file(filepath);
	json_object_object_get_ex(file,"replica",&array);
	length=json_object_array_length(array);
	for(i=0 ; i < length ; i++)
	{
		const char *local_ip;
	  new = json_object_array_get_idx(array, i);

	  json_object_object_get_ex(new, "local_ip",&ip);
	  local_ip = json_object_get_string(ip);
	  if(strcmp(my_ip,local_ip)==0)
	  {
			json_object_object_get_ex(new,"set_info",&replica_attr);
			replica_length=json_object_array_length(replica_attr);

			//replica_config = (DMBD_CONFIG_REPLICA*)malloc(sizeof(DMBD_CONFIG_REPLICA)
			//									*replica_length);
			replica_config = (DMBD_CONFIG_REPLICATION *)calloc(replica_length,sizeof(DMBD_CONFIG_REPLICATION));
			for (k = 0 ; k < replica_length; k++ )
			{
				const char *node_ip;
				replica = json_object_array_get_idx(replica_attr, k);

				json_object_object_get_ex(replica, "node_ip",&ip);
				node_ip = json_object_get_string(ip);
				memcpy(replica_config[k].ip,node_ip,16);
				json_object_object_get_ex(replica,"replica_info",&my_attr);
				replica_info_length = json_object_array_length(my_attr);
				replica_config[k].replica_info = (DMBD_INFORMATION *)calloc(replica_info_length,sizeof(DMBD_INFORMATION));									
				for(j = 0; j< replica_info_length; j++)
				{
					const char *replica_ip;
					const char *replica_dev;
					const char *r_nbd_export;
					const char *r_nbd_port;
					replica_info_attr =  json_object_array_get_idx(my_attr, j);

					json_object_object_get_ex(replica_info_attr,"replica_ip",&repli_ip);
					replica_ip = json_object_get_string(repli_ip);
					printf("replica : %s\n",replica_ip);
					strcpy(replica_config[k].replica_info[j].ip,replica_ip);

					printf("recplia_ip : %s\n",replica_config[k].replica_info[j].ip);

				
					json_object_object_get_ex(replica_info_attr,"dev_name",&repli_dev);
					replica_dev = json_object_get_string(repli_dev);	
					memcpy(replica_config[k].replica_info[j].dev,replica_dev,32);
					printf("recplia_dev : %s\n",replica_config[k].replica_info[j].dev);

				    json_object_object_get_ex(replica_info_attr, "nbd_export",&nbd_export);
  				    r_nbd_export = json_object_get_string(nbd_export);
				    memcpy(replica_config[k].replica_info[j].nbd_export,r_nbd_export,32);

		     	    json_object_object_get_ex(replica_info_attr, "dmbd_nbd_port",&nbd_port);
		 		    r_nbd_port = json_object_get_string(nbd_port);
				    memcpy(replica_config[k].replica_info[j].nbd_port,r_nbd_port,16);
				}
				
			}
			break;
		}
	}
	*len = replica_length;
	*info_len = replica_info_length;
	return replica_config;
}
void ReadNBDReplication(char *filepath,char *my_ip,DMBD_CONFIG_REPLICATION *my_replica,int replica_info_len,int replica_len)
{
	int i,k,j,length;
	json_object *file,*array,*new,*ip,*dev,*replica,*replica_attr,*my_attr, *replica_info_attr,*repli_ip, *repli_dev, *nbd_export, *nbd_port;
	file = json_object_from_file(filepath);
	json_object_object_get_ex(file,"replica",&array);
	length=json_object_array_length(array);


	for(i=0 ; i < length ; i++)
	{
		const char *local_ip;

		new = json_object_array_get_idx(array, i);
		json_object_object_get_ex(new, "local_ip",&ip);
		local_ip = json_object_get_string(ip);
		if(strcmp(local_ip,my_ip)!=0)
		{
			int set_info_len;
			for(int t = 0 ; t < replica_len ; t++)
			{

				json_object_object_get_ex(new,"set_info",&replica_attr);
				set_info_len = json_object_array_length(replica_attr);
				for (k = 0 ; k < set_info_len; k++ )
				{

					const char *node_ip;
					replica = json_object_array_get_idx(replica_attr, k);

					json_object_object_get_ex(replica, "node_ip",&ip);
					node_ip = json_object_get_string(ip);

					if(strcmp(my_replica[t].ip,node_ip)==0)
					{
						for(int m = 0 ; m < replica_info_len ; m++)
						{
							int tmp_replica_info_len;
							json_object_object_get_ex(replica,"replica_info",&my_attr);
							tmp_replica_info_len=json_object_array_length(my_attr);

							for(j = 0; j< tmp_replica_info_len; j++)
							{
								const char *replica_ip;
								const char *replica_dev;
								const char *r_nbd_export;
								const char *r_nbd_port;
								replica_info_attr =  json_object_array_get_idx(my_attr, j);

								json_object_object_get_ex(replica_info_attr,"replica_ip",&repli_ip);
								replica_ip = json_object_get_string(repli_ip);

								json_object_object_get_ex(replica_info_attr,"dev_name",&repli_dev);
								replica_dev = json_object_get_string(repli_dev);	
								if(strcmp(replica_ip,my_replica[t].replica_info[m].ip)==0)
								{
									if(strstr(replica_dev,"nbd")==NULL)
									{
										json_object_object_get_ex(replica_info_attr, "nbd_export",&nbd_export);
										r_nbd_export = json_object_get_string(nbd_export);
										memcpy(my_replica[t].replica_info[m].nbd_export,r_nbd_export,32);

										json_object_object_get_ex(replica_info_attr, "dmbd_nbd_port",&nbd_port);
										r_nbd_port = json_object_get_string(nbd_port);
										memcpy(my_replica[t].replica_info[m].nbd_port,r_nbd_port,16);

									}

								}

							}

						}

					}
				}
			}

		}

	}
}
