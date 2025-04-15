#pragma once

#include "sys_init.h"

// Common defines
#define MODBUS_HOLDING_REG_SLAVE_ID 32      // TEMP ONLY!!! NEEDS TO BE STANDARDIZED TO ADDR 0x0000

// Top level structs
struct modbusConfig_t {
    ModbusRTUMaster *bus;
    bool idAssigned[243];
};

// Object definitions
extern ModbusRTUMaster bus1;
extern ModbusRTUMaster bus2;

extern modbusConfig_t modbusConfig[2];

void init_io_core(void);
void manage_io_core(void);
uint8_t assign_address(modbusConfig_t *busCfg);

// Board specific handlers
void manage_thermocouple(uint8_t index);
void manage_universal_in(uint8_t index);
void manage_digital_io(uint8_t index);
void manage_power_meter(uint8_t index);
