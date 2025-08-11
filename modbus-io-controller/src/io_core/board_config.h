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
    uint32_t recordInterval;
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
                char channelName[33];
                bool recordTemperature;
                bool recordColdJunction;
                bool recordStatus;
                bool showOnDashboard;
                bool monitorFault;
                bool monitorAlarm;
            } channels[8];
        } thermocoupleIO;
        
        // Add more board-specific settings here as needed
    } settings;
};

// Binary configuration format structures
// Packed thermocouple channel configuration (saves ~200 bytes per board)
struct __attribute__((packed)) BinaryThermocoupleChannel {
    uint16_t flags;           // Bit-packed boolean flags (14 bools -> 2 bytes)
    uint8_t tcType;           // Thermocouple type
    float alertSetpoint;      // Alert setpoint temperature
    uint8_t alertHysteresis;  // Alert hysteresis
    char channelName[33];     // Channel name (keep as-is for compatibility)
};

// Binary board configuration header
struct __attribute__((packed)) BinaryBoardHeader {
    uint8_t magic;            // Magic number for validation
    uint8_t version;          // Binary format version
    uint8_t boardCount;       // Number of boards
    uint8_t reserved;         // Reserved for future use
};

// Binary board configuration entry
struct __attribute__((packed)) BinaryBoardConfig {
    char boardName[MAX_BOARD_NAME_LENGTH];
    uint8_t type;
    uint8_t boardIndex;
    uint8_t slaveID;
    uint8_t modbusPort;
    uint32_t pollTime;
    uint32_t recordInterval;
    uint8_t flags;            // initialised, connected flags
    uint8_t reserved[3];      // Padding for alignment
    
    // Board-specific data follows
    union {
        BinaryThermocoupleChannel thermocoupleChannels[8];
        uint8_t rawData[8 * sizeof(BinaryThermocoupleChannel)];
    } settings;
};

// Binary format constants
#define BINARY_CONFIG_MAGIC 0xBC
#define BINARY_CONFIG_VERSION 1
#define BINARY_CONFIG_FILENAME "/board_config.bin"

// Thermocouple channel flag bit positions
#define TC_FLAG_ALERT_ENABLE      0
#define TC_FLAG_OUTPUT_ENABLE     1
#define TC_FLAG_ALERT_LATCH       2
#define TC_FLAG_ALERT_EDGE        3
#define TC_FLAG_RECORD_TEMP       4
#define TC_FLAG_RECORD_CJ         5
#define TC_FLAG_RECORD_STATUS     6
#define TC_FLAG_SHOW_DASHBOARD    7
#define TC_FLAG_MONITOR_FAULT     8
#define TC_FLAG_MONITOR_ALARM     9

// Board configuration manager APIs
void init_board_config(void);
bool loadBoardConfig(void);
bool saveBoardConfig(void);
void setupBoardConfigAPI(void);

// Binary serialization functions
bool saveBoardConfigBinary(void);
bool loadBoardConfigBinary(void);
void packThermocoupleChannel(const BoardConfig* board, int channelIndex, BinaryThermocoupleChannel* binaryChannel);
void unpackThermocoupleChannel(const BinaryThermocoupleChannel* binaryChannel, BoardConfig* board, int channelIndex);
uint16_t packChannelFlags(const BoardConfig* board, int channelIndex);
void unpackChannelFlags(uint16_t flags, BoardConfig* board, int channelIndex);

// API handlers
void handleGetBoardConfig(void);
void handleAddBoard(void);
void handleUpdateBoard(void);
void handleDeleteBoard(void);
void handleGetAllBoards(void);
void handleInitialiseBoard(void);
void handleExportConfig(void);
void handleImportConfig(void);

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
