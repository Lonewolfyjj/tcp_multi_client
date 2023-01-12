#include "uart_mod.h"
#include <sys/select.h> 
#include "msg_queue_mod.h"


#define UART_FIFO_SIZE       256


static fifo_buf_t uart_recv_fifo;
static fifo_buf_t uart_send_fifo;
static pack_msg_t pack_msg;




/******************************************************************************************
 * Function Name :  check_uart_isread
 * Description   :  监听串口是否可读
 * Parameters    :  fd：串口的文件描述符
 *                  sec:阻塞监听秒数
 *                  usec:阻塞监听微秒数
 * Returns       :  无
*******************************************************************************************/
static int check_uart_isread(int fd, int sec, int usec)
{
    int ret = 0;
    struct timeval timeout;
    fd_set read_fd;
    FD_ZERO(&read_fd);
    FD_SET(fd, &read_fd);
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    ret = select(sizeof(read_fd)+1, &read_fd, NULL, NULL, &timeout);
    return ret;
}


static void print_data(uint8_t *temp_buf, int len)
{
    int count;
    dbg_printf("text num:%d \r\n", len); 
    for (count = 0; count < len; count++) {
            dbg_printf("%x ", temp_buf[count]);
    }
    dbg_printf("\n");
}

/******************************************************************************************
 * Function Name :  uart_read_pthread
 * Description   :  读取串口发送过来的数据，并写入fifo中
 * Parameters    :  arg：创建线程时的传进来的参数
 * Returns       :  无
*******************************************************************************************/
static void *uart_read_pthread(void *arg)
{
    int ret, count;
    uint8_t temp_buf[256] = {0}; 
    uart_mod_t *uart_ptr = (uart_mod_t *)arg;
    
    while (uart_ptr->read_pth.run_flag) {
        ret = check_uart_isread(uart_ptr->uart_fd, 2, 0);
        if (ret < 0) {
            dbg_printf("uart_read_pthread read failed\r\n");
            return NULL;
        } else if (ret == 0) {
            continue;
        } else {
            dbg_printf("[uart start read]\r\n");
        }
        memset(temp_buf, 0, sizeof(temp_buf));
        ret = uart_read(uart_ptr->uart_fd, temp_buf, sizeof(temp_buf));
        if (ret < 0) {
            dbg_perror("read failed!\r\n");
            continue;
        }
        dbg_printf("[uart_read text:%s, num:%d]\n", temp_buf, ret);
        print_data(temp_buf, ret);
        printf("[start fifo write]\n");
        ret = fifo_write(uart_ptr->recv_fifo, temp_buf, ret);
        dbg_printf("[end fifo write num:%d]\r\n", ret);
    }
}

/******************************************************************************************
 * Function Name :  uart_handle_pthread
 * Description   :  判断recv_fifo中是否有数据,有就从里面全部读出来,进行hup解析,
 *                  解析的包有效就行进行命令处理，发送给app层
 * Parameters    :  arg：创建线程时的传进来的参数，这里传进来一个uart_ptr_p结构体
 * Returns       :  无
*******************************************************************************************/
static void *uart_handle_pthread(void *arg)
{
    int ret, count;
    uint8_t temp_buf[256] = {0};  
    uart_mod_t *uart_ptr = (uart_mod_t *)arg;

    while (uart_ptr->hand_pth.run_flag) {
        if (fifo_get_readable_num(uart_ptr->recv_fifo) > 0) {
            memset(temp_buf, 0, sizeof(temp_buf));
            ret = fifo_read(uart_ptr->recv_fifo, temp_buf, fifo_get_readable_num(uart_ptr->recv_fifo));

            dbg_printf("[start uart read fifo data]\n");
            print_data(temp_buf, ret);
            
            printf("[%ld, start] hup decode\n", time(NULL));
            for (count = 0; count < ret; count++) {
                hup_unpack(temp_buf[count], &pack_msg);
                if (pack_msg.status == true) {
                    pack_msg.status = false;
                    dbg_printf("[ok] hup decode\n"); 
                    msg_queue_send(uart_ptr->msg_queue_fd, pack_msg.cmd_id, pack_msg.data_buf, pack_msg.length);
                }
            }
            printf("[end] hup decode\n");        
        } else {
            usleep(1000);
        }
    }
}

/******************************************************************************************
 * Function Name :  uart_send_pthread
 * Description   :  判断send_fifo中是否有数据，有数据就将其发送给PC机
 * Parameters    :  arg：创建线程时的传进来的参数，这里传进来一个uart_ptr_p结构体
 * Returns       :  无
*******************************************************************************************/
static void *uart_send_pthread(void *arg)
{
    int ret, count;
    uint8_t temp_buf[256] = {0};  
    uart_mod_t *uart_ptr = (uart_mod_t *)arg;
    while (uart_ptr->send_pth.run_flag) {
        if (fifo_get_readable_num(uart_ptr->send_fifo) > 0) {
            memset(temp_buf, 0, sizeof(temp_buf));      
            ret = fifo_read(uart_ptr->send_fifo, temp_buf, fifo_get_readable_num(uart_ptr->send_fifo));
            dbg_printf("send_pthread--read--%d--\n", ret); 
            print_data(temp_buf, ret);
            uart_write(uart_ptr->uart_fd, temp_buf, ret);
        } else {
            usleep(1000);
        }

    }    
}


int uart_mod_start(uart_mod_t *uart_ptr)
{
    if (NULL == uart_ptr) {
        dbg_perror("uart_start failed!\r\n");
        return -1;
    }
    uart_ptr->read_pth.run_flag = true;
    uart_ptr->hand_pth.run_flag = true;
    uart_ptr->send_pth.run_flag = true;

    pthread_create(&uart_ptr->read_pth.tid, NULL, uart_read_pthread, (void *)uart_ptr);
    pthread_detach(uart_ptr->read_pth.tid);
    pthread_create(&uart_ptr->hand_pth.tid, NULL, uart_handle_pthread, (void *)uart_ptr);
    pthread_detach(uart_ptr->hand_pth.tid);
    pthread_create(&uart_ptr->hand_pth.tid, NULL, uart_send_pthread, (void *)uart_ptr);
    pthread_detach(uart_ptr->hand_pth.tid);
    printf("uart mod start ok\n");
    return 0;
}

int uart_mod_stop(uart_mod_t *uart_ptr)
{
    if (NULL == uart_ptr) {
        dbg_perror("uart_stop failed!\r\n");
        return -1;
    }
    uart_ptr->read_pth.run_flag = false;
    uart_ptr->hand_pth.run_flag = false;
    uart_ptr->send_pth.run_flag = false;
    return 0;
}



/******************************************************************************************
 * Function Name :  uart_mod_init
 * Description   :  串口应用初始化，初始化FIFO，打开串口，配置波特率，初始化uart_ptr结构体等
 * Parameters    :  uart_ptr：串口参数管理结构体
                    q_msg_fd：消息队列句柄
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int uart_mod_init(uart_mod_t *uart_ptr, int q_msg_fd)
{
    int ret = 0;
    uart_drv_init(&uart_ptr->uart_fd, UART_BAUD, UART_DATA_BIT);
    pthread_mutex_init(&uart_ptr->mutex, NULL);
    ret = fifo_init(&uart_recv_fifo, UART_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("uart_app_init failed 1 !\r\n");
        return -1;
    }
    ret = fifo_init(&uart_send_fifo, UART_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("uart_app_init failed 2 !\r\n");
        return -1;
    }
    pack_msg.status = false;
    uart_ptr->recv_fifo = &uart_recv_fifo;
    uart_ptr->send_fifo = &uart_send_fifo;
    uart_ptr->msg_queue_fd = q_msg_fd;
    printf("uart mod init ok, uart_ptr->msg_queue_fd:%d, uart_fd:%d\n", q_msg_fd, uart_ptr->uart_fd);
    write(uart_ptr->uart_fd, "i come you", 10);
    return 0;
}

/******************************************************************************************
 * Function Name :  uart_mod_deinit
 * Description   :  串口去初始化，关闭串口文件，释放FIFO内存，关闭定时器,关闭三个线程
 * Parameters    :  uart_ptr：串口参数管理结构体
 * Returns       :  无
*******************************************************************************************/
int uart_mod_deinit(uart_mod_t *uart_ptr)
{
    if (NULL == uart_ptr) {
        dbg_perror("uart_exit failed!\r\n");
        return -1;
    }
    uart_mod_stop(uart_ptr);
    uart_drv_uninit(uart_ptr->uart_fd);
    fifo_uninit(uart_ptr->recv_fifo);
    fifo_uninit(uart_ptr->send_fifo);   
    return 0;
}

int uart_mod_send(uart_mod_t *uart_ptr, char *data, int data_size, uint8_t cmd)
{
    int ret = 0;
    uint8_t send_buff[256];
    if (NULL == uart_ptr) {
        dbg_perror("uart_exit failed!\r\n");
        return -1;
    }
    ret = hup_pack(data, data_size, cmd, send_buff);
    printf("hup pack end\n");
    print_data(send_buff, ret);
    while (fifo_get_writeable_num(uart_ptr->send_fifo) < ret) {//等待，可写数量大于待写数量
        
    }
    fifo_write(uart_ptr->send_fifo, send_buff, ret);
    
    return 0;
}
