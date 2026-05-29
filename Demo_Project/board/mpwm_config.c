/*
 * Copyright 2020-2026 Yuntu Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * @file mpwm_config.c
 * @brief 
 * 
 */


#include <stddef.h>
#include "mpwm_config.h"

/*
 * Common state
 */
mpwm_state_t MPWM_State0;


/*
 * Common timer
 */


/*
 * PWM
 */

mpwm_pwm_ch_config_t MPWM_PWM_Config7ChConfig[1]={
    {
        .hwChannelId = 7,
        .clkPrs = MPWM_CLK_DIV_BY_1,
        .frequencyHz = 1000,
        .unit = MPWM_UPDATE_IN_DUTY_CYCLE,
        .dutyCyclePercent = 0x8000,
        .alignMode = MPWM_PWM_RIGHT_ALIGN,
        .outputInit = MPWM_OUTPUT_INIT_0,
        .debugMode = false,
        .reloadSrc = MPWM_RELOAD_DISABLE,
        .outputTrigConfig = {
            false,
            false,
        },
        .intEn = false,
        .timerOverflowIntEn = false,
    },
};

mpwm_pwm_config_t MPWM_PWM_Config7={
    .channelNum=1,
    .pwmChannelConfig=MPWM_PWM_Config7ChConfig,
};


/*
 * Output compare
 */


/*
 * Input capture
 */


/*
 * Pulse count
 */


