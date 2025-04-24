#include "terminalManager.h"

bool terminalReady = false;

void init_terminalManager(void) {
  while (!serialReady) {
    delay(10);
  }
  terminalReady = true;
  log(LOG_INFO, false, "Terminal task started\n");
}

void manageTerminal(void)
{
  if (serialLocked || !terminalReady) return;
  serialLocked = true;
  if (Serial.available())
  {
    char serialString[10];  // Buffer for incoming serial data
    memset(serialString, 0, sizeof(serialString));
    int bytesRead = Serial.readBytesUntil('\n', serialString, sizeof(serialString) - 1); // Leave room for null terminator
    serialLocked = false;
    if (bytesRead > 0 ) {
      serialString[bytesRead] = '\0'; // Add null terminator
      log(LOG_INFO, true,"Received:  %s\n", serialString);

      // Reboot ---------------------------------------------->
      if (strcmp(serialString, "reboot") == 0) {
        log(LOG_INFO, true, "Rebooting now...\n");
        rp2040.restart();
      }

      // IP Address ------------------------------------------>
      else if (strcmp(serialString, "ip") == 0) {
        printNetConfig(networkConfig);
      }

      // SD Card --------------------------------------------->
      else if (strcmp(serialString, "sd") == 0) {
        log(LOG_INFO, false, "Getting SD card info...\n");
        printSDInfo();
      }

      // Status ---------------------------------------------->
      else if (strcmp(serialString, "status") == 0) {
        log(LOG_INFO, false, "Getting status...\n");
        if (statusLocked) {
          log(LOG_INFO, false, "Status is locked\n");
        } else {
          statusLocked = true;
          log(LOG_INFO, false, "24V supply %0.1fV status: %s\n", status.Vpsu, status.psuOK ? "OK" : "OUT OF RANGE");
          log(LOG_INFO, false, "RTC status: %s\n", status.rtcOK ? "OK" : "ERROR");
          log(LOG_INFO, false, "Modbus status: %s\n", status.modbusConnected ? "CONNECTED" : "DOWN");
          log(LOG_INFO, false, "Webserver status: %s\n", status.webserverUp ? "OK" : "DOWN");
          statusLocked = false;
        }
      }

      // Assign modbus address ---------------------------------->
      else if (strcmp(serialString, "assign") == 0) {
        log(LOG_INFO, false, "Attempting to assign modbus address on bus 1\n");
        uint8_t address = assign_address(&modbusConfig[0]);
        if (address) {
          log(LOG_INFO, false, "Assigned address %d on bus 1\n", address);
        } else {
          log(LOG_ERROR, false, "Failed to assign address on bus 1\n");
        }
      }

      // Print board configurations ----------------------------->
      else if (strncmp(serialString, "config -", 8) == 0) {
        // Extract the number after "config -"
        int x = atoi(serialString + 8); // Skip over "config -" and convert the rest to int
      
        log(LOG_INFO, false, "Getting board configuration for index %d...\n", x);
        print_board_config(x);
      }

      // Reset thermocouple latches ---------------------------->
      else if (strncmp(serialString, "almrst -", 8) == 0) {
        // Extract the number after "config -"
        int x = atoi(serialString + 8); // Skip over "config -" and convert the rest to int
      
        log(LOG_INFO, false, "Resetting thermocouple latches for board index %d...\n", x);
        thermocouple_latch_reset_all(x);
      }
      else {
        log(LOG_INFO, false, "Unknown command: %s\n", serialString);
        log(LOG_INFO, false, "Available commands: \n\t- ip \t\t(print IP address)\n\t- sd \t\t(print SD card info)\n\t- status\n\t- assign \t(assign modbus address)\n\t- config -x\t(print board x configuration)\n\t- almrst -x\t(reset thermocouple latches for board x)\n\t- reboot\n");
      }
    }
    // Clear the serial buffer each loop.
    serialLocked = true;
    while(Serial.available()) Serial.read();
    serialLocked = false;
  }
  serialLocked = false;
}
