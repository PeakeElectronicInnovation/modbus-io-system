#include "sys_init.h"

// Object definitions

bool core0setupComplete = false;
bool core1setupComplete = false;

// Ensure both cores have an 8k stack size (default is to split 8k accross both cores)
bool core1_separate_stack = true;

bool debug = true;

void init_core0(void);

void init_core1(void);

void init_core0(void) {
    init_logger();
    init_network();
    setupWebServer(); // Setup the web server routes but don't start it yet
}

void init_core1(void) {
    init_statusManager();
    init_timeManager();
    init_powerManager();
    init_terminalManager();
    while (!core0setupComplete) delay(100); // Wait for core0 setup to complete
    init_sdManager();
    init_io_core();         // This will register the board APIs
    startWebServer();       // Now start the web server after all APIs are registered
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
