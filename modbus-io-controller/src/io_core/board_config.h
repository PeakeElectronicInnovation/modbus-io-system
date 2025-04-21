#pragma once

#include "../sys_init.h"
#include "io_objects.h"

// LittleFS configuration
#define BOARD_CONFIG_FILENAME "/board_config.json"
#define BOARD_CONFIG_MAGIC_NUMBER 0x66

// Maximum number of boards that can be configured
#define MAX_BOARDS 16

// Maximum board name length (13 chars + null terminator)
#define MAX_BOARD_NAME_LENGTH 14

// Forward declaration of WebServer class from Arduino framework
//class WebServer;
extern WebServer server;

// Board configuration structure
struct BoardConfig {
    char boardName[MAX_BOARD_NAME_LENGTH];
    deviceType_t type;
    uint8_t boardIndex;
    uint8_t slaveID;
    uint8_t modbusPort;
    uint32_t pollTime;
    bool initialised; // Track if board has been initialised with address assignment
    bool connected;   // Track if board is responding to Modbus requests
    
    // Board-specific settings
    union {
        struct {
            struct {
                bool alertEnable;
                bool outputEnable;
                bool alertLatch;
                bool alertEdge;
                uint8_t tcType;
                float alertSetpoint;
                uint8_t alertHysteresis;
            } channels[8];
        } thermocoupleIO;
        
        // Add more board-specific settings here as needed
    } settings;
};

// Board configuration manager APIs
void init_board_config(void);
bool loadBoardConfig(void);
bool saveBoardConfig(void);
void setupBoardConfigAPI(void);

// API handlers
void handleGetBoardConfig(void);
void handleAddBoard(void);
void handleUpdateBoard(void);
void handleDeleteBoard(void);
void handleGetAllBoards(void);
void handleInitialiseBoard(void);

// Board management functions
bool addBoard(BoardConfig newBoard);
bool updateBoard(uint8_t index, BoardConfig updatedBoard);
bool deleteBoard(uint8_t index);
uint8_t getBoardCount(void);
BoardConfig* getBoard(uint8_t index);
uint8_t assignBoardIndex(deviceType_t type);
uint8_t assignSlaveID(uint8_t modbusPort);
bool initialiseBoard(uint8_t index);

// Helper functions
const char* getDeviceTypeName(deviceType_t type);

// Global variables
extern BoardConfig boardConfigs[MAX_BOARDS];
extern uint8_t boardCount;
