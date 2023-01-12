#include "app.h"
#include "msg_queue_mod.h"


enum process_cmd {
    HUP_END_PROGRAM_RUN = 0x00,     //结束程序
    HUP_UART_PRINT     = 0x01,      //串口打印
    HIP_USER_LOGIN     = 0x02,      //用户登录
    HIP_KEEP_ALIVE     = 0x03,      //心跳保持
    HIP_UART_PASS_THROUGH = 0x07,   //串口透传
    CLIENT_DEV         = 1,
    SERVER_DEV         = 2,
};

static int app_server_cmd_process(int cmd, app_mod_t *app_ptr)
{
     switch (app_ptr->msg_queue_ptr.cmd) {
        case HUP_END_PROGRAM_RUN:
            app_ptr->main_run_flag = false;
            break;
        case HUP_UART_PRINT:
            printf("app cmd uart printf ok\n");
            uart_mod_send(&app_ptr->uart_mod_ptr, app_ptr->msg_queue_ptr.data, app_ptr->msg_queue_ptr.length, HUP_UART_PRINT);
            break;
        case HIP_UART_PASS_THROUGH:
            //将tcp发过来的数据放进来，串口打印
            uart_mod_send(&app_ptr->uart_mod_ptr, app_ptr->msg_queue_ptr.data, app_ptr->msg_queue_ptr.length, HUP_UART_PRINT);
            break;
        default:
            break;
    }
    return 0;
}

static int app_client_cmd_process(int cmd, app_mod_t *app_ptr)
{
    if (NULL == app_ptr) {
        printf("[error] app client cmd process failed\n");
        return -1;
    }
     switch (app_ptr->msg_queue_ptr.cmd) {
        case HUP_END_PROGRAM_RUN:
            app_ptr->main_run_flag = false;
            break;
        case HUP_UART_PRINT:
            printf("[ok] app cmd uart printf\n");
            uart_mod_send(&app_ptr->uart_mod_ptr, app_ptr->msg_queue_ptr.data, app_ptr->msg_queue_ptr.length, HUP_UART_PRINT);
            break;
        case HIP_UART_PASS_THROUGH:
            //将串口发过来的数据放进来，tcp发包（先hip打包再发送)
            tcp_mod_send(&app_ptr->tcp_cs_ptr, app_ptr->msg_queue_ptr.data, app_ptr->msg_queue_ptr.length, HIP_UART_PASS_THROUGH);
            break;
        default:
            break;
    }
    return 0;
}

static int app_cmd_process(app_mod_t *app_ptr)
{
    if (NULL == app_ptr) {
        printf("app_cmd_process failed!\n");
        return -1;
    }
    printf("[start] deal queue msg\n");
    printf("[cmd:%x] msg queue cmd\n", app_ptr->msg_queue_ptr.cmd);
    switch (app_ptr->cmd_dev) {
        case CLIENT_DEV:
            app_client_cmd_process(app_ptr->msg_queue_ptr.cmd, app_ptr);
            break;
        case SERVER_DEV:
            app_server_cmd_process(app_ptr->msg_queue_ptr.cmd, app_ptr);
            break;
        default:
            break;
    }
    printf("[ok] deal queue msg\n");
    return 0;
}

static void *app_pthread_handle(void *arg)
{
    if (NULL == arg) {
        printf("app_pthread_handle failed!\n");
        return NULL;
    }
    app_mod_t *app_ptr = (app_mod_t *)arg;
    while (app_ptr->app_pthread.run_flag) {
        msg_queue_recv(app_ptr->msg_queue_fd, &app_ptr->msg_queue_ptr);
        printf("[ok] app recv queue msg\n");
        app_cmd_process(app_ptr);
    }
    return 0;
}

int app_start_handle(app_mod_t *app_ptr)
{
    if (NULL == app_ptr) {
        printf("app_start_pth failed!\n");
        return -1;
    }
    app_ptr->main_run_flag = true;
    app_ptr->app_pthread.run_flag = true;
    pthread_create(&app_ptr->app_pthread.tid, NULL, app_pthread_handle, (void *)app_ptr);
    pthread_detach(app_ptr->app_pthread.tid);
    return 0;
}

int app_stop_handle(app_mod_t *app_ptr)
{
    if (NULL == app_ptr) {
        printf("app_stop_pth failed!\n");
        return -1;
    }
    app_ptr->app_pthread.run_flag = false;
    return 0;
}
