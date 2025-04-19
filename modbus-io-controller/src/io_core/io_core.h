#pragma once

#include "sys_init.h"
#include "io_objects.h"

// Forward declarations to avoid circular dependencies
struct BoardConfig;

// Common defines
#define MODBUS_HOLDING_REG_SLAVE_ID 0

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

// Board configuration application functions
void apply_board_configs(void);
bool apply_thermocouple_config(BoardConfig* config);
uint8_t findFreeDeviceIndex(void);

// Board specific handlers
void manage_thermocouple(uint8_t index);
void manage_universal_in(uint8_t index);
void manage_digital_io(uint8_t index);
void manage_power_meter(uint8_t index);
