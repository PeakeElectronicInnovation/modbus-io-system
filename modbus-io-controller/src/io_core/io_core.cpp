#include "io_core.h"
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

    // Test thermocouple board
    thermocoupleIO_index.tcIO[0].bus = &bus1;
    thermocoupleIO_index.tcIO[0].slaveID = 1;
    thermocoupleIO_index.tcIO[0].status = 0;
    thermocoupleIO_index.tcIO[0].lastUpdate = millis();
    thermocoupleIO_index.tcIO[0].pollTime = 1000;
    thermocoupleIO_index.tcIO[0].reg.slaveID = 1;
    thermocoupleIO_index.tcIO[0].reg.alertEnable[0] = true;
    thermocoupleIO_index.tcIO[0].reg.outputEnable[0] = true;    
    thermocoupleIO_index.tcIO[0].reg.alertLatch[0] = false;     // Auto-clear
    thermocoupleIO_index.tcIO[0].reg.alertEdge[0] = false;      // Alert on rising edge
    thermocoupleIO_index.tcIO[0].reg.alertSP[0] = 100.0;
    thermocoupleIO_index.tcIO[0].reg.alarmHyst[0] = 15;
    thermocoupleIO_index.tcIO[0].reg.type[0] = 0;               // K type thermocouple

    deviceIndex[0].type = THERMOCOUPLE_IO;
    deviceIndex[0].index = 0;
    deviceIndex[0].configured = true;
    log(LOG_INFO, false, "IO Core initialised, added thermocouple board for testing...\n");
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
    if (!busCfg->bus->readHoldingRegisters(245, 0, buf, 1)) return 0;

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
    if(millis() - thermocoupleIO_index.tcIO[index].lastUpdate < thermocoupleIO_index.tcIO[index].pollTime) return;
    thermocoupleIO_index.tcIO[index].lastUpdate = millis();
    bool coils[32];
    bool discreteInputs[32];
    uint16_t holdingRegisters[35];
    uint16_t inputRegisters[48];
    memcpy(coils, &thermocoupleIO_index.tcIO[index].reg, sizeof(coils));
    memcpy(holdingRegisters, &thermocoupleIO_index.tcIO[index].reg.type, 64); // Careful, slaveID is padded here, copy separately
    holdingRegisters[32] = thermocoupleIO_index.tcIO[index].slaveID;
    bool changed = false;

    // Check for changes to writable registers and write if changed
    // Coils
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

    // Holding registers
    for (int i = 0; i < 33; i++) {
        if (thermocoupleIO_index.tcIO[index].holdingRegisters[i] != holdingRegisters[i]) {
            thermocoupleIO_index.tcIO[index].holdingRegisters[i] = holdingRegisters[i];
            changed = true;
            log(LOG_INFO, true, "Thermocouple board at index %d holding register %d changed to %d\n", index, i, holdingRegisters[i]);
        }
    }
    if (changed) {
        int retries = 0;
        while (retries < 3) {
            if(thermocoupleIO_index.tcIO[index].bus->writeMultipleHoldingRegisters(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, holdingRegisters, 33)) {
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
