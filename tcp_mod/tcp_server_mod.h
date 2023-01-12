#ifndef __TCP_SERVER_MOD_H__
#define __TCP_SERVER_MOD_H__

#include "common.h"
#include "fifo.h"
#include "hip_mod.h"

#define CONNECT_CLIENT      1
#define UNCONNECT_CLIENT    0
#define CLIENT_MAX_NUM      5
#define SERVER_DEV_IPLEN    32
#define SERVER_DEV_MACLEN   16
#define CLIENT_KEEP_ALIVE_TIME   20

typedef struct tcp_server_info {
    int port_no;
    int port;
    int tcp_socketfd;
    int listen_num;
	char local_mac[SERVER_DEV_MACLEN];
	char local_ip[SERVER_DEV_IPLEN];	
}server_info_t;


typedef struct server_status_manage {
    int pro_no;
    int port;
    uint8_t client_socketfd;
    char ip_addr[SERVER_DEV_IPLEN];
	int send_time;
	int connect_client_status;
	int get_log_status;
}ser_cli_connect_status_t;
        
    
typedef struct server_param {
    int client_conectnum;
    int msg_queue_fd;
    fd_set read_fd_group;
    pth_stru_t listen_pth;
    pth_stru_t recv_pth;
    pth_stru_t recv_handle_pth;
    pth_stru_t check_alive_pth;
    pthread_mutex_t mutex;
    fifo_buf_t *recv_fifo;
    fifo_buf_t *send_fifo;
    server_info_t server_info;
    int fd_group[CLIENT_MAX_NUM];
    ser_cli_connect_status_t client_info[CLIENT_MAX_NUM];
}server_mod_t;


int tcp_server_mod_init(server_mod_t *tcp_ptr, char *ip_addr, int server_port, int msg_fd);
void tcp_server_mod_deinit(server_mod_t *tcp_ptr);
int tcp_server_mod_start(server_mod_t *tcp_ptr);
int tcp_server_mod_stop(server_mod_t *tcp_ptr);
int tcp_server_mod_send(int sockfd, uint8_t *data, int data_size, uint8_t cmd);

#endif