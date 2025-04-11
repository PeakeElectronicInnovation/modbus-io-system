#include "MCP960x.h"

// Constructor
MCP960x::MCP960x(uint8_t address, TwoWire *wire) {
    _address = address;
    _wire = wire;
}

// Initialise the MCP960x ----------------------------------------------------------------
uint8_t MCP960x::begin() {
    _wire->begin();
    uint8_t deviceID = readDeviceID(); // Read the device ID
    if (deviceID == 0) {
        return 0; // Failed to read device ID
    }
    uint8_t data = 0x04; 
    if(!writeRegister(MCP960x_REG_SENSOR_CONFIG, &data, 1)) { // Reset the device configuration
        Serial.println("Error: Failed to write sensor config to MCP960x.");
        return 0; // Failed to write to the device
    }
    uint8_t readValue;
    if(!readRegister(MCP960x_REG_SENSOR_CONFIG, &readValue, 1)) { // Read the sensor configuration
        Serial.println("Error: Failed to read sensor config from MCP960x.");
        return 0; // Failed to read from the device
    }
    data = 0x00; // Set the device configuration to default
    if(!writeRegister(MCP960x_REG_DEVICE_CONFIG, &data, 1)) { // Reset the device configuration
        Serial.println("Error: Failed to write device config to MCP960x.");
        return 0; // Failed to write to the device
    }
    if(!readRegister(MCP960x_REG_STATUS, &readValue, 1)) { // Read the device configuration
        Serial.println("Error: Failed to read status from MCP960x.");
        return 0; // Failed to read from the device
    }
    Serial.print("MCP960x status: 0x");
    Serial.println(readValue, HEX); // Print the status in HEX format
    return deviceID;
}

// Temperature read functions -----------------------------------------------------------
// Read temperature in Celsius (T∆ + cold)
float MCP960x::readTemperature() {
    uint8_t buffer[2];
    if (readRegister(MCP960x_REG_HOT_J_TEMP, buffer, 2)) {
        int16_t rawTemp = (buffer[0] << 8) | buffer[1];
        return rawTemp * 0.0625; // Convert to Celsius
    }
    Serial.println("Error: Failed to read temperature from MCP960x.");
    return 0.0; // Return 0 if read fails
}

// Read cold junction temperature in Celsius (on chip)
float MCP960x::readColdJunctionTemperature() {
    uint8_t buffer[2];
    if (readRegister(MCP960x_REG_COLD_J_TEMP, buffer, 2)) {
        int16_t rawTemp = (buffer[0] << 8) | buffer[1];
        return rawTemp * 0.0625; // Convert to Celsius
    }
    Serial.println("Error: Failed to read cold junction temperature from MCP960x.");
    return 0.0; // Return 0 if read fails
}

// Read hot junction temperature in Celsius (T∆ Junction to Cold)
float MCP960x::readDeltaTemperature() {
    uint8_t buffer[2];
    if (readRegister(MCP960x_REG_DELTA_TEMP, buffer, 2)) {
        int16_t rawTemp = (buffer[0] << 8) | buffer[1];
        return rawTemp * 0.0625; // Convert to Celsius
    }
    Serial.println("Error: Failed to read hot junction temperature from MCP960x.");
    return 0.0; // Return 0 if read fails
}

// Device status update function -------------------------------------------------------
uint8_t MCP960x::updateStatus() {
    uint8_t statusByte[1];
    if (!readRegister(MCP960x_REG_STATUS, statusByte, 1)) return 0xFF;
    status.burstComplete = (statusByte[0] >> MCP960x_STATUS_BURST_COMPLETE_bp) & 1;
    status.tempUpdated = (statusByte[0] >> MCP960x_STATUS_TEMP_UPDATED_bp) & 1;
    status.shortCircuit = (statusByte[0] >> MCP960x_STATUS_SHORT_CIRCUIT_bp) & 1;
    status.openCircuit = (statusByte[0] >> MCP960x_STATUS_OPEN_CIRCUIT_bp) & 1;
    status.alert[0] = (statusByte[0] >> MCP960x_STATUS_ALERT1_bp) & 1;
    status.alert[1] = (statusByte[0] >> MCP960x_STATUS_ALERT2_bp) & 1;
    status.alert[2] = (statusByte[0] >> MCP960x_STATUS_ALERT3_bp) & 1;
    status.alert[3] = (statusByte[0] >> MCP960x_STATUS_ALERT4_bp) & 1;
    return statusByte[0];
}

// Alert functions ---------------------------------------------------------------------
float MCP960x::readAlarmSP(uint8_t alarm) {
    if (alarm > 3) {
        Serial.println("Error: Invalid alarm number. Must be 0-3.");
        return 0.0; // Return 0 if alarm number is invalid
    }
    uint8_t buffer[2];
    if (readRegister(MCP960x_REG_ALERT1_LIMIT + alarm, buffer, 2)) {
        int16_t rawTemp = (buffer[0] << 8) | buffer[1];
        return rawTemp * 0.0625; // Convert to Celsius
    }
    Serial.println("Error: Failed to read alarm setpoint from MCP960x.");
    return 0.0; // Return 0 if read fails
}

bool MCP960x::setAlarmSP(uint8_t alarm, float temperature) {
    if (alarm > 3) {
        Serial.println("Error: Invalid alarm number. Must be 0-3.");
        return false; // Return false if alarm number is invalid
    }
    int16_t rawTemp = static_cast<int16_t>(temperature / 0.0625); // Convert to raw temperature
    uint8_t buffer[2];
    buffer[0] = rawTemp >> 8; // Convert to raw temperature
    buffer[1] = rawTemp & 0xFF; // Convert to raw temperature
    return writeRegister(MCP960x_REG_ALERT1_LIMIT + alarm, buffer, 2); // Write to the register
}

uint8_t MCP960x::readAlarmHyst(uint8_t alarm) {
    if (alarm > 3) {
        Serial.println("Error: Invalid alarm number. Must be 0-3.");
        return 0.0; // Return 0 if alarm number is invalid
    }
    uint8_t buffer;
    if (readRegister(MCP960x_REG_ALERT1_HYST + alarm, &buffer, 1)) {
        return buffer; // Return the alarm hysteresis value
    }
    Serial.println("Error: Failed to read alarm hysteresis from MCP960x.");
    return 0; // Return 0 if read fails
}

bool MCP960x::setAlarmHyst(uint8_t alarm, uint8_t temperature) {
    if (alarm > 3) {
        Serial.println("Error: Invalid alarm number. Must be 0-3.");
        return false; // Return false if alarm number is invalid
    }
    return writeRegister(MCP960x_REG_ALERT1_HYST + alarm, &temperature, 1); // Write to the register
}

bool MCP960x::enableAlert(uint8_t alert, bool enable) {
    if (alert > 3) {
        Serial.println("Error: Invalid alert number. Must be 0-3.");
        return false; // Return false if alert number is invalid
    }
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1)) {
        if (enable) {
            buffer[0] |= MCP960x_CONFIG_ALERT_ENABLE_bm; // Enable the alert
        } else {
            buffer[0] &= ~MCP960x_CONFIG_ALERT_ENABLE_bm; // Disable the alert
        }
        return writeRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1); // Write to the register
    }
    Serial.println("Error: Failed to read status from MCP960x.");
    return false; // Return false if read fails
}

bool MCP960x::latchAlert(uint8_t alert, bool latch) {
    if (alert > 3) {
        Serial.println("Error: Invalid alert number. Must be 0-3.");
        return false; // Return false if alert number is invalid
    }
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1)) {
        if (latch) {
            buffer[0] |= MCP960x_CONFIG_ALERT_LATCH_bm; // Set latch mode
        } else {
            buffer[0] &= ~MCP960x_CONFIG_ALERT_LATCH_bm; // Clear latch mode
        }
        return writeRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1); // Write to the register
    }
    Serial.println("Error: Failed to read status from MCP960x.");
    return false; // Return false if read fails
}

bool MCP960x::clearAlert(uint8_t alert) {
    if (alert > 3) {
        Serial.println("Error: Invalid alert number. Must be 0-3.");
        return false; // Return false if alert number is invalid
    }
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1)) {
        buffer[0] |= MCP960x_CONFIG_ALERT_CLEAR_bm; // Clear the alert
        return writeRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1); // Write to the register
    }
    Serial.println("Error: Failed to read status from MCP960x.");
    return false; // Return false if read fails
}

bool MCP960x::setAlartEdge(uint8_t alert, bool rising) {
    if (alert > 3) {
        Serial.println("Error: Invalid alert number. Must be 0-3.");
        return false; // Return false if alert number is invalid
    }
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1)) {
        if (rising) {
            buffer[0] |= MCP960x_CONFIG_ALERT_EDGE_bm; // Set alert edge to alarm on rise
        } else {
            buffer[0] &= ~MCP960x_CONFIG_ALERT_EDGE_bm; // Alert on falling temperature
        }
        return writeRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1); // Write to the register
    }
    Serial.println("Error: Failed to read status from MCP960x.");
    return false; // Return false if read fails
}

bool MCP960x::setAlertPolarity(uint8_t alert, bool act_high) {
    if (alert > 3) {
        Serial.println("Error: Invalid alert number. Must be 0-3.");
        return false; // Return false if alert number is invalid
    }
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1)) {
        if (act_high) {
            buffer[0] |= MCP960x_CONFIG_ALERT_POLARITY_bm; // Set alert polarity to active high
        } else {
            buffer[0] &= ~MCP960x_CONFIG_ALERT_POLARITY_bm; // Active low
        }
        return writeRegister(MCP960x_REG_ALERT1_CONFIG + alert, buffer, 1); // Write to the register
    }
    Serial.println("Error: Failed to read status from MCP960x.");
    return false; // Return false if read fails
}

// Configuration functions -------------------------------------------------------------
bool MCP960x::setType(MCP960x_type_t type) {
    if (type > 7) {
        Serial.println("Error: Invalid thermocouple type. Must be 0-7.");
        return false;
    }
    config.type = type;
    uint8_t configBuf[1];
    configBuf[0] =  (config.type << MCP960x_CONFIG_TYPE_bp) | 
                    (config.filter << MCP960x_CONFIG_FILTER_bp) | config.resolution;
    if (!writeRegister(MCP960x_REG_SENSOR_CONFIG, configBuf, 1)) return false;
}
MCP960x_type_t MCP960x::getType() {
    return config.type;
}

bool MCP960x::setFilter(uint8_t filter) {
    if (filter > 7) {
        Serial.println("Error: Invalid filter value. Must be 0-7.");
        return false;
    }
    config.filter = filter;
    uint8_t configBuf[1];
    configBuf[0] =  (config.type << MCP960x_CONFIG_TYPE_bp) | 
                    (config.filter << MCP960x_CONFIG_FILTER_bp) | config.resolution;
    if (!writeRegister(MCP960x_REG_SENSOR_CONFIG, configBuf, 1)) return false;
}
uint8_t MCP960x::getFilter() {
    return config.filter;
}

// Miscellaneous functions -------------------------------------------------------------
// Read raw ADC data
uint32_t MCP960x::readRawADC() {
    uint8_t buffer[3];
    if (readRegister(MCP960x_REG_RAW_ADC, buffer, 3)) {
        uint32_t rawADC = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
        return rawADC; // Return the raw ADC value
    }
    Serial.println("Error: Failed to read raw ADC data from MCP960x.");
    return 0; // Return 0 if read fails
}

// Read the device ID
uint8_t MCP960x::readDeviceID() {
    uint8_t buffer[1];
    if (readRegister(MCP960x_REG_DEVICE_ID, buffer, 1)) {
        return buffer[0]; // Return the device ID
    }
    return 0; // Return 0 if read fails
}

// Global configuration for initialization
bool MCP960x::setConfig() {
    uint8_t configBuf[1];

    // Set sensor configuration
    configBuf[0] =  (config.type << MCP960x_CONFIG_TYPE_bp) | 
                    (config.filter << MCP960x_CONFIG_FILTER_bp) | config.resolution;
    if (!writeRegister(MCP960x_REG_SENSOR_CONFIG, configBuf, 1)) return false;

    // Set device configuration
    configBuf[0] =  (config.shutdown << MCP960x_CONFIG_SHUTDOWN_bp) | 
                    (config.burstSamples << MCP960x_CONFIG_BURST_SAMPLES_bp) | 
                    (config.resolution << MCP960x_CONFIG_ADC_RESOLUTION_bp) | 
                    (config.cjResolution << MCP960x_CONFIG_CJ_RESOLUTION_bp);
    if (!writeRegister(MCP960x_REG_DEVICE_CONFIG, configBuf, 1)) return false;

    // Sert alert configurations
    for (int i = 0; i < 4; i++) {
        configBuf[0] =  (config.alertEnable[i] << MCP960x_CONFIG_ALERT_ENABLE_bp) | 
                        (config.alertLatch[i] << MCP960x_CONFIG_ALERT_LATCH_bp) | 
                        (config.alertPolarity[i] << MCP960x_CONFIG_ALERT_POLARITY_bp) | 
                        (config.alertEdge[i] << MCP960x_CONFIG_ALERT_EDGE_bp) | 
                        (config.alertSensor[i] << MCP960x_CONFIG_ALERT_SENSOR_bp);
        if (!writeRegister(MCP960x_REG_ALERT1_CONFIG + i, configBuf, 1)) return false;
        if (!setAlarmSP(i, config.alertSP[i])) return false;
        if (!writeRegister(MCP960x_REG_ALERT1_HYST + i, &config.alertHyst[i], 1)) return false;
    }
    return true;
}

void MCP960x::printConfig() {
    Serial.printf("--------------- Config ---------------\n");
    uint8_t configBuf[2];
    readRegister(MCP960x_REG_SENSOR_CONFIG, configBuf, 1);
    Serial.printf("Sensor Config: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_DEVICE_CONFIG, configBuf, 1);
    Serial.printf("Device Config: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT1_CONFIG, configBuf, 1);
    Serial.printf("Alert Config 1: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT2_CONFIG, configBuf, 1);
    Serial.printf("Alert Config 2: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT3_CONFIG, configBuf, 1);
    Serial.printf("Alert Config 3: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT4_CONFIG, configBuf, 1);
    Serial.printf("Alert Config 4: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT1_HYST, configBuf, 1);
    Serial.printf("Alert Hysteresis 1: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT2_HYST, configBuf, 1);
    Serial.printf("Alert Hysteresis 2: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT3_HYST, configBuf, 1);
    Serial.printf("Alert Hysteresis 3: 0x%02x\n", configBuf[0]);
    readRegister(MCP960x_REG_ALERT4_HYST, configBuf, 1);
    Serial.printf("Alert Hysteresis 4: 0x%02x\n", configBuf[0]);
    Serial.print("Alert Setpoint 1: ");
    Serial.println(readAlarmSP(0), 2);
    Serial.print("Alert Setpoint 2: ");
    Serial.println(readAlarmSP(1), 2);
    Serial.print("Alert Setpoint 3: ");
    Serial.println(readAlarmSP(2), 2);
    Serial.print("Alert Setpoint 4: ");
    Serial.println(readAlarmSP(3), 2);
    readRegister(MCP960x_REG_STATUS, configBuf, 1);
    Serial.printf("Status: 0x%02x\n", configBuf[0]);
    Serial.print("Temperature: ");
    Serial.println(readTemperature(), 2);
    Serial.println("----------------------------------------");
}

// Private functions -------------------------------------------------------------------
// Read a register from the MCP960x
bool MCP960x::readRegister(uint8_t reg, uint8_t *buffer, size_t length) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission() != 0) {
        Serial.println("Error: Failed to write to MCP960x.");
        return false; // Transmission failed
    }
    _wire->requestFrom(_address, length);
    for (size_t i = 0; i < length; i++) {
        if (_wire->available()) {
            buffer[i] = _wire->read();
            //Serial.print("Read value: 0x");
            //Serial.print(buffer[i], HEX); // Print the read value in HEX format
        } else {
            Serial.println("Error: Not enough data available from MCP960x.");
            return false; // Not enough data available
        }
    }
    return true;
}

// Write to a register on the MCP960x
bool MCP960x::writeRegister(uint8_t reg, uint8_t *buffer, size_t length) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    for (size_t i = 0; i < length; i++) {
        _wire->write(buffer[i]);
    }
    return (_wire->endTransmission() == 0); // Return true if transmission was successful
}