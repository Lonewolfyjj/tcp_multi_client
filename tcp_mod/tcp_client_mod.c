#include "tcp_client_mod.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define TCP_FIFO_SIZE       256
#define TEMP_BUF_SIZE       256

static fifo_buf_t s_recv_fifo;
static fifo_buf_t s_send_fifo;

static int get_current_time(void)
{
    time_t times = time(NULL);
    return times;
}

static int check_client_sockfd_isread(int fd, int sec, int usec)
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


static int client_connect_server(char *server_ip_addr, int server_port)
{
    if (NULL == server_ip_addr || server_port < 1024) {
        dbg_perror("[error] tcp_client_mod_send failed!\r\n");
        return -1;
    }
    int ret = 0, sockfd = 0;
    struct sockaddr_in recv_addr;
    socklen_t addrlen = sizeof(recv_addr);
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(server_port);
    recv_addr.sin_addr.s_addr = inet_addr(server_ip_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[error] create socket error\r\n");
        exit(0);
    }
    ret = connect(sockfd, (struct sockaddr *)&recv_addr, addrlen);
    printf("[connect ret:%d] connect server\n", ret);
    if (ret < 0 ) {
        perror("[error] connect server failed");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

static void tcp_close_client_connect(client_mod_t *tcp_ptr)
{
    if (tcp_ptr->g_client_info.tcp_socketfd > 0) {
        close(tcp_ptr->g_client_info.tcp_socketfd);
        tcp_ptr->g_client_info.tcp_socketfd = 0;
    }
    tcp_ptr->g_client_status.g_connect_master_status = UNCONNECT_SERVER;
    printf("[close sockfd ok] sockfd close and unconnect\n");             
}

int tcp_client_mod_send(client_mod_t *tcp_ptr, uint8_t *data, uint16_t data_size, uint8_t cmd)
{
    int ret = 0;
    uint8_t msgbuf[TEMP_BUF_SIZE];
    //检测参数
    if (NULL == tcp_ptr) {
        dbg_perror("tcp_client_mod_send failed!\r\n");
        return -1;
    }
    //开始HIP打包
    ret = hip_mod_pack(data, data_size, cmd, msgbuf);
    fifo_write(tcp_ptr->send_fifo, msgbuf, ret);
    return 0;
}

static void get_send_data(fifo_buf_t *send_fifo, int data_len, uint8_t *data_out)
{
    int count;
    while (1) {
        if (fifo_get_readable_num(send_fifo) >= data_len) {
            fifo_read(send_fifo, data_out, data_len);
            break;
        } else {
            usleep(5);
        }
    }
}

static void *tcp_client_send_pthread(void *arg)
{
    int ret = 0, count, data_len = 0, read_num = 0;
    uint8_t temp_buf[TEMP_BUF_SIZE] = {0}; 
    uint8_t send_buf[TEMP_BUF_SIZE] = {0}; 
    client_mod_t *tcp_ptr = (client_mod_t *)arg;

    while (tcp_ptr->send_pth.run_flag) {
        read_num = fifo_get_readable_num(tcp_ptr->send_fifo);
        if (read_num >= BYTE_PACK_HEAD_SIZE) {
            memset(send_buf, 0, sizeof(send_buf)); 
            fifo_read(tcp_ptr->send_fifo, send_buf, BYTE_PACK_HEAD_SIZE);
            data_len = (send_buf[2] | send_buf[3]);
            get_send_data(tcp_ptr->send_fifo, data_len, temp_buf);
            memcpy(send_buf + BYTE_PACK_HEAD_SIZE, temp_buf, data_len);
            send(tcp_ptr->g_client_info.tcp_socketfd, send_buf, BYTE_PACK_HEAD_SIZE + data_len, 0);
            dbg_printf("[start send tcp data:%d]\n", BYTE_PACK_HEAD_SIZE + data_len);  
            for (count = 0; count < (BYTE_PACK_HEAD_SIZE + data_len); count++) {
                dbg_printf("%x ", send_buf[count]);
            }
            dbg_printf("\n");
        } else {
            usleep(1000);
        }
    }    
}

static void tcp_client_cmd_deal(uint8_t *hip_msg, client_mod_t *tcp_ptr)
{
    uint8_t commond = hip_msg[1];
    uint16_t data_len = ((hip_msg[2] << 8) | hip_msg[3]);
    switch (commond) {
        case HIP_USER_LOGIN_PACK:
            //和服务端已连接
            printf("[ok] login true\n");
            tcp_ptr->g_client_status.send_log_num = 0;
            tcp_ptr->g_client_status.g_connect_master_status = CONNECT_SERVER;
            tcp_ptr->g_client_status.recv_time = get_current_time();
            break;
        case HIP_KEEP_ALIVE_PACK:
            //更新recv_time
            printf("[ok] keep alive true\n");;
            tcp_ptr->g_client_status.recv_time = get_current_time();
            break;
        case HIP_UART_PASS_THROUGH_PACK:
            msg_queue_send(tcp_ptr->msg_queue_fd, commond, (hip_msg + BYTE_PACK_HEAD_SIZE), data_len);
            printf("[ok] send msg queue\n");
            break;
        default :
            printf("[error] tcp_client_cmd_deal\n");
            break;
    }
}

static void *tcp_client_recv_pthread(void *arg)
{
    int ret, count;
    uint8_t recvbuf[1024] = {0};  
    client_mod_t *tcp_ptr = (client_mod_t *)arg;

    while (tcp_ptr->recv_pth.run_flag) {
        if (tcp_ptr->g_client_info.tcp_socketfd <= 0) {
            continue;
        }
        ret = check_client_sockfd_isread(tcp_ptr->g_client_info.tcp_socketfd, 2, 0);
        if (ret < 0) {
            printf("[error] tcp_client_recv_pthread select failed\n");
            continue;
        } else if (ret == 0) {
            usleep(1000);
            continue;
        }
        ret = recv(tcp_ptr->g_client_info.tcp_socketfd, recvbuf, sizeof(recvbuf), 0);
        if (ret <= 0) {
            printf("[error, recv len:%d, sockfd:%d] client recv failed\n", ret, tcp_ptr->g_client_info.tcp_socketfd);
            tcp_close_client_connect(tcp_ptr);
            continue;
        }
        tcp_ptr->g_client_status.recv_time = get_current_time();
        printf("[recv data num:%d] ", ret);
        for (int i = 0; i < ret; i++) {
            printf("%x ", recvbuf[i]);
        }
        printf("\n");
        fifo_write(tcp_ptr->recv_fifo, recvbuf, ret);
    }    
}

static void *tcp_client_recv_handle_pthread(void *arg)
{
    int data_len = 0, read_num = 0;
    client_mod_t *tcp_ptr = (client_mod_t *)arg;
    hip_msg_pack_t msg_data[10];
    uint8_t temp_buf[TEMP_BUF_SIZE] = {0}; 
    uint8_t deal_buf[TEMP_BUF_SIZE] = {0}; 

    while (tcp_ptr->recv_handle_pth.run_flag) {
        read_num = fifo_get_readable_num(tcp_ptr->recv_fifo);
        if (read_num >= BYTE_PACK_HEAD_SIZE) {
            fifo_read(tcp_ptr->recv_fifo, temp_buf, BYTE_PACK_HEAD_SIZE);
            data_len = (temp_buf[2] | temp_buf[3]);
            memcpy(deal_buf, temp_buf, BYTE_PACK_HEAD_SIZE);
            while (1) {
                if (fifo_get_readable_num(tcp_ptr->recv_fifo) > temp_buf[2]) {
                    fifo_read(tcp_ptr->recv_fifo, (temp_buf + BYTE_PACK_HEAD_SIZE), data_len);
                    break;
                }
            }
            memcpy(deal_buf + BYTE_PACK_HEAD_SIZE, temp_buf, data_len);
            tcp_client_cmd_deal(deal_buf, tcp_ptr);
            printf("[ok] client cmd deal\n");
        }       
    }
}

static void *tcp_client_reconnect_pthread(void *arg)
{
    int ret, count;
    uint8_t msgbuf[TEMP_BUF_SIZE] = {0};  
    client_mod_t *tcp_ptr = (client_mod_t *)arg;
    tcp_ptr->g_client_status.send_log_num = 0;
    while (tcp_ptr->connect_pth.run_flag) {
        if (tcp_ptr->g_client_info.tcp_socketfd <= 0) {
            tcp_ptr->g_client_info.tcp_socketfd = client_connect_server(tcp_ptr->g_client_info.server_ip, tcp_ptr->g_client_info.server_port);
            printf("[end connect, sockfd:%d] connect server\n", tcp_ptr->g_client_info.tcp_socketfd);
        }
        if (tcp_ptr->g_client_status.g_connect_master_status == UNCONNECT_SERVER && tcp_ptr->g_client_info.tcp_socketfd > 0) {
            tcp_client_mod_send(tcp_ptr, " ", 1, HIP_USER_LOGIN_PACK);
            printf("[%d, ok] send login pack\n", get_current_time());
            tcp_ptr->g_client_status.send_time = get_current_time();
            tcp_ptr->g_client_status.send_log_num++;
            if (tcp_ptr->g_client_status.send_log_num >= 5) {
                printf("[login over time]\n");
                tcp_ptr->g_client_status.send_log_num = 0;
                tcp_close_client_connect(tcp_ptr);
            }
        }
        sleep(2);
    }    
}

static void *tcp_client_keep_alive_pthread(void *arg)
{
    int ret, count, cur_time;
    uint8_t msgbuf[TEMP_BUF_SIZE] = {0};  
    client_mod_t *tcp_ptr = (client_mod_t *)arg;
    while (tcp_ptr->keep_alive_pth.run_flag) {
        if (tcp_ptr->g_client_status.g_connect_master_status == CONNECT_SERVER) {
            cur_time = time(NULL);
            if (abs(cur_time - tcp_ptr->g_client_status.send_time) >= SEND_KEEP_ALIVE_TIME) {
                //发送心跳消息
                tcp_client_mod_send(tcp_ptr, " ", 0, HIP_KEEP_ALIVE_PACK);
                tcp_ptr->g_client_status.send_time = get_current_time();
                printf("[ok] send keep alive\n");
            }
            if (abs(cur_time - tcp_ptr->g_client_status.recv_time) >= KEEP_ALIVE_TIMEOUT_TIME) {
                //检测到已经断线
                printf("[UNCONNECT_SERVER]\n");
                tcp_close_client_connect(tcp_ptr);
            }
        } else {
            usleep(1000);
        }
    }    
}

int tcp_client_mod_start(client_mod_t * tcp_ptr)
{
	if (NULL == tcp_ptr) {
        dbg_perror("uart_start failed!\r\n");
        return -1;
    }
	tcp_ptr->send_pth.run_flag = true;
	tcp_ptr->recv_pth.run_flag = true;
    tcp_ptr->recv_handle_pth.run_flag = true;
    tcp_ptr->connect_pth.run_flag = true;
    tcp_ptr->keep_alive_pth.run_flag = true;
    pthread_create(&tcp_ptr->send_pth.tid, NULL, tcp_client_send_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->send_pth.tid);
	pthread_create(&tcp_ptr->recv_pth.tid, NULL, tcp_client_recv_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->recv_pth.tid);
    pthread_create(&tcp_ptr->recv_handle_pth.tid, NULL, tcp_client_recv_handle_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->recv_handle_pth.tid);
    pthread_create(&tcp_ptr->connect_pth.tid, NULL, tcp_client_reconnect_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->connect_pth.tid);
	pthread_create(&tcp_ptr->keep_alive_pth.tid, NULL, tcp_client_keep_alive_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->keep_alive_pth.tid);
    printf("tcp client mod init ok\n");
	return 0;
}

int tcp_client_mod_stop(client_mod_t *tcp_ptr)
{
	if (NULL == tcp_ptr) {
        dbg_perror("[error, tcp_client_mod_stop failed!]\r\n");
        return -1;
    }
	tcp_ptr->send_pth.run_flag = false;
	tcp_ptr->recv_pth.run_flag = false;
    tcp_ptr->recv_handle_pth.run_flag = false;
    tcp_ptr->connect_pth.run_flag = false;
    tcp_ptr->keep_alive_pth.run_flag = false;
	return 0;
}


int tcp_client_mod_init(client_mod_t * tcp_ptr, char *server_addr, int server_port, int msg_fd)
{   
    int ret;
    if (NULL == server_addr || tcp_ptr == NULL || server_port <= 0) {
        dbg_perror("[error] tcp_client_mod_init failed!\r\n");
        return -1;
    }
    ret = fifo_init(&s_recv_fifo, TCP_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("[error] tcp_client_mod_init fifo_init failed 1 !\r\n");
        return -1;
    }
    ret = fifo_init(&s_send_fifo, TCP_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("[error] tcp_client_mod_init fifo_init failed 2 !\r\n");
        return -1;
    }
    tcp_ptr->recv_fifo = &s_recv_fifo;
	tcp_ptr->send_fifo = &s_send_fifo;
    tcp_ptr->msg_queue_fd = msg_fd;
	memcpy(tcp_ptr->g_client_info.server_ip, server_addr, CLIENT_DEV_IPLEN);
    tcp_ptr->g_client_info.server_port = server_port;
    tcp_ptr->g_client_info.dev_type = 0;
    tcp_ptr->g_client_info.tcp_socketfd = 0;
    tcp_ptr->g_client_status.g_connect_master_status = UNCONNECT_SERVER;
    tcp_ptr->g_client_status.send_time = get_current_time();
    tcp_ptr->g_client_status.recv_time = get_current_time();
    return 0;
}


void tcp_client_mod_deinit(client_mod_t *tcp_ptr)
{
    if (NULL == tcp_ptr) {
        dbg_perror("[error, tcp_client_mod_deinit failed!]\r\n");
        return;
    }
    tcp_client_mod_stop(tcp_ptr);
	close(tcp_ptr->g_client_info.tcp_socketfd);
    tcp_ptr->g_client_info.tcp_socketfd = 0;
}

