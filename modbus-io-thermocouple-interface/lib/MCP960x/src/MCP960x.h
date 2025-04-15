#pragma once

#include <Arduino.h>
#include <Wire.h>

// MCP960x register addresses
#define MCP960x_REG_HOT_J_TEMP       0x00  // Thermocouple Hot-Junction Temperature
#define MCP960x_REG_DELTA_TEMP       0x01  // Junctions Temperature Delta
#define MCP960x_REG_COLD_J_TEMP      0x02  // Cold-Junction Temperature
#define MCP960x_REG_RAW_ADC          0x03  // Raw ADC Data
#define MCP960x_REG_STATUS           0x04  // STATUS register
#define MCP960x_REG_SENSOR_CONFIG    0x05  // Thermocouple Sensor Configuration
#define MCP960x_REG_DEVICE_CONFIG    0x06  // Device Configuration
#define MCP960x_REG_ALERT1_CONFIG    0x08  // Alert 1 Configuration
#define MCP960x_REG_ALERT2_CONFIG    0x09  // Alert 2 Configuration
#define MCP960x_REG_ALERT3_CONFIG    0x0A  // Alert 3 Configuration
#define MCP960x_REG_ALERT4_CONFIG    0x0B  // Alert 4 Configuration
#define MCP960x_REG_ALERT1_HYST      0x0C  // Alert 1 Hysteresis
#define MCP960x_REG_ALERT2_HYST      0x0D  // Alert 2 Hysteresis
#define MCP960x_REG_ALERT3_HYST      0x0E  // Alert 3 Hysteresis
#define MCP960x_REG_ALERT4_HYST      0x0F  // Alert 4 Hysteresis
#define MCP960x_REG_ALERT1_LIMIT     0x10  // Temperature Alert 1 Limit
#define MCP960x_REG_ALERT2_LIMIT     0x11  // Temperature Alert 2 Limit
#define MCP960x_REG_ALERT3_LIMIT     0x12  // Temperature Alert 3 Limit
#define MCP960x_REG_ALERT4_LIMIT     0x13  // Temperature Alert 4 Limit
#define MCP960x_REG_DEVICE_ID        0x20  // Device ID/Revision

// MCP960x configuration bit positions --------------------
// Status register
#define MCP960x_STATUS_ALERT1_bp         0  // Alert 1 Status
#define MCP960x_STATUS_ALERT2_bp         1  // Alert 2 Status
#define MCP960x_STATUS_ALERT3_bp         2  // Alert 3 Status
#define MCP960x_STATUS_ALERT4_bp         3  // Alert 4 Status
#define MCP960x_STATUS_OPEN_CIRCUIT_bp   4  // Open Circuit Detected
#define MCP960x_STATUS_SHORT_CIRCUIT_bp  5  // Short Circuit Detected
#define MCP960x_STATUS_TEMP_UPDATED_bp   6  // Temperature Updated
#define MCP960x_STATUS_BURST_COMPLETE_bp 7  // Burst Complete

// Sensor config register
#define MCP960x_CONFIG_FILTER_bp            0  // Filter Value (0-7)
#define MCP960x_CONFIG_TYPE_bp              4  // Thermocouple Type

// Device config register
#define MCP960x_CONFIG_SHUTDOWN_bp          0  // 2-bit Shutdown mode
#define MCP960x_CONFIG_BURST_SAMPLES_bp     2  // 3-bit Burst Mode Samples
#define MCP960x_CONFIG_ADC_RESOLUTION_bp    5  // 2-bit ADC Resolution
#define MCP960x_CONFIG_CJ_RESOLUTION_bp     7  // 1-bit Cold Junction Resolution

// Alert config registers (x4)
#define MCP960x_CONFIG_ALERT_ENABLE_bp      0  // Alert Enable
#define MCP960x_CONFIG_ALERT_LATCH_bp       1  // Alert Latch Enable
#define MCP960x_CONFIG_ALERT_POLARITY_bp    2  // Alert Polarity
#define MCP960x_CONFIG_ALERT_EDGE_bp        3  // Alert Edge Select
#define MCP960x_CONFIG_ALERT_SENSOR_bp      4  // Alert Sensor Select (0 = Hot, 1 = Cold)
#define MCP960x_CONFIG_ALERT_CLEAR_bp       7  // Alert Clear

// MCP960x configuration bit masks --------------------
// Alert config registers (x4)
#define MCP960x_CONFIG_ALERT_ENABLE_bm     0x01  // Alert Enable
#define MCP960x_CONFIG_ALERT_LATCH_bm      0x02  // Alert Latch Enable
#define MCP960x_CONFIG_ALERT_POLARITY_bm   0x04  // Alert Polarity
#define MCP960x_CONFIG_ALERT_EDGE_bm       0x08  // Alert Edge Select
#define MCP960x_CONFIG_ALERT_MODE_bm       0x10  // Alert Mode
#define MCP960x_CONFIG_ALERT_CLEAR_bm      0x80  // Alert Clear

// Enum
enum MCP960x_type_t {
    TYPE_K = 0x00,
    TYPE_J = 0x01,
    TYPE_T = 0x02,
    TYPE_N = 0x03,
    TYPE_S = 0x04,
    TYPE_E = 0x05,
    TYPE_B = 0x06,
    TYPE_R = 0x07
};

enum MCP960x_shutdown_t {
    SHUTDOWN_NORMAL = 0x00,
    SHUTDOWN_BURST = 0x01,
    SHUTDOWN_SHUTDOWN = 0x02
};

enum MCP960x_burst_samples_t {
    BURST_SAMPLES_1 = 0x00,
    BURST_SAMPLES_2 = 0x01,
    BURST_SAMPLES_4 = 0x02,
    BURST_SAMPLES_8 = 0x03,
    BURST_SAMPLES_16 = 0x04,
    BURST_SAMPLES_32 = 0x05,
    BURST_SAMPLES_64 = 0x06,
    BURST_SAMPLES_128 = 0x07
};

enum MCP960x_cj_resolution_t {
    CJ_RESOLUTION_PT0625 = 0x00,
    CJ_RESOLUTION_PT25 = 0x01
};

enum MCP960x_resolution_t {
    RESOLUTION_18_bit = 0x00,
    RESOLUTION_16_bit = 0x01,
    RESOLUTION_14_bit = 0x02,
    RESOLUTION_12_bit = 0x03
};

class MCP960x {
    public:
        // Constructor
        MCP960x(uint8_t address = 0x60, TwoWire *wire = &Wire);

        // Initialize the MCP960x
        uint8_t begin();

        // Operational functions
        float readTemperature(); // Read temperature in Celsius (T∆ + cold)
        float readColdJunctionTemperature(); // Read cold junction temperature in Celsius (on chip)
        float readDeltaTemperature(); // Read hot junction temperature in Celsius (T∆ Junction to Cold)

        // Status
        struct MCP960x_status_t {
            bool burstComplete = 0;
            bool tempUpdated = 0;
            bool shortCircuit = 0;
            bool openCircuit = 0;
            bool alert[4] = {0, 0, 0, 0};
        } status;

        uint8_t updateStatus(); // Update the status structure

        // Alerts
        float readAlertSP(uint8_t alert); // Read alert setpoint in Celsius
        bool setAlertSP(uint8_t alert, float value); // Set alert setpoint in Celsius
        uint8_t readAlertHyst(uint8_t alert); // Read alert hysteresis in Celsius
        bool setAlertHyst(uint8_t alert, uint8_t value); // Set alert hysteresis in Celsius

        bool enableAlert(uint8_t alert, bool enable); // Enable alert (0-3)
        bool latchAlert(uint8_t alert, bool latch); // Set alert mode (0-3) (0 = comparator/auto clear, 1 = interrupt/latch)
        bool clearAlert(uint8_t alert); // Clear alert (0-3)
        bool setAlartEdge(uint8_t alert, bool rising); // Set alert edge (0-3) (0 = alert on fall, 1 = alert on rise)
        bool setAlertPolarity(uint8_t alert, bool act_high); // Set alert polarity (0-3) (0 = active low, 1 = active high)
        
        // Configuration
        bool setType(MCP960x_type_t type); // Set the thermocouple type
        MCP960x_type_t getType(); // Get the thermocouple type

        bool setFilter(uint8_t filter); // Set the filter value (0-7)
        uint8_t getFilter(); // Get the filter value

        bool setADCresolution(MCP960x_resolution_t resolution); // Set the ADC resolution (0-3)
        MCP960x_resolution_t getADCresolution(); // Get the ADC resolution

        uint8_t readDeviceID(); // Read device ID

        uint32_t readRawADC(); // Read raw ADC data

        struct MCP960x_Config_t {
            uint8_t filter = 3; // Filter value (0-7)
            MCP960x_type_t type = TYPE_K; // Thermocouple type (K, J, T, N, S, R, B)
            MCP960x_shutdown_t shutdown = SHUTDOWN_NORMAL; // Shutdown mode (continuous, burst, shutdown)
            MCP960x_burst_samples_t burstSamples = BURST_SAMPLES_1; // Burst mode samples (1, 2, 4, 8, 16, 32, 64 or 128)
            MCP960x_resolution_t resolution = RESOLUTION_18_bit; // ADC resolution (18, 16, 14 or 12 bits)
            MCP960x_cj_resolution_t cjResolution = CJ_RESOLUTION_PT0625; // Cold junction resolution (0.0625 or 0.25 degrees)
            float alertSP[4] = {200, 0, 0, 0}; // Alert setpoint in Celsius (0-3)
            uint8_t alertHyst[4] = {5, 0, 0, 0};
            bool alertEnable[4] = {true, 0, 0, 0};
            bool alertLatch[4] = {false, 0, 0, 0};
            bool alertPolarity[4] = {HIGH, HIGH, HIGH, HIGH}; // 0 = active low, 1 = active high
            bool alertEdge[4] = {LOW, LOW, LOW, LOW}; // HIGH = falling temperature, LOW = rising temperature
            bool alertSensor[4] = {0, 0, 0, 0}; // 0 = hot junction, 1 = cold junction
        } config;

        bool setConfig();

        void printConfig();

        bool readRegister(uint8_t reg, uint8_t *buffer, size_t length);
        bool writeRegister(uint8_t reg, uint8_t *buffer, size_t length);

    private:
        TwoWire *_wire; // Pointer to the I2C bus
        uint8_t _address; // I2C address of the MCP960x
};
