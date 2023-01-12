#include "common.h"
#include "msg_queue_mod.h"
#include "app.h"
#include "tcp_mod.h"


int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("argc param num is 4, format:./xx int ip prot!\n");
        return -1;
    } else if (atoi(argv[1]) != 1 && atoi(argv[1]) != 2) {
        printf("[argv[1] val error] client:1, server:2!\n");
        return -1;
    } else {
        printf("[inter program]\n");
    }
    app_mod_t app_mod;
    app_mod.cmd_dev = atoi(argv[1]);
    
    msg_queue_init(&app_mod.msg_queue_fd);    
    uart_mod_init(&app_mod.uart_mod_ptr, app_mod.msg_queue_fd);
    tcp_mod_init(argv[1], &app_mod.tcp_cs_ptr, argv[2], atoi(argv[3]), app_mod.msg_queue_fd);
    uart_mod_start(&app_mod.uart_mod_ptr);      
    tcp_mod_start(&app_mod.tcp_cs_ptr);   
    app_start_handle(&app_mod);
    while (app_mod.main_run_flag) {
        usleep(1000);
    }  
    dbg_printf("program exit\n");
    app_stop_handle(&app_mod);
    tcp_mod_deinit(&app_mod.tcp_cs_ptr);
    uart_mod_deinit(&app_mod.uart_mod_ptr);
    msg_queue_deinit(app_mod.msg_queue_fd);
    return 0;
}






