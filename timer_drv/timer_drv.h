#ifndef __TIMER_DRV_H__
#define __TIMER_DRV_H__

#include "common.h"
#include <sys/time.h>
#include <signal.h>
#include <time.h>

void timer_signal_init(void *arg);
void start_or_stop_time(int sec, int usec);
void posix_timer_init(timer_t *timer_id, void *func, int arg);
void posix_timer_start(timer_t timer_id, int sec, int nsec);
void posix_timer_uninit(timer_t timer_id);

#endif

