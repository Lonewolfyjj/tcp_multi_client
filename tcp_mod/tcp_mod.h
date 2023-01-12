#ifndef __TCP_MOD_H__
#define __TCP_MOD_H__


#include "tcp_client_mod.h"
#include "tcp_server_mod.h"

typedef struct tcp_client_server_struct {
    client_mod_t client_mod_ptr;
    server_mod_t server_mod_ptr;
}client_server_t;


int tcp_mod_init(char *cs_type, void *tcp_ptr, char *ip_addr, int port, int msg_fd);
void tcp_mod_deinit(void *tcp_ptr);
int tcp_mod_start(void *tcp_ptr);
int tcp_mod_stop(void *tcp_ptr);
int tcp_mod_send(void *tcp_ptr, char *data, int data_size, uint8_t cmd);

#endif
