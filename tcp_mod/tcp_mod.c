#include "tcp_mod.h"


static char s_tcp_cs_type;

enum cs_type {
    CLIENT_TYPE = 1,
    SERVER_TYPE
};

int tcp_mod_init(char *cs_type, void *tcp_ptr, char *ip_addr, int port, int msg_fd)
{
    int ret = 0;
    client_server_t *tcp_mod_ptr = (client_server_t *)tcp_ptr;
    s_tcp_cs_type = atoi(cs_type);
    printf("c:1, s:2, c_s_type:%d\n", s_tcp_cs_type);
    switch (s_tcp_cs_type) {
        case CLIENT_TYPE:
            printf("tcp client mod init\n");
            ret = tcp_client_mod_init(&tcp_mod_ptr->client_mod_ptr, ip_addr, port, msg_fd);
            break;
        case SERVER_TYPE:
            printf("tcp server mod init\n");
            ret = tcp_server_mod_init(&tcp_mod_ptr->server_mod_ptr, ip_addr, port, msg_fd);
            break;
        default :
            printf("[error] tcp mod init\n");
            break;
    }
    printf("tcp mod init ok\n");
    return ret;
}


void tcp_mod_deinit(void *tcp_ptr)
{
    client_server_t *tcp_mod_ptr = (client_server_t *)tcp_ptr;
    switch (s_tcp_cs_type) {
        case CLIENT_TYPE:
            tcp_client_mod_deinit(&tcp_mod_ptr->client_mod_ptr);
            break;
        case SERVER_TYPE:
            tcp_server_mod_deinit(&tcp_mod_ptr->server_mod_ptr);
            break;
        default :
            printf("[error] tcp mod deinit\n");
            break;
    }
}
int tcp_mod_start(void *tcp_ptr)
{
    int ret = 0;
    client_server_t *tcp_mod_ptr = (client_server_t *)tcp_ptr;
    switch (s_tcp_cs_type) {
        case CLIENT_TYPE:
            ret = tcp_client_mod_start(&tcp_mod_ptr->client_mod_ptr);
            break;
        case SERVER_TYPE:
            ret = tcp_server_mod_start(&tcp_mod_ptr->server_mod_ptr);
            break;
        default :
            printf("[error] tcp mod start\n");
            break;
    }
    printf("tcp mod start ok\n");
    return ret;   
}

int tcp_mod_stop(void *tcp_ptr)
{
    int ret = 0;
    client_server_t *tcp_mod_ptr = (client_server_t *)tcp_ptr;
    switch (s_tcp_cs_type) {
        case CLIENT_TYPE:
            ret = tcp_client_mod_stop(&tcp_mod_ptr->client_mod_ptr);
            break;
        case SERVER_TYPE:
            ret = tcp_server_mod_stop(&tcp_mod_ptr->server_mod_ptr);
            break;
        default :
            printf("[error] tcp mod stop\n");
            break;
    }
    return ret;    
}
int tcp_mod_send(void *tcp_ptr, char *data, int data_size, uint8_t cmd)
{
    int ret = 0;
    client_server_t *tcp_mod_ptr = (client_server_t *)tcp_ptr;
    switch (s_tcp_cs_type) {
        case CLIENT_TYPE:
            ret = tcp_client_mod_send(&tcp_mod_ptr->client_mod_ptr, data, data_size, cmd);
            break;
        case SERVER_TYPE:
            ret = tcp_server_mod_send(*((int *)tcp_ptr), data, data_size, cmd);
            break;
        default :
            printf("[error] tcp mod send\n");
            break;
    }
    printf("tcp mod send ok");
    return ret;    
}