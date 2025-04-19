#include "sys_init.h"

// Early initialization code that runs before main C runtime
void __attribute__((section(".init3"))) early_init(void) {
  // Set all Port A pins high immediately after reset
  PORTA.OUTSET = 0xFF;  // Set all Port A pins high
  PORTA.DIRSET = 0xFF;  // Set all Port A pins as outputs
}

void saveConfig() {
  newDataToSave = false;
  // Save Slave ID and board name to EEPROM
  EEPROM.put(EEPROM_MODBUSCFG_ADDR, modbusHolding.slaveID);
  EEPROM.put(EEPROM_BOARDNAME_ADDR, modbusHolding.boardName);
  // Save thermocouple config to EEPROM
  for(int i = 0; i < 8; i++) {
    EEPROM.put(EEPROM_CONFIG_ADDR + (i * sizeof(tc_config_t)), tcConfig[i]);
  }
}

void getConfig() {
  // EEPROM memory map -----------------------------------------
  // 0 - EEPROM Valid Value (0xAA)
  // 1 - Modbus Config (12 bytes)
  // 16 - MCP960x Config (8 * 10 bytes)
  // -----------------------------------------------------------

  // Read configuration from EEPROM
  uint8_t valid = EEPROM.read(EEPROM_VALID_ADDR);

  // Store defaults if valid byte is not set
  if (valid != EEPROM_VALID_VALUE) {
    Serial.println("Invalid EEPROM data, initializing to default values:");
    saveConfig();
    EEPROM.write(EEPROM_VALID_ADDR, EEPROM_VALID_VALUE);
    Serial.println("Default configuration saved to EEPROM.");
    return;
  }

  // Load configuration from EEPROM
  Serial.println("Loading previously saved configuration from EEPROM...");
  // Slave ID:
  EEPROM.get(EEPROM_MODBUSCFG_ADDR, modbusHolding.slaveID);
  if (modbusHolding.slaveID < 245) {
    modbusInitialised = true;
    statusLedColour = LED_OK;
    Serial.printf("Modbus slave ID: %d\n", modbusHolding.slaveID);
  } else {
    statusLedColour = LED_UNCONFIGURED;
    Serial.printf("Modbus unconfigured\n");
  }
  // Board name:
  EEPROM.get(EEPROM_BOARDNAME_ADDR, modbusHolding.boardName);
  Serial.printf("Board name: %s\n", modbusHolding.boardName);
  // Thermocouple config:
  for(int i = 0; i < 8; i++) {
    EEPROM.get(EEPROM_CONFIG_ADDR + (i * sizeof(tc_config_t)), tcConfig[i]);
  }

  // Store config to tc structs and modbus registers
  for(int i = 0; i < 8; i++) {
    tc[i].config.type = static_cast<MCP960x_type_t>(tcConfig[i].type);
    tc[i].config.alertSP[0] = tcConfig[i].alertSP;
    tc[i].config.alertHyst[0] = tcConfig[i].alertHyst;
    tc[i].config.alertEnable[0] = tcConfig[i].alertEnable;
    tc[i].config.alertLatch[0] = tcConfig[i].alertLatch;
    tc[i].config.alertEdge[0] = tcConfig[i].alertEdge;
    modbusHolding.type[i] = static_cast<uint16_t>(tcConfig[i].type);
    modbusHolding.alertSP[i] = tcConfig[i].alertSP;
    modbusHolding.alertHyst[i] = tcConfig[i].alertHyst;
    modbusOutSet.alertEnable[i] = tcConfig[i].alertEnable;
    modbusOutSet.alertLatch[i] = tcConfig[i].alertLatch;
    modbusOutSet.alertEdge[i] = tcConfig[i].alertEdge;
    modbusOutSet.outputEnable[i] = tcConfig[i].outputEnable;

    memcpy(holdingReg, &modbusHolding, sizeof(modbusHolding));
    memcpy(coil, &modbusOutSet, sizeof(modbusOutSet));

    // Debug print
    Serial.printf("Type: %d, SD: ", modbusHolding.type[i]);
    Serial.print(modbusHolding.alertSP[i]);
    Serial.printf(", Hyst: %d, Enable: %d, Latch: %d, Edge: %d, Output Enable: %d\n", modbusHolding.alertHyst[i], modbusOutSet.alertEnable[i], modbusOutSet.alertLatch[i], modbusOutSet.alertEdge[i], modbusOutSet.outputEnable[i]);
  }
}

void setupModbus() {
  pinMode(PIN_ADDR_BTN, INPUT_PULLUP);
  bus.configureCoils(coil, 32);
  bus.configureDiscreteInputs(inputDiscrete, 32);
  bus.configureHoldingRegisters(holdingReg, 42);
  bus.configureInputRegisters(inputReg, 48);
  if (modbusInitialised) {
    bus.begin(modbusHolding.slaveID, 500000);
    commLedColour = LED_OK;
  } else {
    commLedColour = LED_OFF;
    Serial.println("Modbus not configured, hold Address button for 3 seconds to start configuration.");
  }
}

void setupThermocoupleInterface() {
  Wire.pins(PIN_I2C_SDA, PIN_I2C_SCL); // Set I2C pins for Wire library
  Wire.begin(); // Initialize I2C bus
  Wire.setClock(50000);
  bool error = false;
  for (int i = 0; i < 8; i++) {
    uint8_t deviceID = tc[i].begin(); // Initialize each MCP960x thermocouple interface
    if (deviceID == 0) {
      Serial.printf("Failed to initialize MCP960x at address 0x6%x\n", i);
      error = true;
    } else {
      Serial.printf("MCP960x initialized successfully! Device at address 0x6%x has device ID: 0x%02x\n", i, deviceID);
      if (!tc[i].setConfig()) { // Set the configuration for each MCP960x
        Serial.printf("Failed to set configuration for MCP960x at address 0x6%x\n", i);
        error = true;
      } else {
        Serial.printf("Configuration set successfully for MCP960x at address 0x6%x\n", i);
        tc[i].printConfig();
      }
    }
  }
  statusLedColour = error ? LED_ERROR : modbusInitialised ? LED_OK : LED_UNCONFIGURED;

  if (error) {
    Serial.println("Error initializing MCP960x devices. Check connections and power supply.");
    leds.setPixelColor(0, LED_ERROR); // Set the first LED to error color
    leds.show(); // Update the LEDs
    delay(2000);
    _PROTECTED_WRITE(RSTCTRL.SWRR,1);
  }

  // Enable outputs
  for (int i = 0; i < 8; i++) {
    pinMode(enablePin[i], OUTPUT);
    digitalWrite(enablePin[i], !modbusOutSet.outputEnable[i]);
    Serial.printf("Out Enable %d is: %i\n", i, modbusOutSet.outputEnable[i]);
    Serial.printf("Output %d is: %s\n", i, digitalRead(enablePin[i]) ? "HIGH" : "LOW");
  }
}

void readAll() {
  if (!modbusInitialised) return;
  bool error = false;
  // Read data from all MCP9601 ICs and check output states
  for (int i = 0; i < 8; i++) {
    modbusInput.temperature[i] = tc[i].readTemperature();
    modbusInput.coldJunction[i] = tc[i].readColdJunctionTemperature();
    modbusInput.deltaJunction[i] = tc[i].readDeltaTemperature();
    if (tc[i].updateStatus() == 0xFF) error = true;
    modbusFlag.outputState[i] = digitalRead(outputFBpin[i]); // Read the output state from the corresponding pin
    modbusFlag.alertState[i] = tc[i].status.alert[0];
    modbusFlag.openCircuit[i] = tc[i].status.openCircuit;
    modbusFlag.shortCircuit[i] = tc[i].status.shortCircuit;  
  }
  statusLedColour = error ? LED_ERROR : LED_OK;
  // Copy data to modbus registers (read only)
  memcpy(inputDiscrete, &modbusFlag, sizeof(modbusFlag)); // Copy the modbusFlag struct to the inputDiscrete array
  memcpy(inputReg, &modbusInput, sizeof(modbusInput)); // Copy the modbusInput struct to the inputReg array
}

void setupLEDs() {
  // Initialize the LEDs
  leds.begin();
  leds.setBrightness(50); // Set brightness to 50%
  leds.setPixelColor(0, LED_STARTUP); // Set the first LED to the startup color
  leds.setPixelColor(1, LED_OFF); // Set the second LED to off
  leds.show(); // Update the LEDs to show the new colors
  Serial.println("LEDs initialised");
  ledPulseTime = millis() + ledPulseDelay;
}

void handleLEDs() {
  if (millis() >= ledPulseTime) {
    if (ledState) leds.setPixelColor(0, LED_OFF);
    else leds.setPixelColor(0, statusLedColour);
    ledState = !ledState;
    leds.show();
    ledPulseTime += ledPulseDelay;
  }
}

void handleModbus() {
  if (!modbusInitialised  && !waitingForModbusConfig) return;
  int FC = bus.poll();
  if (FC == 0) {
    if (!waitingForModbusConfig) commLedColour = LED_OK; // No Modbus request received, set LED to OK color
    return; // No Modbus request received
  }
  commLedColour = LED_BUSY;
  leds.setPixelColor(1, commLedColour);
  leds.show();
  Serial.printf("Modbus request recieved, function code: %d\n", FC);

  bool changed = false;

  // Handle update to coils
  if (FC == MODBUS_FC05_WRITE_SINGLE_COIL || FC == MODBUS_FC15_WRITE_MULTIPLE_COILS) {
    modbus_coil_t coilData;
    memcpy(&coilData, coil, sizeof(modbus_coil_t)); // Copy the response buffer to the coilData struct
    for (int i = 0; i < 8; i++) {
      if (coilData.outputEnable[i] != modbusOutSet.outputEnable[i]) {
        changed = true;
        modbusOutSet.outputEnable[i] = coilData.outputEnable[i];
        tcConfig[i].outputEnable = coilData.outputEnable[i];
        Serial.printf("Out Enable %d is: %i\n", i, modbusOutSet.outputEnable[i]);
        digitalWrite(enablePin[i], !modbusOutSet.outputEnable[i]);  // LOW = enabled
      }
      if (coilData.alertEnable[i] != modbusOutSet.alertEnable[i]) {
        changed = true;
        modbusOutSet.alertEnable[i] = coilData.alertEnable[i];
        tcConfig[i].alertEnable = coilData.alertEnable[i];
        Serial.printf("alert Enable %d is: %i\n", i, modbusOutSet.alertEnable[i]);
        tc[i].enableAlert(0, modbusOutSet.alertEnable[i]);          // 1 = enabled
      }
      if (coilData.alertLatch[i] != modbusOutSet.alertLatch[i]) {
        changed = true;
        modbusOutSet.alertLatch[i] = coilData.alertLatch[i];
        tcConfig[i].alertLatch = coilData.alertLatch[i];
        Serial.printf("alert Latch %d is: %i\n", i, modbusOutSet.alertLatch[i]);
        tc[i].latchAlert(0, modbusOutSet.alertLatch[i]);            // 1 = latch, 0 = auto clear (@T_alert - hysteresis value)
      }
      if (coilData.alertEdge[i] != modbusOutSet.alertEdge[i]) {
        changed = true;
        modbusOutSet.alertEdge[i] = coilData.alertEdge[i];
        tcConfig[i].alertEdge = coilData.alertEdge[i];
        Serial.printf("alert Edge %d is: %i\n", i, modbusOutSet.alertEdge[i]);
        tc[i].setAlartEdge(0, modbusOutSet.alertEdge[i]);           // 1 = alert on falling temp, 0 = alert on rising temp
      }
    }
  }

  // Handle update to holding registers
  if (FC == MODBUS_FC06_WRITE_SINGLE_REGISTER || FC == MODBUS_FC16_WRITE_MULTIPLE_REGISTERS) {
    modbus_holding_t holdingData;
    memcpy(&holdingData, holdingReg, sizeof(modbus_holding_t));
    for (int i = 0; i < 8; i++) {
      if ((holdingData.type[i] <= 7) && (holdingData.type[i] != modbusHolding.type[i])) {
        changed = true;
        Serial.printf("Thermocouple %d type changed to %d\n", i, holdingData.type[i]);
        modbusHolding.type[i] = holdingData.type[i];
        tcConfig[i].type = holdingData.type[i];
        tc[i].setType(static_cast<MCP960x_type_t>(modbusHolding.type[i])); // Set the configuration for each MCP960x
        changed = true;
      }
      if ((holdingData.alertSP[i] < 1500.0) && (holdingData.alertSP[i] > -40.0) && (holdingData.alertSP[i] != modbusHolding.alertSP[i])) {
        changed = true;
        Serial.printf("Thermocouple %d SP changed to ", i);
        Serial.println(holdingData.alertSP[i]);
        modbusHolding.alertSP[i] = holdingData.alertSP[i];
        tcConfig[i].alertSP = holdingData.alertSP[i];
        tc[i].setAlertSP(0, modbusHolding.alertSP[i]);
        changed = true;
      }
      if (holdingData.alertHyst[i] != modbusHolding.alertHyst[i]) {
        changed = true;
        Serial.printf("Thermocouple %d Hyst changed to %d\n", i, holdingData.alertHyst[i]);
        modbusHolding.alertHyst[i] = holdingData.alertHyst[i];
        tcConfig[i].alertHyst = holdingData.alertHyst[i];
        tc[i].setAlertHyst(0, modbusHolding.alertHyst[i]);
        changed = true;
      }
      if ((holdingData.slaveID != modbusHolding.slaveID) && (holdingData.slaveID > 0)) {
        if ((holdingData.slaveID > 244) || (holdingData.slaveID < 1)) {
          modbusHolding.slaveID = 245;
          modbusInitialised = false;
          statusLedColour = LED_UNCONFIGURED;
          Serial.printf("Slave ID reset, Modbus unconfigured\n");
        } else {
          modbusHolding.slaveID = holdingData.slaveID;
          Serial.printf("Modbus slave ID changed to %d\n", modbusHolding.slaveID);
          bus.begin(modbusHolding.slaveID, 500000);
          modbusInitialised = true;
        }
        if (waitingForModbusConfig) {
          waitingForModbusConfig = false;
          Serial.println("Modbus configuration complete!");
        }
        changed = true;
      }        
    }
  }
  if (changed) {
    newDataTime = millis();
    newDataToSave = true;
  }
  leds.setPixelColor(1, LED_OFF);
  leds.show();
}

void handleAddrBtn() {
  // Check if button not pressed
  if (digitalRead(PIN_ADDR_BTN)) {
    if (addrBtnPressed) {
      addrBtnPressed = false;
      addrBtnTime = 0;
      return;
    }
  }
  // Ignore if already waiting for Modbus configuration
  if (waitingForModbusConfig) return;
  if (!digitalRead(PIN_ADDR_BTN)) {
    if (!addrBtnPressed) {
      addrBtnPressed = true;
      addrBtnTime = millis();
      return;
    }
    if (addrBtnPressed && ((millis() - addrBtnTime) < 2000)) return;
    waitingForModbusConfig = true;
    bus.begin(245, 500000); // Set Modbus address to 245
    leds.setPixelColor(1, LED_UNCONFIGURED);
    leds.show();
    Serial.println("Modbus address set to 245, waiting for configuration.");
  }
}

void saveHandler() {
  if (newDataToSave && ((millis() - newDataTime) > saveDelay_ms)) {
    newDataToSave = false;
    Serial.print("Saving configuration to EEPROM... ");
    saveConfig();
    Serial.println("Done!");
  }
}

void setup() {
  Serial.pins(PIN_GPIO_0_TX, PIN_GPIO_1_RX); // Set RX and TX pins for Serial communication
  Serial.begin(115200);
  Serial.println("Testing AVR64DD32...");

  setupLEDs();

  Serial.printf("Loading configuration from EEPROM...\n");
  getConfig(); // Load configuration from EEPROM

  Serial.printf("Starting Modbus interface...\n");
  setupModbus(); // Setup Modbus interface

  Serial.printf("Starting thermocouple interface 1...\n");
  setupThermocoupleInterface();
  readAll();

  Serial.println("Done!");
  slowLoopTime = millis() + slowLoopDelay;
}

void loop() {
  readAll();
  handleModbus();
  handleLEDs();
  handleAddrBtn();
  saveHandler();
  /*if (millis() >= slowLoopTime) {
    tc[0].printConfig(); // Print the configuration of the first thermocouple
    slowLoopTime += slowLoopDelay;
  }*/
}