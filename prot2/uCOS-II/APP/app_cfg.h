/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                            ATMEL  AVR32 UC3  Application Configuration File
*
*                                 (c) Copyright 2007; Micrium; Weston, FL
*                                           All Rights Reserved
*
* File    : APP_CFG.H
* By      : Fabiano Kovalski
*********************************************************************************************************
*/


/*
**************************************************************************************************************
*                                               STACK SIZES
**************************************************************************************************************
*/

#define  APP_TASK_START_STK_SIZE          256

/*
**************************************************************************************************************
*                                             TASK PRIORITIES
**************************************************************************************************************
*/

#define  APP_TASK_START_PRIO                1
#define  OS_TASK_TMR_PRIO                   28
