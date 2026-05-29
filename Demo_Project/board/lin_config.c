/*
 * Copyright 2020-2026 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * @file lin_config.c
 * @brief 
 * 
 */



#include "lin_config.h"


#define CLASSIC_PID_NUM 2
static uint8_t classicPID[CLASSIC_PID_NUM]={ 0x3C,0x3D };



/*lin_config0*/


lin_state_t lin_config0_State;
lin_user_config_t lin_config0 = {
    .baudRate=19200U,
    .nodeFunction=false,
    .autobaudEnable=true,
    .timerGetTimeIntervalCallback=NULL,
    .classicPID=classicPID,
    .numOfClassicPID=CLASSIC_PID_NUM,
};

