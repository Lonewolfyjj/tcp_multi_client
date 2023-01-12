#include "timer_drv.h"





/******************************************************************************************
 * Function Name :  timer_signal_init
 * Description   :  定时器信号初始化
 * Parameters    :  arg：即函数指针，定时器定时时间到后，要执行的函数
 * Returns       :  无
*******************************************************************************************/
void timer_signal_init(void *arg)
{
    struct sigaction act;
    act.sa_handler = arg;                    //定时要处理的函数arg
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask); 
    sigaction(SIGALRM, &act, NULL);          //设置信号 SIGPROF 的处理函数为 arg

}

/******************************************************************************************
 * Function Name :  start_or_stop_time
 * Description   :  开启或关闭定时器，当sec和usec都为0时，定时器关闭
 * Parameters    :  sec：秒数，
                    usec: 微秒数
 * Returns       :  无
*******************************************************************************************/
void start_or_stop_time(int sec, int usec) 
{ 
    struct itimerval value; 
    value.it_value.tv_sec = sec;            //定时器启动后，每隔sec秒将执行相应的函数
    value.it_value.tv_usec= usec; 
    value.it_interval = value.it_value; 
    setitimer(ITIMER_REAL, &value, NULL);   //初始化 timer，到期发送 SIGPROF 信号
} 

/******************************************************************************************
 * Function Name :  posix_timer_init
 * Description   :  posix定时器信号初始化
 * Parameters    :  timer_id：定时器id变量的地址
 *                  func：即函数指针，定时器定时到期执行的函数
 *                  arg：执行函数的整型参数
 * Returns       :  无
*******************************************************************************************/
void posix_timer_init(timer_t *timer_id, void *func, int arg)
{
    struct sigevent evp;
    memset(&evp, 0, sizeof(evp));
    evp.sigev_value.sival_ptr = timer_id;
    evp.sigev_notify = SIGEV_THREAD;        
    evp.sigev_notify_function = func;	//函数指针
    evp.sigev_value.sival_int = arg;   //作为func()的参数
    timer_create(CLOCK_REALTIME, &evp, timer_id);
    return;
}

/******************************************************************************************
 * Function Name :  posix_timer_start
 * Description   :  启动posix定时器
 * Parameters    :  timer_id：定时器id
 *                  sec：定时秒数
 *                  nsec：定时纳秒数
 * Returns       :  无
*******************************************************************************************/
void posix_timer_start(timer_t timer_id, int sec, int nsec)
{
    struct itimerspec ts;
    ts.it_interval.tv_sec = sec;		//定时的秒数
    ts.it_interval.tv_nsec = nsec;	//定时的纳秒数
    ts.it_value.tv_sec = 3;
    ts.it_value.tv_nsec = 0;
    timer_settime(timer_id, TIMER_ABSTIME, &ts, NULL);
    return;
}

void posix_timer_uninit(timer_t timer_id)
{
    timer_delete(timer_id);
}
