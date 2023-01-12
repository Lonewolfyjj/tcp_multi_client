#ifndef __TCP_CLIENT_MOD_H__
#define __TCP_CLIENT_MOD_H__

#include "common.h"
#include "fifo.h"
#include "hip_mod.h"
#include "msg_queue_mod.h"


#define     CLIENT_DEV_MACLEN           16
#define     CLIENT_DEV_IPLEN            32
#define     CONNECT_SERVER              1
#define     UNCONNECT_SERVER            0


#define     SEND_KEEP_ALIVE_TIME        3
#define     KEEP_ALIVE_TIMEOUT_TIME     20

typedef struct tcp_client_info {
	int tcp_socketfd;
	int server_port;
    char dev_type;
	char local_mac[CLIENT_DEV_MACLEN];
	char local_ip[CLIENT_DEV_IPLEN];
	char server_ip[CLIENT_DEV_IPLEN];	
}client_info_t;


typedef struct client_status_manage {
	unsigned int send_time;
	unsigned int recv_time;
	int g_connect_master_status;
	int send_log_num;
}client_status_t;

typedef struct tcp_client_struct {
    int msg_queue_fd;
    pth_stru_t send_pth;
    pth_stru_t recv_pth;
    pth_stru_t recv_handle_pth;
    pth_stru_t connect_pth;
    pth_stru_t keep_alive_pth;
    pthread_mutex_t mutex;
    fifo_buf_t *recv_fifo;
    fifo_buf_t *send_fifo;
    client_status_t g_client_status;
    client_info_t g_client_info;
}client_mod_t;


int tcp_client_mod_init(client_mod_t *tcp_ptr, char *server_addr, int server_port, int msg_fd);
void tcp_client_mod_deinit(client_mod_t *tcp_ptr);
int tcp_client_mod_start(client_mod_t *tcp_ptr);
int tcp_client_mod_stop(client_mod_t *tcp_ptr);
int tcp_client_mod_send(client_mod_t *tcp_ptr, uint8_t *data, uint16_t data_size, uint8_t cmd);


#endif