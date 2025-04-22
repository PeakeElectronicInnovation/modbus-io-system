#pragma once

// Forward declaration to avoid circular dependencies
struct deviceIndex_t;

enum deviceType_t {
    MASTER_CONTROLLER,
    ANALOGUE_DIGITAL_IO,
    THERMOCOUPLE_IO,
    RTD_IO,
    ENERGY_METER
};

// Index object
struct deviceIndex_t {
    deviceType_t type = THERMOCOUPLE_IO;
    uint8_t index = 0;
    bool configured = false;
};

// Key standard holding register addresses
#define EXP_HOLDING_REG_STATUS         0    // Status and board type are read only
#define EXP_HOLDING_REG_BOARD_TYPE     1    // written data will be ignored
#define EXP_HOLDING_REG_BOARD_NAME     2
#define EXP_HOLDING_REG_SLAVE_ID       9

// IO board objects defined here ----------------------->
// *Device*_index_t -> *Device*_t -> *Device*_Modbus_t
// Modbus register map defined first in order of function code, then general device config struct with
// configured RS485 bus, slave ID, polling vars and modbus register struct. Then device index struct for
// device type specific indexing (array of 16 devices). The main device index is used to track all devices,
// with device type specifying which method to access the device with.

// Thermocouple interface board objects
// NOTE:Take care that bytes are 32 bit aligned to avoid unintended padding and therefore memcpy issues!
struct thermocoupleModbus_t { // Thermocouple interface board modbus registers (FC01 - FC04)
    // FC01 - Read Coils    // uint16 address    | uint8 ptr
    bool alertEnable[8];    // 0-7               | 0
    bool outputEnable[8];   // 8-15              | 8
    bool alertLatch[8];     // 16-23             | 16
    bool alertEdge[8];      // 24-31             | 24

    // FC02 - Read Discrete Inputs
    bool outputState[8];    // 0-7               | 32
    bool alarmState[8];     // 8-15              | 40
    bool openCircuit[8];    // 16-23             | 48
    bool shortCircuit[8];   // 24-31             | 56

    // FC03 - Read Holding Registers
    uint16_t status;        // 0                 | 64
    uint16_t boardType;     // 1                 | 66
    char boardName[14];     // 2-8               | 68
    uint16_t slaveID;       // 9                 | 82
    uint16_t type[8];       // 10-17             | 84
    float alertSP[8];       // 18-33             | 100
    uint16_t alarmHyst[8];  // 34-41             | 132

    // FC04 - Read Input Registers
    float temperature[8];   // 0-15              | 148
    float coldJunction[8];  // 16-31             | 180
    float deltaJunction[8]; // 32-47             | 212 -> 244
};

// TCIO specific holding register addresses
#define TCIO_HOLDING_REG_TYPE           10
#define TCIO_HOLDING_REG_ALERT_SP       18
#define TCIO_HOLDING_REG_ALARM_HYST     34

struct thermocoupleIO_t {
    ModbusRTUMaster *bus;
    uint8_t slaveID;
    char boardName[14];
    uint32_t lastUpdate;
    uint32_t pollTime;
    thermocoupleModbus_t reg;
    bool coils[32];
    uint16_t holdingRegisters[40]; // first 2 registers are excluded!!! read only
    bool configInitialised = false;
    bool modbusError = false;
    bool I2CError = false;
    bool PSUError = false;
    float Vpsu = 0;
};
  
struct themocoupleIO_index_t {
    thermocoupleIO_t tcIO[16];
};