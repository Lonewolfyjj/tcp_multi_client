#include "tcp_server_mod.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "msg_queue_mod.h"

#define TCP_SERVER_FIFO_SIZE    256
static fifo_buf_t s_recv_fifo;
static fifo_buf_t s_send_fifo;

enum decode_state {
    READ_PACK_HEAD,
    READ_PACK_DATA,
    SELECT_LISTEN_FD,
    SELECT_CLIENT_FD
};
static int get_current_time(void)
{
    time_t times = time(NULL);
    return times;
}

static void set_client_send_time(int sockfd, server_mod_t *tcp_ptr) 
{
    int count = 0;
    for (count = 0; count < CLIENT_MAX_NUM; count++) {
        if (tcp_ptr->client_info[count].client_socketfd == sockfd) {
            tcp_ptr->client_info[count].send_time = get_current_time();
            break;
        }
    }
}

static void tcp_server_bind_ip_port(server_mod_t *tcp_ptr)
{
    int one = 1, sockfd;
    struct sockaddr_in  recv_addr;
	socklen_t addrlen = sizeof(recv_addr);
    if (NULL == tcp_ptr) {
        dbg_perror("[error] tcp_server_bind_ip failed!\r\n");
        return;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd){
        dbg_perror("[error] socket failed failed!\r\n");
        return;
	}
	recv_addr.sin_family = AF_INET;	
	recv_addr.sin_port =  htons(tcp_ptr->server_info.port);
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
	if (bind(sockfd, (struct sockaddr *)&recv_addr, addrlen) < 0) {
		dbg_perror("[error] bind failed!\r\n");
        return;
	}
    listen(sockfd, tcp_ptr->server_info.listen_num);
    tcp_ptr->server_info.tcp_socketfd = sockfd;
}


/******************************************************************************************
 * Function Name :  check_fd_isread
 * Description   :  监听输入的句柄是否可读
 * Parameters    :  fd：文件描述符
 *                  sec:阻塞监听秒数
 *                  usec:阻塞监听微秒数
 * Returns       :  无
*******************************************************************************************/
static int check_fd_isread(server_mod_t *tcp_ptr, int select_mod, int sec, int usec)
{
    int ret = 0, count, num = 0;

    struct timeval timeout;
    FD_ZERO(&tcp_ptr->read_fd_group);
    
    switch (select_mod) {
        case SELECT_CLIENT_FD:
            for (count = 0; count < CLIENT_MAX_NUM; count++) {
                if (tcp_ptr->client_info[count].client_socketfd != 0) {
                    tcp_ptr->fd_group[num++] = tcp_ptr->client_info[count].client_socketfd;
                    FD_SET(tcp_ptr->client_info[count].client_socketfd, &tcp_ptr->read_fd_group);
                }            
            }
            break;
        case SELECT_LISTEN_FD:
            FD_SET(tcp_ptr->server_info.tcp_socketfd, &tcp_ptr->read_fd_group);
            break;
        default :
            printf("[error] select add fd error\n");
            break;
    }
    
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    ret = select(sizeof(tcp_ptr->read_fd_group)+1, &tcp_ptr->read_fd_group, NULL, NULL, &timeout);
    return ret;
}

static void tcp_close_client_connect(int error_fd, server_mod_t *tcp_ptr)
{
    int count;
    for (count = 0; count < tcp_ptr->client_conectnum; count++) {
        if (tcp_ptr->client_info[count].client_socketfd == error_fd) {
            close(error_fd);
            tcp_ptr->client_info[count].client_socketfd = 0;
            tcp_ptr->client_info[count].connect_client_status = UNCONNECT_CLIENT;
            tcp_ptr->client_conectnum--;
            memset(tcp_ptr->client_info[count].ip_addr, 0, sizeof(tcp_ptr->client_info[count].ip_addr));
            break;
        }
    }    
}

static void server_add_newclient_fd(server_mod_t *tcp_ptr, char *ip_addr, int new_fd)
{
    int count = 0;
    for (count = 0; count < CLIENT_MAX_NUM; count++) {
        if (strcmp(tcp_ptr->client_info[count].ip_addr, ip_addr) != 0) {               
            if (tcp_ptr->client_conectnum >= CLIENT_MAX_NUM) {
                printf("server connect max num full\n");
                break;
            }
            if (strlen(tcp_ptr->client_info[count].ip_addr) == 0) {
                printf("you is first connect, count:%d\n", count);
                tcp_ptr->client_info[count].send_time = get_current_time();
                memcpy(tcp_ptr->client_info[count].ip_addr, ip_addr, strlen(ip_addr));
                tcp_ptr->client_info[count].client_socketfd = new_fd;
                tcp_ptr->client_info[count].connect_client_status = CONNECT_CLIENT;
                tcp_ptr->client_conectnum++;
                break;
            }
        } else {
            close(new_fd);
            printf("[ip exist]\n");
            break;
        }
    } 
}

//监听套接字等待连接
static void *tcp_server_listen_pthread(void *arg)
{
    int ret = 0, new_fd, client_len;
    char ip_addr[32] = {0};
    server_mod_t *tcp_ptr = (server_mod_t *)arg;
    struct sockaddr_in client_address;
    client_len = sizeof(client_address);
    
    while (tcp_ptr->listen_pth.run_flag) {
        ret = check_fd_isread(tcp_ptr, SELECT_LISTEN_FD, 0, 100000);//select监听fd
        if (ret < 0) {
            dbg_perror("[error] check_fd_isread  listen fd failed!\r\n");
            close(tcp_ptr->server_info.tcp_socketfd);
            tcp_server_bind_ip_port(tcp_ptr);
            continue;
        } else if (ret == 0) {
            continue;
        }
        //接收socketfd
        new_fd = accept(tcp_ptr->server_info.tcp_socketfd, (struct sockaddr *)&client_address, &client_len);
        inet_ntop(AF_INET, &(client_address.sin_addr) , ip_addr, sizeof(ip_addr));//将网络字节序IP转换成字符串IP
        printf("client connect server succeess, client ip[%s], port[%d]\n", ip_addr, ntohs(client_address.sin_port));
        printf("[this nsockfd:%d]\n", new_fd);
        server_add_newclient_fd(tcp_ptr, ip_addr, new_fd);                                            
    }
    
}

static void recv_data_write_fifo(fifo_buf_t *fifo_w, uint8_t *data_buf, int length, uint8_t *sockfd)
{
    uint8_t data_len = 0;
    while(1) {
        if (fifo_get_writeable_num(fifo_w) > (length + 5)) {
            break;
        } else {
            printf("[fifo write full]\n");
            usleep(5);
        }
    }
    fifo_write(fifo_w, sockfd, 1);
    data_len = (length >> 24) & 0xff;
    fifo_write(fifo_w, &data_len, 1);
    data_len = (length >> 16) & 0xff;
    fifo_write(fifo_w, &data_len, 1);
    data_len = (length >> 8) & 0xff;
    fifo_write(fifo_w, &data_len, 1);
    data_len = (length & 0xff);
    fifo_write(fifo_w, &data_len, 1);
    fifo_write(fifo_w, data_buf, length);
}

static void *tcp_server_recv_pthread(void *arg)
{
    int ret = 0, rettwo = 0, connect_num = 0, count;
    uint8_t buf[TCP_SERVER_FIFO_SIZE];
    server_mod_t *tcp_ptr = (server_mod_t *)arg;

    while (tcp_ptr->recv_pth.run_flag) {
        ret = check_fd_isread(tcp_ptr, SELECT_CLIENT_FD, 0, 100000);//select监听多个socketfd
        if (ret < 0) {
            printf("[error, ret:%d] recv data select failed\r\n", ret);
            sleep(1);
            continue;
        } else if (ret == 0) {
            continue;
        }       
        for (count = 0; count < tcp_ptr->client_conectnum; count++) {           
            if (FD_ISSET(tcp_ptr->fd_group[count], &tcp_ptr->read_fd_group)) {  
                ret = recv(tcp_ptr->fd_group[count], buf, sizeof(buf), 0); 
                if (ret <= 0) {
                    tcp_close_client_connect(tcp_ptr->fd_group[count], tcp_ptr); //不break, 因为可能还有其他客户端的数据要处理
                    continue;
                }
                printf("[recv client fd:%d, data num:%d]\n", tcp_ptr->fd_group[count], ret);
                recv_data_write_fifo(tcp_ptr->recv_fifo, buf, ret, (uint8_t *)&tcp_ptr->fd_group[count]);                      
            }        
        }
    }
}


int tcp_server_mod_send(int sockfd, uint8_t *data, int data_size, uint8_t cmd)
{
    uint8_t data_out[TCP_SERVER_FIFO_SIZE];
    int ret;
    //1、HIP打包数据
    ret = hip_mod_pack(data, data_size, cmd, data_out);
    //2、发送给对应的tcp客户端
    send(sockfd, data_out, ret, 0);
}


static void tcp_server_cmd_deal(int sockfd, hip_msg_pack_t *hip_msg, server_mod_t *tcp_ptr)
{
    uint8_t commond = hip_msg->msg_head.commond;
    switch (commond) {
        case HIP_USER_LOGIN_PACK:
            set_client_send_time(sockfd, tcp_ptr);
            tcp_server_mod_send(sockfd, hip_msg->data, hip_msg->data_len, HIP_USER_LOGIN_PACK);
            break;
        case HIP_KEEP_ALIVE_PACK:
            set_client_send_time(sockfd, tcp_ptr);
            tcp_server_mod_send(sockfd, hip_msg->data, hip_msg->data_len, HIP_KEEP_ALIVE_PACK);
            break;
        case HIP_UART_PASS_THROUGH_PACK:
            msg_queue_send(tcp_ptr->msg_queue_fd, HIP_UART_PASS_THROUGH_PACK, hip_msg->data, hip_msg->data_len);
            printf("[ok] send msg queue\n");
            break;
        default :
            printf("[error] tcp_server_cmd_deal\n");
            break;
    }
}



static int read_recv_data_pack(fifo_buf_t *read_fifo, hip_msg_pack_t *msg_pack, int pack_size)
{
    int state = READ_PACK_HEAD, num = 0, data_len, read_num = 0;
    bool run_flag = true;
    uint8_t data_buf[TCP_SERVER_FIFO_SIZE];

    while (run_flag) {       
        switch (state) {
            case READ_PACK_HEAD:
                if (fifo_get_readable_num(read_fifo) >= BYTE_PACK_HEAD_SIZE) {
                    fifo_read(read_fifo, data_buf, BYTE_PACK_HEAD_SIZE);
                    data_len = (data_buf[2] << 8) | data_buf[3];
                    memcpy((uint8_t *)&msg_pack[num].msg_head, data_buf, BYTE_PACK_HEAD_SIZE);
                    read_num += BYTE_PACK_HEAD_SIZE;
                    state = READ_PACK_DATA;
                }
                break;  
            case READ_PACK_DATA:
                if (fifo_get_readable_num(read_fifo) >= data_len) {
                    fifo_read(read_fifo, data_buf, data_len);
                    memcpy(msg_pack[num].data, data_buf, data_len);
                    msg_pack[num].data_len = data_len;
                    read_num += data_len;
                    num++;
                    if (read_num == pack_size) {
                        run_flag = false;
                    } else {
                        state = READ_PACK_HEAD;
                    }
                }
                break;
            default :
                printf("[error] read_recv_data_pack decode error\n");
                break;
        }
        usleep(10);
    }
    return num;
}



static void *tcp_server_recv_handle_pthread(void *arg)
{
    int ret = 0, count, counttwo, pack_num = 0, sockfd, data_len;
    server_mod_t *tcp_ptr = (server_mod_t *)arg;   
    hip_msg_pack_t msg_data_pack[10];
    uint8_t data_buf[TCP_SERVER_FIFO_SIZE];
    
    while (tcp_ptr->recv_handle_pth.run_flag) {
        ret = fifo_get_readable_num(tcp_ptr->recv_fifo);
        if (ret >= 5) {
            memset(msg_data_pack, 0, sizeof(msg_data_pack));
            fifo_read(tcp_ptr->recv_fifo, data_buf, 5);           
            sockfd = data_buf[0];
            data_len = (data_buf[1] << 24) | (data_buf[2] << 16) | (data_buf[3] << 8) | data_buf[4];
            pack_num = read_recv_data_pack(tcp_ptr->recv_fifo, msg_data_pack, data_len);
            printf("[decode pack num:%d]\n", pack_num);
            for (count = 0; count < pack_num; count++) {
                tcp_server_cmd_deal(sockfd, &msg_data_pack[count], tcp_ptr);
                printf("[ok] server cmd deal\n");
            }          
        } else {
            usleep(1000);
        }

    }
}

static void *tcp_server_check_alive_pthread(void *arg)
{
    int count = 0;
    int cur_time;
    server_mod_t *tcp_ptr = (server_mod_t *)arg;

    while (tcp_ptr->check_alive_pth.run_flag) {
        cur_time = get_current_time();
        //检测每一个对应的客户端发送的心跳时间，和约定时间是否超过，超过就将其连接状态置位unconnect,清理
        for (count = 0; count < CLIENT_MAX_NUM; count++) {
            if (tcp_ptr->client_info[count].connect_client_status == CONNECT_CLIENT) {
                if ((cur_time - tcp_ptr->client_info[count].send_time) >= CLIENT_KEEP_ALIVE_TIME) {
                    printf("[%s is unconnect, start close sockfd:%d]\n", tcp_ptr->client_info[count].ip_addr, tcp_ptr->client_info[count].client_socketfd);
                    printf("[check cur_time:%d, send_time:%d]\n", cur_time , tcp_ptr->client_info[count].send_time);
                    close(tcp_ptr->client_info[count].client_socketfd);
                    tcp_ptr->client_info[count].client_socketfd = 0;
                    tcp_ptr->client_conectnum--;
                    memset(tcp_ptr->client_info[count].ip_addr, 0, sizeof(tcp_ptr->client_info[count].ip_addr));
                    tcp_ptr->client_info[count].connect_client_status = UNCONNECT_CLIENT;
                }
            }
            
        }
        usleep(100000);
    }
}


int tcp_server_mod_start(server_mod_t *tcp_ptr)
{   
    if (NULL == tcp_ptr) {
        dbg_perror("tcp_server_mod_start failed 1 !\r\n");
        return -1;
    }
    tcp_ptr->listen_pth.run_flag = true;
    tcp_ptr->recv_pth.run_flag = true;
    tcp_ptr->recv_handle_pth.run_flag = true;
    tcp_ptr->check_alive_pth.run_flag = true;

    pthread_create(&tcp_ptr->listen_pth.tid, NULL, tcp_server_listen_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->listen_pth.tid);
	pthread_create(&tcp_ptr->recv_pth.tid, NULL, tcp_server_recv_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->recv_pth.tid);
    pthread_create(&tcp_ptr->recv_handle_pth.tid, NULL, tcp_server_recv_handle_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->recv_handle_pth.tid);
	pthread_create(&tcp_ptr->check_alive_pth.tid, NULL, tcp_server_check_alive_pthread, (void *)tcp_ptr);
    pthread_detach(tcp_ptr->check_alive_pth.tid);
    return 0;
}

int tcp_server_mod_stop(server_mod_t *tcp_ptr)
{
    if (NULL == tcp_ptr) {
        dbg_perror("tcp_server_mod_stop failed 1 !\r\n");
        return -1;
    }
    tcp_ptr->listen_pth.run_flag = false;
    tcp_ptr->recv_pth.run_flag = false;
    tcp_ptr->recv_handle_pth.run_flag = false;
    tcp_ptr->check_alive_pth.run_flag = false;
    return 0;
}

int tcp_server_mod_init(server_mod_t *tcp_ptr, char *ip_addr, int port, int msg_fd)
{
    int ret, count;
    if (NULL == ip_addr || tcp_ptr == NULL || port <= 0) {
        dbg_perror("[error] tcp_client_mod_init failed!\r\n");
        return -1;
    }
    ret = fifo_init(&s_recv_fifo, TCP_SERVER_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("[error] tcp_client_mod_init fifo_init failed 1 !\r\n");
        return -1;
    }
    ret = fifo_init(&s_send_fifo, TCP_SERVER_FIFO_SIZE);
    if (ret < 0) {
        dbg_perror("[error] tcp_client_mod_init fifo_init failed 1 !\r\n");
        return -1;
    }
    //结构体变量初始化
    memset(&tcp_ptr->server_info, 0, sizeof(server_info_t));
    for (count = 0; count < CLIENT_MAX_NUM; count++) {
        memset(&tcp_ptr->client_info[count], 0, sizeof(ser_cli_connect_status_t));
    }
    tcp_ptr->recv_fifo = &s_recv_fifo;
    tcp_ptr->send_fifo = &s_send_fifo;
    //监听最大数量
    tcp_ptr->server_info.listen_num = CLIENT_MAX_NUM;
    tcp_ptr->server_info.port = port;
    tcp_ptr->client_conectnum = 0;
    tcp_ptr->msg_queue_fd = msg_fd;
    tcp_server_bind_ip_port(tcp_ptr);   
    return 0;
}


void tcp_server_mod_deinit(server_mod_t *tcp_ptr)
{
    if (NULL == tcp_ptr) {
        dbg_perror("tcp_server_mod_deinit failed 1 !\r\n");
        return;
    }
    //停止线程
    tcp_server_mod_stop(tcp_ptr);
    //关闭套接字
    close(tcp_ptr->server_info.tcp_socketfd);
    //释放FIFO
    fifo_uninit(&s_recv_fifo);
    fifo_uninit(&s_send_fifo);
    return;
}