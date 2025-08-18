#pragma once

#include "sys_init.h"
#include "io_objects.h"

// Forward declarations to avoid circular dependencies
struct BoardConfig;

// Top level structs
struct modbusConfig_t {
    ModbusRTUMaster *bus;
    bool idAssigned[243];
};

// Object definitions
extern ModbusRTUMaster bus1;
extern ModbusRTUMaster bus2;

extern modbusConfig_t modbusConfig[2];
extern thermocoupleIO_index_t thermocoupleIO_index;

void init_io_core(void);
void manage_io_core(void);
uint8_t assign_address(modbusConfig_t *busCfg);
void setSlaveIDInUse(uint8_t slaveID, uint8_t modbusPort);

// Board configuration application functions
void apply_board_configs(void);
bool apply_thermocouple_config(BoardConfig* config);
uint8_t findFreeDeviceIndex(void);

// Board specific handlers
// Analogue digital IO board management functions ------------>
void manage_analogue_digital_io(uint8_t index);

// Thermocouple board management functions ------------------->
void manage_thermocouple(uint8_t index);
bool setup_thermocouple(uint8_t index);
bool thermocouple_latch_reset(uint8_t index, uint8_t channel);
bool thermocouple_latch_reset_all(uint8_t index);
bool record_thermocouple(uint8_t index);

// RTD board management functions ---------------------------->
void manage_rtd(uint8_t index);

// Energy meter board management functions ------------------->
void manage_energy_meter(uint8_t index);

// Fault and alarm handling functions ------------------------>
void handle_faults_and_alarms(void);

// Print board configuration --------------------------------->
void print_board_config(uint8_t index);