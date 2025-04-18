#include "sys_init.h"

// Object definitions

bool core0setupComplete = false;
bool core1setupComplete = false;

bool debug = true;

void init_core0(void);

void init_core1(void);

void init_core0(void) {
    init_logger();
    init_network();
}

void init_core1(void) {
    init_statusManager();
    init_timeManager();
    init_powerManager();
    init_terminalManager();
    while (!core0setupComplete) delay(100); // 
    init_sdManager();
    init_io_core();
}

void manage_core0(void) {
    manageNetwork();
}

void manage_core1(void) {
    manageStatus();
    manageTime();
    managePower();
    manageTerminal();
    manageSD();
    manage_io_core();
}
