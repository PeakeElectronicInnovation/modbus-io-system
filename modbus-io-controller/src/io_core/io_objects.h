#pragma once

#include "io_core.h"

enum deviceType_t {
    THERMOCOUPLE_IO,
    UNIVERSAL_IN,
    DIGITAL_IO,
    POWER_METER
};

// Index object
struct deviceIndex_t {
    deviceType_t type = THERMOCOUPLE_IO;
    uint8_t index = 0;
    bool configured = false;
};

// IO board objects defined here ----------------------->
// *Device*_index_t -> *Device*_t -> *Device*_Modbus_t
// Modbus register map defined first in order of function code, then general device config struct with
// configured RS485 bus, slave ID, polling vars and modbus register struct. Then device index struct for
// device type specific indexing (array of 16 devices). The main device index is used to track all devices,
// with device type specifying which method to access the device with.

// Thermocouple interface board objects
struct thermocoupleModbus_t { // Thermocouple interface board modbus regisers (FC01 - FC04)
    // FC01 - Read Coils
    bool alertEnable[8];    // 0-7
    bool outputEnable[8];   // 8-15
    bool alertLatch[8];     // 16-23
    bool alertEdge[8];      // 24-31

    // FC02 - Read Discrete Inputs
    bool outputState[8];    // 0-7
    bool alarmState[8];     // 8-15
    bool openCircuit[8];    // 16-23
    bool shortCircuit[8];   // 24-31

    // FC03 - Read Holding Registers
    uint16_t type[8];       // 0-7
    float alertSP[8];       // 8-23
    uint16_t alarmHyst[8];  // 24-31
    uint16_t slaveID;       // 32  <----- Beware struct padding here, 4 bytes (not memcpy safe!!!)
    uint32_t I2Cerrors;     // 33-34

    // FC04 - Read Input Registers
    float temperature[8];   // 0-15
    float coldJunction[8];  // 16-31
    float deltaJunction[8]; // 32-47
};
struct thermocoupleIO_t {
    ModbusRTUMaster *bus;
    uint8_t slaveID;
    uint8_t status;
    uint32_t lastUpdate;
    uint32_t pollTime;
    thermocoupleModbus_t reg;
    bool coils[32];
    uint16_t holdingRegisters[35];
};
  
struct themocoupleIO_index_t {
    thermocoupleIO_t tcIO[16];
};