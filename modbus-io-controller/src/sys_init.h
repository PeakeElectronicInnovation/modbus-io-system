#pragma once

#define VERSION "1.0.0"

// Include libraries
#include <Arduino.h>
#include <W5500lwIP.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "Adafruit_Neopixel.h"
#include "MCP79410.h"
#include "ModbusRTUMaster.h"

// Include program files
#include "hardware/pins.h"

#include "network/network.h"

#include "utils/logger.h"
#include "utils/statusManager.h"
#include "utils/timeManager.h"
#include "utils/powerManager.h"
#include "utils/terminalManager.h"

#include "storage/sdManager.h"

#include "io_core/io_core.h"

void init_core0(void);
void init_core1(void);
void manage_core0(void);
void manage_core1(void);

// Task handler prototypes
void handleSDManager(void);
void handleLEDManager(void);
void handleTimeManager(void);
void handlePowerManager(void);
void handleTerminalManager(void);
void handleIpcManager(void);
void handleNetworkManager(void);

// Object definitions

extern bool core0setupComplete;
extern bool core1setupComplete;

extern bool debug;