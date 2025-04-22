#include <Arduino.h>
#include <tinyNeoPixel.h>
#include <EEPROM.h>

#include "MCP960x.h"
#include "ModbusRTUSlave.h"

// Hardware pins
#define PIN_EN_CH1      PIN_PA0
#define PIN_EN_CH2      PIN_PA1
#define PIN_EN_CH3      PIN_PA2
#define PIN_EN_CH4      PIN_PA3
#define PIN_EN_CH5      PIN_PA4
#define PIN_EN_CH6      PIN_PA5
#define PIN_EN_CH7      PIN_PA6
#define PIN_EN_CH8      PIN_PA7

#define PIN_RS485_TX    PIN_PC0
#define PIN_RS485_RX    PIN_PC1

#define PIN_I2C_SDA     PIN_PC2
#define PIN_I2C_SCL     PIN_PC3

#define PIN_LED_DAT     PIN_PD1
#define PIN_ADDR_BTN    PIN_PD2
#define PIN_PS_FB       PIN_PD3

#define PIN_GPIO_0_TX   PIN_PD4
#define PIN_GPIO_1_RX   PIN_PD5

#define PIN_OUT_FB_1    PIN_PD6
#define PIN_OUT_FB_2    PIN_PD7
#define PIN_OUT_FB_3    PIN_PF0
#define PIN_OUT_FB_4    PIN_PF1
#define PIN_OUT_FB_5    PIN_PF2
#define PIN_OUT_FB_6    PIN_PF3
#define PIN_OUT_FB_7    PIN_PF4
#define PIN_OUT_FB_8    PIN_PF5

#define PIN_RESET       PIN_PF6
#define PIN_UPDI        PIN_PF7

// EEPROM defines (for AVR64DD32 MCU max is 256 bytes)
#define EEPROM_VALID_ADDR     0x00
#define EEPROM_VALID_VALUE    0xA5
#define EEPROM_MODBUSCFG_ADDR 0x01 // (2 bytes)
#define EEPROM_BOARDNAME_ADDR 0x03 // (14 bytes)
#define EEPROM_CONFIG_ADDR    0x20 // (8 * sizeof(tc_config_t) = 80 bytes)

// Objects
tinyNeoPixel leds = tinyNeoPixel(2, PIN_LED_DAT, NEO_GRB);

MCP960x tc[8] = {
    MCP960x(0x60),
    MCP960x(0x61),
    MCP960x(0x62),
    MCP960x(0x63),
    MCP960x(0x64),
    MCP960x(0x65),
    MCP960x(0x66),
    MCP960x(0x67)
};

ModbusRTUSlave bus(Serial1);

// Structs
struct tc_config_t {
    uint8_t type = 0; // Thermocouple type (default to K)
    float alertSP = 200.0;
    uint8_t alertHyst = 5;
    bool alertEnable = true;
    bool alertLatch = false;  // Auto clear by default
    bool alertEdge = false;   // Triggered by rising temperature by default
    bool outputEnable = true; // MCU output enable line (user controlable)
} tcConfig[8];  // Struct for EEPROM storage of thermocouple config data

// Modbus data structures
struct modbus_coil_t {      // FC01/05/15
    bool alertEnable[8];    // 0-7
    bool outputEnable[8];   // 8-15
    bool alertLatch[8];     // 16-23
    bool alertEdge[8];      // 24-31
} modbusOutSet;

struct modbus_discrete_t {  // FC02
    bool outputState[8];    // 0-7
    bool alertState[8];     // 8-15
    bool openCircuit[8];    // 16-23
    bool shortCircuit[8];   // 24-31
} modbusFlag;

struct modbus_holding_t {   // FC03/06/16
    uint16_t status = 0;    // 0
    const uint16_t boardType = 2; // 1 (0x0002 = Thermocouple IO board ID)
    char boardName[14];     // 2-8
    uint16_t slaveID = 0;   // 9
    uint16_t type[8];       // 10-17
    float alertSP[8];       // 18-33
    uint16_t alertHyst[8];  // 34-41
} modbusHolding;

struct modbus_input_t {     // FC04
    float temperature[8];   // 0-15
    float coldJunction[8];  // 16-31
    float deltaJunction[8]; // 32-47
} modbusInput;

// Modbus register arrays
bool coil[32];
bool inputDiscrete[32];
uint16_t inputReg[48];
uint16_t holdingReg[42];

int enablePin[8] = {
    PIN_EN_CH1,
    PIN_EN_CH2,
    PIN_EN_CH3,
    PIN_EN_CH4,
    PIN_EN_CH5,
    PIN_EN_CH6,
    PIN_EN_CH7,
    PIN_EN_CH8
};

int outputFBpin[8] = {
    PIN_OUT_FB_1,
    PIN_OUT_FB_2,
    PIN_OUT_FB_3,
    PIN_OUT_FB_4,
    PIN_OUT_FB_5,
    PIN_OUT_FB_6,
    PIN_OUT_FB_7,
    PIN_OUT_FB_8
};

bool modbusInitialised = false;
bool waitingForModbusConfig = false;
bool addrBtnPressed = false;
uint32_t addrBtnTime = 0;

// EEPROM write timer (allows for multiple config changes before writing to EEPROM)
bool newDataToSave = false;
uint32_t newDataTime = 0;
uint32_t saveDelay_ms = 10000; // 10 second delay for EEPROM write

// LED timing
uint32_t ledPulseTime;
uint32_t ledPulseDelay = 500;
bool ledState = 0;

// Slow loop timing
uint32_t slowLoopTime;
uint32_t slowLoopDelay = 2000;

// Status LED colours
#define LED_OFF 0x000000
#define LED_RED 0xFF0000
#define LED_GREEN 0x00FF00
#define LED_BLUE 0x0000FF
#define LED_YELLOW 0xFFFF00
#define LED_CYAN 0x00FFFF
#define LED_MAGENTA 0xFF00FF

#define LED_STARTUP LED_YELLOW
#define LED_OK  LED_GREEN
#define LED_ERROR LED_RED
#define LED_BUSY LED_BLUE
#define LED_UNCONFIGURED LED_CYAN

uint32_t statusLedColour = LED_STARTUP;
uint32_t commLedColour = LED_OFF;