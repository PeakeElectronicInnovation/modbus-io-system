#include "statusManager.h"
#include "../io_core/board_config.h"

// Object definition
Adafruit_NeoPixel leds(4, PIN_LED_DAT, NEO_GRB + NEO_KHZ800);

// Status variables
StatusVariables status;
bool statusLocked = false;
static bool blinkState = false;
static uint32_t ledTS = 0;

void init_statusManager() {
  leds.begin();
  leds.setBrightness(50);
  leds.fill(LED_COLOR_OFF, 0, 4);
  leds.setPixelColor(LED_SYSTEM_STATUS, LED_STATUS_STARTUP);
  leds.show();
  status.ledPulseTS = millis();
  log(LOG_INFO, false, "Status manager started\n");
  ledTS = millis();
}

void manageStatus(void)
{
  if (millis() - ledTS < LED_UPDATE_PERIOD) return;
  if (statusLocked) return;
  statusLocked = true;
  
  // Check for status change and update LED colours accordingly
  if (status.updated) {
    // System status LED colours
    if (!status.rtcOK) {
      status.LEDcolour[LED_SYSTEM_STATUS] = LED_STATUS_ERROR;
    } else if (!status.psuOK || !status.sdCardOK) {
      status.LEDcolour[LED_SYSTEM_STATUS] = LED_STATUS_WARNING;
    } else {
      status.LEDcolour[LED_SYSTEM_STATUS] = LED_STATUS_OK;
    }
    // Modbus status LED colours
    if (status.modbusBusy) {
      status.LEDcolour[LED_MODBUS_STATUS] = LED_STATUS_BUSY;
    } else if (hasOfflineBoards()) {
      status.LEDcolour[LED_MODBUS_STATUS] = LED_STATUS_WARNING; // Amber for offline boards
    } else if (status.modbusConnected) {
      status.LEDcolour[LED_MODBUS_STATUS] = LED_STATUS_OK;
    } else {
      status.LEDcolour[LED_MODBUS_STATUS] = LED_STATUS_OFF;
    }
    leds.setPixelColor(LED_MODBUS_STATUS, status.LEDcolour[LED_MODBUS_STATUS]);
    // Reset the update flag
    status.updated = false;
  }  
  // Status LED blink updater  
  if (millis() - status.ledPulseTS >= LED_BLINK_PERIOD) {
    blinkState = !blinkState;
    status.ledPulseTS += LED_BLINK_PERIOD;
    if (blinkState) {
      leds.setPixelColor(LED_SYSTEM_STATUS, status.LEDcolour[LED_SYSTEM_STATUS]);
    } else {
      leds.setPixelColor(LED_SYSTEM_STATUS, LED_COLOR_OFF);
    }
    leds.show();
  }
  statusLocked = false;
}

// Check if any initialised boards are offline
bool hasOfflineBoards(void) {
  for (uint8_t i = 0; i < boardCount; i++) {
    BoardConfig* board = &boardConfigs[i];
    // Only check boards that have been initialised (assigned Modbus address)
    if (board->initialised && !board->connected) {
      return true;
    }
  }
  return false;
}