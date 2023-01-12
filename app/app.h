#ifndef __APP_H__
#define __APP_H__

#include "common.h"
#include "uart_mod.h"
#include "tcp_client_mod.h"
#include "tcp_server_mod.h"
#include "tcp_mod.h"
#include "msg_queue_mod.h"



typedef struct app_param_struct {
    int cmd_dev;
    pth_stru_t app_pthread;
    int msg_queue_fd;
    bool main_run_flag;
    uart_mod_t uart_mod_ptr;
    pack_msg_t pack_msg_ptr;
    client_server_t tcp_cs_ptr;
    msg_queue_mod_t msg_queue_ptr; 
}app_mod_t;


int app_start_handle(app_mod_t *app_ptr);
int app_stop_handle(app_mod_t *app_ptr);


#endif