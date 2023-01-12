/******************************************************************************
* Copyright (C), 2016-2017, Sunny.Guo
* FileName: common.h 
* Author: Sunny.Guo
* Version: 1.0
* Date: 2017�� 01�� 19�� ������ 10:12:50 CST
* Description: Common definition         
*
* History:        
*  <author>  	<time>   	<version >   	<desc>
*  Sunny.Guo   	19/01/2017      1.0     	create this moudle  
*                                                                          
* Licensed under GPLv2 or later, see file LICENSE in this source tree.
*******************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG 1

#ifdef DEBUG
#define dbg_printf(fmt, args...) printf(fmt, ##args)
#define dbg_perror(msg) (perror(msg))
#else
#define dbg_printf(fmt, args...)
#define dbg_perror(msg)
#endif

#define MIN(x,y)  (((x)<(y))?(x):(y))
#define MAX(x,y)  (((x)>(y))?(x):(y))

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1

#define MAX_INPUT 		255

typedef struct pthread_param_struct {
    bool run_flag;
    pthread_t tid;
}pth_stru_t, *pth_stru_p;

#endif		// __COMMON_H__

