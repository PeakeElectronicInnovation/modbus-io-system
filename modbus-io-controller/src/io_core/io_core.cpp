#include "io_core.h"
#include "io_objects.h"
#include "board_config.h"
#include "board_status.h"
#include "dashboard_config.h"


// Object definitions
ModbusRTUMaster bus1(Serial1);
ModbusRTUMaster bus2(Serial2);

modbusConfig_t modbusConfig[2] = {
    {&bus1, {false}},
    {&bus2, {false}}
};

deviceIndex_t deviceIndex[64];
thermocoupleIO_index_t thermocoupleIO_index;

void init_io_core(void) {
    Serial1.setTX(PIN_RS485_TX_1);
    Serial1.setRX(PIN_RS485_RX_1);
    Serial2.setTX(PIN_RS485_TX_2);
    Serial2.setRX(PIN_RS485_RX_2);
    Serial1.setFIFOSize(128);
    Serial2.setFIFOSize(128);
    bus1.begin(500000);
    bus2.begin(500000);

    pinMode(PIN_ALM_LED, OUTPUT);
    pinMode(PIN_ALM_SOUNDER, OUTPUT);
    digitalWrite(PIN_ALM_LED, LOW);
    digitalWrite(PIN_ALM_SOUNDER, LOW);

    // Initialise board configuration
    init_board_config();

    // Apply saved board configurations
    apply_board_configs();

    // Setup board status API
    setupBoardStatusAPI();

    // Setup dashboard configuration API
    init_dashboard_config();

    log(LOG_INFO, false, "IO Core initialised\n");
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
        
        // Skip boards that haven't been initialised with a Modbus address
        if (!board->initialised) {
            log(LOG_INFO, false, "Skipping board %d (%s) - not yet initialised\n", 
                i, board->boardName);
            continue;
        }

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
    thermocoupleIO_index.tcIO[config->boardIndex].lastUpdate = millis();
    thermocoupleIO_index.tcIO[config->boardIndex].pollTime = config->pollTime;
    thermocoupleIO_index.tcIO[config->boardIndex].reg.slaveID = config->slaveID;
    strcpy(thermocoupleIO_index.tcIO[config->boardIndex].reg.boardName, config->boardName);

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
            // Get the board configuration for this device
            BoardConfig* board = getBoard(deviceIndex[i].index);
            
            // Skip devices that haven't been properly initialised
            if (!board || !board->initialised) {
                continue;
            }
            switch (deviceIndex[i].type) {
                case ANALOGUE_DIGITAL_IO:
                    manage_analogue_digital_io(deviceIndex[i].index);
                    break;
                case THERMOCOUPLE_IO:
                    manage_thermocouple(deviceIndex[i].index);
                    leds.setPixelColor(LED_MODBUS_STATUS, status.LEDcolour[LED_MODBUS_STATUS]);
                    break;
                case RTD_IO:
                    manage_rtd(deviceIndex[i].index);
                    break;
                case ENERGY_METER:
                    manage_energy_meter(deviceIndex[i].index);
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
            if (busCfg->bus->writeSingleHoldingRegister(245, EXP_HOLDING_REG_SLAVE_ID, i)) {
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

void setSlaveIDInUse(uint8_t slaveID, uint8_t modbusPort) {
    if (modbusPort >= 2) {
        log(LOG_ERROR, true, "Invalid Modbus port %d\n", modbusPort);
        return;
    }
    if (slaveID < 1 || slaveID > 247) {
        log(LOG_ERROR, true, "Invalid slave ID %d\n", slaveID);
        return;
    }
    modbusConfig[modbusPort].idAssigned[slaveID] = true;
}

// Analogue digital IO board management functions -------------------------->
void manage_analogue_digital_io(uint8_t index) {
}

// Thermocouple IO board management functions ------------------------------>
void manage_thermocouple(uint8_t index) {
    if (!thermocoupleIO_index.tcIO[index].configInitialised) {
        if (!setup_thermocouple(index)) {
            log(LOG_ERROR, true, "Failed to setup thermocouple board configuration registers at index %d\n", index);
            getBoard(index)->initialised = false;
            return;
        }
        thermocoupleIO_index.tcIO[index].configInitialised = true;
        
        if (!statusLocked) {
            statusLocked = true;
            status.modbusConnected = true;
            status.updated = true;
            statusLocked = false;
        }
        return;
    }
    // Check if polling time has elapsed
    if(millis() - thermocoupleIO_index.tcIO[index].lastUpdate < thermocoupleIO_index.tcIO[index].pollTime) {
        record_thermocouple(index); // Check if record interval has elapsed before returning
        return;
    }
    thermocoupleIO_index.tcIO[index].lastUpdate = millis();

    leds.setPixelColor(LED_MODBUS_STATUS, LED_STATUS_BUSY);
    leds.show();

    // Check board is online
    uint16_t buf[1];
    int retries = 0;
    while (retries < 3) {
        if (!thermocoupleIO_index.tcIO[index].bus->readHoldingRegisters(thermocoupleIO_index.tcIO[index].slaveID, EXP_HOLDING_REG_BOARD_TYPE, buf, 1)) {
            retries++;
            continue;
        } else break;
    }

    if (retries == 3) {
        if (getBoard(index)->connected == true) {
            log(LOG_ERROR, true, "Board at index %d is offline\n", index);
            getBoard(index)->connected = false;
        }
        return;
    }
    
    // Ensure board is a thermocouple board (type 2)
    if (deviceType_t(buf[0]) != THERMOCOUPLE_IO) {
        log(LOG_ERROR, true, "Board at index %d is not a thermocouple board. Type: %d, %s\n", index, buf[0], getDeviceTypeName(deviceType_t(buf[0])));
        return;
    }
    if (!getBoard(index)->connected) {
        getBoard(index)->connected = true;
        log(LOG_INFO, true, "Board at index %d is online\n", index);
    }

    // Get board status
    retries = 0;
    while (retries < 3) {
        if (!thermocoupleIO_index.tcIO[index].bus->readHoldingRegisters(thermocoupleIO_index.tcIO[index].slaveID, EXP_HOLDING_REG_STATUS, buf, 1)) {
            retries++;
        } else break;
    }
    if (retries == 3) {
        log(LOG_ERROR, true, "Failed to read board status\n");
        return;
    }
    thermocoupleIO_index.tcIO[index].modbusError = buf[0] & 0x01;
    thermocoupleIO_index.tcIO[index].I2CError = (buf[0] >> 1) & 0x01;
    thermocoupleIO_index.tcIO[index].PSUError = (buf[0] >> 2) & 0x01;
    thermocoupleIO_index.tcIO[index].Vpsu = static_cast<float>(buf[0] >> 4) / 10.0f;

    // Register buffers
    bool coils[32];
    bool discreteInputs[32];
    uint16_t holdingRegisters[40];
    uint16_t inputRegisters[48];

    // Load Coil and Holding Register buffers with current config values
    memcpy(coils, &thermocoupleIO_index.tcIO[index].reg, sizeof(coils));
    memcpy(holdingRegisters, &thermocoupleIO_index.tcIO[index].reg.boardName, sizeof(holdingRegisters));
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
        retries = 0;
        while (retries < 3) {
            if(thermocoupleIO_index.tcIO[index].bus->writeMultipleCoils(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, coils, 32)) {
                log(LOG_INFO, true, "Thermocouple board at index %d coils written successfully\n", index);
                break;
            } else {
                log(LOG_ERROR, true, "Thermocouple board at index %d coils write failed\n", index);
            }
            retries++;
            delay(100); // Wait before retrying
        }
        if (retries == 3) {
            log(LOG_ERROR, true, "Thermocouple board at index %d coils write failed after 3 retries\n", index);
            getBoard(index)->connected = false;
            thermocoupleIO_index.tcIO[index].configInitialised = false;
            return;
        }
        changed = false;
    }

    // Holding registers ----->
    for (int i = 0; i < 40; i++) {
        if (i == EXP_HOLDING_REG_BOARD_TYPE) continue; // Skip board type register (read only!)
        if (thermocoupleIO_index.tcIO[index].holdingRegisters[i] != holdingRegisters[i]) {
            thermocoupleIO_index.tcIO[index].holdingRegisters[i] = holdingRegisters[i];
            changed = true;
            log(LOG_INFO, true, "Thermocouple board at index %d holding register %d changed to %d\n", index, i, holdingRegisters[i]);
        }
    }
    if (changed) {
        retries = 0;
        while (retries < 3) {
            if(thermocoupleIO_index.tcIO[index].bus->writeMultipleHoldingRegisters(thermocoupleIO_index.tcIO[index].slaveID, EXP_HOLDING_REG_BOARD_NAME, holdingRegisters, 40)) {
                log(LOG_INFO, true, "Holding registers written successfully\n");
                break;
            } else {
                log(LOG_ERROR, true, "Holding registers write failed, retrying...\n");
                retries++;
                delay(100); // Wait before retrying
            }
        } if (retries == 3) {
            log(LOG_ERROR, true, "Thermocouple board at index %d holding registers write failed after 3 retries\n", index);
            getBoard(index)->connected = false;
            thermocoupleIO_index.tcIO[index].configInitialised = false;
            return;
        }
    }

    // Read discrete inputs
    retries = 0;
    while (retries < 3) {
        if(!thermocoupleIO_index.tcIO[index].bus->readDiscreteInputs(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, discreteInputs, 32)) {
            retries++;
        } else break;
    }
    if (retries == 3) {
        log(LOG_ERROR, true, "Thermocouple board at index %d discrete inputs read failed after 3 retries\n", index);
        getBoard(index)->connected = false;
        return;
    }
    memcpy(&thermocoupleIO_index.tcIO[index].reg.outputState, discreteInputs, sizeof(discreteInputs));

    // Read input registers
    retries = 0;
    while (retries < 3) {
        if(!thermocoupleIO_index.tcIO[index].bus->readInputRegisters(thermocoupleIO_index.tcIO[index].slaveID, 0x0000, inputRegisters, 48)) {
            retries++;
        } else break;
    }
    if (retries == 3) {
        log(LOG_ERROR, true, "Thermocouple IO board index %d input registers read failed after 3 retries\n", index);
        getBoard(index)->connected = false;
        return;
    }
    memcpy(&thermocoupleIO_index.tcIO[index].reg.temperature, inputRegisters, sizeof(inputRegisters));

    // Handle monitored faults and alarms
    handle_faults_and_alarms();

    // Record temperature data if record interval has elapsed
    record_thermocouple(index);
}

bool setup_thermocouple(uint8_t index) {
    // Add a small delay before configuring the board to ensure bus is ready
    // This is particularly important for the second board during startup
    delay(100);
    
    uint16_t holdingRegisters[40];
    bool coils[32];
    memcpy(holdingRegisters, &thermocoupleIO_index.tcIO[index].reg.boardName, sizeof(holdingRegisters));
    memcpy(coils, &thermocoupleIO_index.tcIO[index].reg, sizeof(coils));

    // Retry mechanism for holding registers
    int retries = 0;
    bool success = false;
    while (retries < 3 && !success) {
        // Attempt to load holding registers (excluding first 2 registers)
        success = thermocoupleIO_index.tcIO[index].bus->writeMultipleHoldingRegisters(
            thermocoupleIO_index.tcIO[index].slaveID, 
            EXP_HOLDING_REG_BOARD_NAME, 
            holdingRegisters, 
            40);
            
        if (!success) {
            log(LOG_WARNING, true, "Failed to write holding registers to board %d (attempt %d/3)\n", 
                index, retries + 1);
            retries++;
            delay(200); // Increasing delay between retries
        }
    }
    
    if (!success) {
        log(LOG_ERROR, true, "Failed to write holding register config to thermocouple IO board at index %d\n", index);
        return false;
    }
    
    // Copy holding registers to local buffer
    memcpy(thermocoupleIO_index.tcIO[index].holdingRegisters, holdingRegisters, sizeof(holdingRegisters));

    // Reset retry counter for coils
    retries = 0;
    success = false;
    
    // Retry mechanism for coils
    while (retries < 3 && !success) {
        // Attempt to load coils
        success = thermocoupleIO_index.tcIO[index].bus->writeMultipleCoils(
            thermocoupleIO_index.tcIO[index].slaveID, 
            0x0000, 
            coils, 
            32);
            
        if (!success) {
            log(LOG_WARNING, true, "Failed to write coils to board %d (attempt %d/3)\n", 
                index, retries + 1);
            retries++;
            delay(200); // Increasing delay between retries
        }
    }
    
    if (!success) {
        log(LOG_ERROR, true, "Failed to write coil register config to thermocouple IO board at index %d\n", index);
        return false;
    }
    
    // Copy coils to local buffer
    memcpy(thermocoupleIO_index.tcIO[index].coils, coils, sizeof(coils));
    return true;
}

bool thermocouple_latch_reset(uint8_t index, uint8_t channel) {
    if (index >= boardCount) return false;
    if (channel > 8) return false;

    return thermocoupleIO_index.tcIO[index].bus->writeSingleCoil(thermocoupleIO_index.tcIO[index].slaveID, channel + TCIO_COIL_LATCH_RESET_PTR, true);
}    

bool thermocouple_latch_reset_all(uint8_t index) {
    if (index >= boardCount) return false;

    bool buf[8] = {true, true, true, true, true, true, true, true};
    return thermocoupleIO_index.tcIO[index].bus->writeMultipleCoils(thermocoupleIO_index.tcIO[index].slaveID, TCIO_COIL_LATCH_RESET_PTR, buf, 8);
}

bool record_thermocouple(uint8_t index) {
    if (thermocoupleIO_index.tcIO[index].recordInterval != getBoard(index)->recordInterval) {
        thermocoupleIO_index.tcIO[index].recordInterval = getBoard(index)->recordInterval;
        if (thermocoupleIO_index.tcIO[index].recordInterval < 15000) {
            return false;
        }
    }
    if (millis() - thermocoupleIO_index.tcIO[index].lastRecord < thermocoupleIO_index.tcIO[index].recordInterval) return false;
    if (!thermocoupleIO_index.tcIO[index].configInitialised) return false;
    if (index >= boardCount) return false;
    if (!sdInfo.ready) return false;

    // Check for a change in settings since last record
    bool changed = false;
    for (int i = 0; i < 8; i++) {
        if (thermocoupleIO_index.tcIO[index].recordTemperature[i] != getBoard(index)->settings.thermocoupleIO.channels[i].recordTemperature) {
            thermocoupleIO_index.tcIO[index].recordTemperature[i] = getBoard(index)->settings.thermocoupleIO.channels[i].recordTemperature;
            changed = true;
        }
        if (thermocoupleIO_index.tcIO[index].recordColdJunction[i] != getBoard(index)->settings.thermocoupleIO.channels[i].recordColdJunction) {
            thermocoupleIO_index.tcIO[index].recordColdJunction[i] = getBoard(index)->settings.thermocoupleIO.channels[i].recordColdJunction;
            changed = true;
        }
        if (thermocoupleIO_index.tcIO[index].recordStatus[i] != getBoard(index)->settings.thermocoupleIO.channels[i].recordStatus) {
            thermocoupleIO_index.tcIO[index].recordStatus[i] = getBoard(index)->settings.thermocoupleIO.channels[i].recordStatus;
            changed = true;
        }
    }

    char fileName[40];
    snprintf(fileName, sizeof(fileName), "%s - ID %d sensor records", getBoard(index)->boardName, index);
    char dataString[500] = {0};

    bool record = false;

    if (changed) { // Build new header string
        char buf[25];
        for (int i = 0; i < 8; i++) {
            if (thermocoupleIO_index.tcIO[index].recordTemperature[i]) {
                snprintf(buf, sizeof(buf), ",Ch %d Temp", i+1);
                strcat(dataString, buf);
                record = true;
            }
            if (thermocoupleIO_index.tcIO[index].recordColdJunction[i]) {
                snprintf(buf, sizeof(buf), ",Ch %d ColdJ", i+1);
                strcat(dataString, buf);
                record = true;
            }
            if (thermocoupleIO_index.tcIO[index].recordStatus[i]) {
                snprintf(buf, sizeof(buf), ",Ch %d Alarm,Ch %d Out", i+1, i+1);
                strcat(dataString, buf);
                record = true;
            }
        }
        if (record) {
            strcat(dataString, "\n");
            writeSensorData(dataString, fileName, true);
        }
    } else {
        char buf[10];
        for (int i = 0; i < 8; i++) {
            if (thermocoupleIO_index.tcIO[index].recordTemperature[i]) {
                snprintf(buf, sizeof(buf), ",%0.2f", thermocoupleIO_index.tcIO[index].reg.temperature[i]);
                strcat(dataString, buf);
                record = true;
            }
            if (thermocoupleIO_index.tcIO[index].recordColdJunction[i]) {
                snprintf(buf, sizeof(buf), ",%0.2f", thermocoupleIO_index.tcIO[index].reg.coldJunction[i]);
                strcat(dataString, buf);
                record = true;
            }
            if (thermocoupleIO_index.tcIO[index].recordStatus[i]) {
                bool alarm = thermocoupleIO_index.tcIO[index].reg.alarmState[i] | thermocoupleIO_index.tcIO[index].reg.openCircuit[i] | thermocoupleIO_index.tcIO[index].reg.shortCircuit[i];
                bool output = thermocoupleIO_index.tcIO[index].reg.outputState[i];
                snprintf(buf, sizeof(buf), ",%d,%d", alarm, output);
                strcat(dataString, buf);
                record = true;
            }
        }
        if (record) {
            strcat(dataString, "\n");
            writeSensorData(dataString, fileName, false);
        }
    }

    // Update timestamp and return success
    thermocoupleIO_index.tcIO[index].lastRecord = millis();
    return true;
}

// RTD board management functions -------------------------------------------->
void manage_rtd(uint8_t index) {
}

// Energy meter board management functions ------------------------------------>
void manage_energy_meter(uint8_t index) {
}

// Fault and alarm handling functions ----------------------------------------->
void handle_faults_and_alarms(void) {
    bool globalFault = false;
    bool globalAlarm = false;

    static bool globalFaultState = false;
    static bool globalAlarmState = false;

    // Check for faults and alarms on all boards
    for (int i = 0; i < boardCount; i++) {
        if (getBoard(i)->connected && getBoard(i)->initialised) {
            // Check for faults and alarms by board type
            switch (getBoard(i)->type) {
                case THERMOCOUPLE_IO: // ----------------------->
                    for (int j = 0; j < 8; j++) {
                        // Check for alarm monitoring enabled and alarm state
                        if (getBoard(i)->settings.thermocoupleIO.channels[j].monitorAlarm) {
                            if (thermocoupleIO_index.tcIO[i].reg.alarmState[j]) {
                                globalAlarm = true;
                                break;
                            }
                        }

                        // Check for fault monitoring enabled and fault state
                        if (getBoard(i)->settings.thermocoupleIO.channels[j].monitorFault) {
                            if (thermocoupleIO_index.tcIO[i].reg.openCircuit[j] || thermocoupleIO_index.tcIO[i].reg.shortCircuit[j]) {
                                globalFault = true;
                                break;
                            }
                        }
                    }
                    
                    break; // <---------------------------------|
                // Add more board types as needed
                default:
                    break;
            }
        }
    }

    if (globalAlarm != globalAlarmState) {
        globalAlarmState = globalAlarm;
        if (globalAlarm) {
            log(LOG_WARNING, true, "Global alarm state is now active\n");
        } else {
            log(LOG_INFO, true, "Global alarm cleared\n");
        }
    }

    if (globalFault != globalFaultState) {
        globalFaultState = globalFault;
        if (globalFault) {
            log(LOG_WARNING, true, "Global fault state is now active\n");
        } else {
            log(LOG_INFO, true, "Global fault cleared\n");
        }
    }

    
    digitalWrite(PIN_ALM_LED, globalFault | globalAlarm);
    digitalWrite(PIN_ALM_SOUNDER, globalAlarm);
}

// Terminal functions --------------------------------------------------------->
void print_board_config(uint8_t index) {
    if (index >= boardCount) {
        log(LOG_ERROR, false, "Invalid board index: %d\n", index);
        return;
    }
    log(LOG_INFO, false, "Board at index %d configured as %s\n", index, getDeviceTypeName(getBoard(index)->type));
    log(LOG_INFO, false, "Board name: %s\n", getBoard(index)->boardName);
    log(LOG_INFO, false, "Slave ID: %d\n", getBoard(index)->slaveID);
    log(LOG_INFO, false, "Modbus port: %d\n", getBoard(index)->modbusPort);
    log(LOG_INFO, false, "Poll time: %d\n", getBoard(index)->pollTime);
    log(LOG_INFO, false, "Initialised: %s\n", getBoard(index)->initialised ? "Yes" : "No");
    log(LOG_INFO, false, "Connected: %s\n", getBoard(index)->connected ? "Yes" : "No");
}