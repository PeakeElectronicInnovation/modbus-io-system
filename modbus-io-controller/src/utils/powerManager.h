#pragma once

#include "../sys_init.h"

// Voltage divider constants
#define V_PSU_MUL_V      0.01726436769

// Limits
#define V_PSU_MIN        11.5
#define V_PSU_MAX        29.0

#define POWER_UPDATE_INTERVAL 1000

void init_powerManager(void);
void managePower(void);

// Timestamp global
extern uint32_t powerTS;
