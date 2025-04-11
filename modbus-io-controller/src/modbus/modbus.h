#pragma once

#include "../sys_init.h"

struct modbusConfig_t {
    ModbusRTUMaster *modbusMaster;
    uint32_t baudRate;
    uint8_t parity;
    uint8_t stopBits;
    uint8_t flowControl;
};

extern modbusConfig_t modbusConfig[2];

bool init_modbus(void);
void manage_modbus(void);
uint8_t handleAddressSet(modbusConfig_t *config);