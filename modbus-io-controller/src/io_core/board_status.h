#pragma once

#include "../sys_init.h"
#include "io_objects.h"
#include "board_config.h"

// API function declarations
void setupBoardStatusAPI(void);

// API handlers
void handleGetBoardStatus(void);
void handleGetAllBoardsStatus(void);
void handleGetThermocoupleData(void);
