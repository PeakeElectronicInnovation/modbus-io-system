#include "io_core.h"
#include "board_config.h"
#include "io_objects.h"

// Object definitions
ModbusRTUMaster bus1(Serial1);
ModbusRTUMaster bus2(Serial2);

modbusConfig_t modbusConfig[2] = {
    {&bus1, {false}},
    {&bus2, {false}}
};

deviceIndex_t deviceIndex[64];

themocoupleIO_index_t thermocoupleIO_index;

void init_io_core(void) {
    Serial1.setTX(PIN_RS485_TX_1);
    Serial1.setRX(PIN_RS485_RX_1);
    Serial2.setTX(PIN_RS485_TX_2);
    Serial2.setRX(PIN_RS485_RX_2);
    bus1.begin(500000);
    bus2.begin(500000);

    // Initialize board configuration
    init_board_config();

    // Apply saved board configurations
    apply_board_configs();

    log(LOG_INFO, false, "IO Core initialized\n");
}

// Apply board configurations from config file
void apply_board_configs() {
    uint8_t appliedBoards = 0;
    uint8_t count = getBoardCount();

    // Reset device index
    memset(deviceIndex, 0, sizeof(deviceIndex));

    for (uint8_t i = 0; i < count; i++) {
        BoardConfig* board = getBoard(i);
        if (!board) continue;

        switch (board->type) {
            case THERMOCOUPLE_IO:
                if (apply_thermocouple_config(board)) {
                    appliedBoards++;
                }
                break;
            // Add more board types as needed
            default:
                log(LOG_WARNING, false, "Unknown board type %d\n", board->type);
                break;
        }
    }

    log(LOG_INFO, false, "Applied %d board configurations\n", appliedBoards);
}

// Apply thermocouple IO board configuration
bool apply_thermocouple_config(BoardConfig* config) {
    if (!config || config->type != THERMOCOUPLE_IO || config->boardIndex >= 16) {
        return false;
    }

    // Configure the board
    thermocoupleIO_index.tcIO[config->boardIndex].bus = (config->modbusPort == 0) ? &bus1 : &bus2;
    thermocoupleIO_index.tcIO[config->boardIndex].slaveID = config->slaveID;
    thermocoupleIO_index.tcIO[config->boardIndex].status = 0;
    thermocoupleIO_index.tcIO[config->boardIndex].lastUpdate = millis();
    thermocoupleIO_index.tcIO[config->boardIndex].pollTime = config->pollTime;
    thermocoupleIO_index.tcIO[config->boardIndex].reg.slaveID = config->slaveID;

    // Configure channels
    for (uint8_t ch = 0; ch < 8; ch++) {
        thermocoupleIO_index.tcIO[config->boardIndex].reg.alertEnable[ch] = config->settings.thermocoupleIO.channels[ch].alertEnable;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.outputEnable[ch] = config->settings.thermocoupleIO.channels[ch].outputEnable;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.alertLatch[ch] = config->settings.thermocoupleIO.channels[ch].alertLatch;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.alertEdge[ch] = config->settings.thermocoupleIO.channels[ch].alertEdge;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.type[ch] = config->settings.thermocoupleIO.channels[ch].tcType;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.alertSP[ch] = config->settings.thermocoupleIO.channels[ch].alertSetpoint;
        thermocoupleIO_index.tcIO[config->boardIndex].reg.alarmHyst[ch] = config->settings.thermocoupleIO.channels[ch].alertHysteresis;
    }

    // Update device index
    uint8_t idx = findFreeDeviceIndex();
    if (idx < 64) {
        deviceIndex[idx].type = THERMOCOUPLE_IO;
        deviceIndex[idx].index = config->boardIndex;
        deviceIndex[idx].configured = true;
        log(LOG_INFO, false, "Added thermocouple board '%s' with ID %d at index %d\n", 
            config->boardName, config->slaveID, config->boardIndex);
        return true;
    }

    log(LOG_WARNING, false, "Device index is full, cannot add thermocouple board\n");
    return false;
}

// Find a free device index slot
uint8_t findFreeDeviceIndex() {
    for (uint8_t i = 0; i < 64; i++) {
        if (!deviceIndex[i].configured) {
            return i;
        }
    }
    return 255; // Error - no free slot
}

void manage_io_core(void) {
    // Check for configured devices
    for (int i = 0; i < 64; i++) {
        if (deviceIndex[i].configured) {
            switch (deviceIndex[i].type) {
                case THERMOCOUPLE_IO:
                    manage_thermocouple(deviceIndex[i].index);
                    break;
                case UNIVERSAL_IN:
                    manage_universal_in(deviceIndex[i].index);
                    break;
                case DIGITAL_IO:
                    manage_digital_io(deviceIndex[i].index);
                    break;
                case POWER_METER:
                    manage_power_meter(deviceIndex[i].index);
                    break;
            }
        }
    }
}

uint8_t assign_address(modbusConfig_t *busCfg) {
    uint16_t buf[1];
    // Check for device waiting for address assignment (at address 245)
    if (!busCfg->bus->readHoldingRegisters(245, 0, buf, 1)) {
        log(LOG_ERROR, true, "No device waiting for address assignment\n");
        return 0;
    }
    for (uint8_t i = 1; i < 243; i++) {
        if (!busCfg->idAssigned[i]) {
            // Assign address to device
            if (busCfg->bus->writeSingleHoldingRegister(245, MODBUS_HOLDING_REG_SLAVE_ID, i)) {
                busCfg->idAssigned[i] = true;
                log(LOG_INFO, true, "Assigned address %d to device\n", i);
                return i;
            } else {
                log(LOG_ERROR, true, "Failed to assign address %d to device\n", i);
                return 0;
            }
        }
    }
    return 0; // No free addresses available
}

void manage_thermocouple(uint8_t index) {
    // Check if polling time has elapsed
    if(millis() - thermocoupleIO_index.tcIO[index].lastUpdate < thermocoupleIO_index.tcIO[index].pollTime) return;
    thermocoupleIO_index.tcIO[index].lastUpdate = millis();

    // Register buffers
    bool coils[32];
    bool discreteInputs[32];
    uint16_t holdingRegisters[42];
    uint16_t inputRegisters[48];

    // Load Coil and Holding Register buffers with current config values
    memcpy(coils, &thermocoupleIO_index.tcIO[index].reg, sizeof(coils));
    memcpy(holdingRegisters, &thermocoupleIO_index.tcIO[index].reg.slaveID, sizeof(holdingRegisters));
    bool changed = false;

    // Check for changes to writable registers and write if changed
    // Coils ----->
    for (int i = 0; i < sizeof(coils); i++) {
        if (thermocoupleIO_index.tcIO[index].coils[i] != coils[i]) {
            thermocoupleIO_index.tcIO[index].coils[i] = coils[i];
            changed = true;
            log(LOG_INFO, true, "Thermocouple board at index %d coil %d changed to %d\n", index, i, coils[i]);
        }
    }
    if (changed) {
        if(thermocoupleIO_index.tcIO[index].bus->writeMultipleCoils(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, coils, 32)) {
            log(LOG_DEBUG, false, "Thermocouple board at index %d coils written successfully\n", index);
        } else {
            log(LOG_ERROR, true, "Thermocouple board at index %d coils write failed\n", index);
        }
        changed = false;
    }

    // Holding registers ----->
    for (int i = 0; i < 42; i++) {
        if (thermocoupleIO_index.tcIO[index].holdingRegisters[i] != holdingRegisters[i]) {
            thermocoupleIO_index.tcIO[index].holdingRegisters[i] = holdingRegisters[i];
            changed = true;
            log(LOG_INFO, true, "Thermocouple board at index %d holding register %d changed to %d\n", index, i, holdingRegisters[i]);
        }
    }
    if (changed) {
        int retries = 0;
        while (retries < 3) {
            if(thermocoupleIO_index.tcIO[index].bus->writeMultipleHoldingRegisters(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, holdingRegisters, 42)) {
                log(LOG_DEBUG, false, "Holding registers written successfully\n");
                break;
            } else {
                log(LOG_ERROR, false, "Holding registers write failed, retrying...\n");
                retries++;
                delay(100); // Wait before retrying
            }
        } if (retries == 3) {
            log(LOG_ERROR, true, "Thermocouple board at index %d holding registers write failed after 3 retries\n", index);
        }
    }

    // Read registers
    if(thermocoupleIO_index.tcIO[index].bus->readDiscreteInputs(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, discreteInputs, 32)) {
        log(LOG_DEBUG, false, "Thermocouple %d discrete inputs read successfully\n", index);
    } else {
        log(LOG_ERROR, false, "Thermocouple %d discrete inputs read failed\n", index);
    }
    if(thermocoupleIO_index.tcIO[index].bus->readInputRegisters(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, inputRegisters, 48)) {
        log(LOG_DEBUG, false, "Thermocouple %d input registers read successfully\n", index);
    } else {
        log(LOG_ERROR, false, "Thermocouple %d input registers read failed\n", index);
    }
    memcpy(&thermocoupleIO_index.tcIO[index].reg.outputState, discreteInputs, sizeof(discreteInputs));
    memcpy(&thermocoupleIO_index.tcIO[index].reg.temperature, inputRegisters, sizeof(inputRegisters));

    log(LOG_DEBUG, false, "Thermocouple 0 temperature: %.2f\n", thermocoupleIO_index.tcIO[index].reg.temperature[0]);
}

void manage_universal_in(uint8_t index) {
}

void manage_digital_io(uint8_t index) {
}

void manage_power_meter(uint8_t index) {
}
