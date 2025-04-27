#include "board_status.h"
#include "io_core.h"
#include "board_config.h"
#include "../utils/logger.h"
#include <ArduinoJson.h>

// Temperature history is removed to save RAM

// Setup the API endpoints for board status
void setupBoardStatusAPI() {
    log(LOG_INFO, false, "Setting up board status API\n");
    
    // Add CORS pre-flight options handler for all endpoints
    server.on("/api/status", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200, "text/plain", "");
    });
    
    // Add CORS pre-flight options for reset_alarm endpoint
    server.on("/api/status/reset_alarm", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200, "text/plain", "");
    });
    
    // Add CORS pre-flight options for reset_all_alarms endpoint
    server.on("/api/status/reset_all_alarms", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200, "text/plain", "");
    });
    
    // Get status of all boards
    server.on("/api/status", HTTP_GET, handleGetAllBoardsStatus);
    
    // Get detailed status of a specific board
    server.on("/api/status/board", HTTP_GET, handleGetBoardStatus);
    
    // Get thermocouple data for a specific board
    server.on("/api/status/tc", HTTP_GET, handleGetThermocoupleData);
    
    // Reset a specific channel's latched alarm
    server.on("/api/status/reset_alarm", HTTP_GET, handleResetAlarm);
    
    // Reset all latched alarms for a board
    server.on("/api/status/reset_all_alarms", HTTP_GET, handleResetAllAlarms);
        
    log(LOG_INFO, false, "Board status API setup complete\n");
}

// Handler for getting status of all boards
void handleGetAllBoardsStatus() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Create JSON document to hold response
    const size_t capacity = JSON_ARRAY_SIZE(MAX_BOARDS) + 
                           MAX_BOARDS * JSON_OBJECT_SIZE(5); // Each board has 5 basic properties
    DynamicJsonDocument doc(capacity);
    
    // Create JSON array for boards
    JsonArray boardsArray = doc.to<JsonArray>();
    
    // Add basic status for each board
    for (uint8_t i = 0; i < boardCount; i++) {
        // Get the board configuration
        BoardConfig* config = &boardConfigs[i];
        
        // Add board basic information
        JsonObject boardObj = boardsArray.createNestedObject();
        boardObj["id"] = i;
        boardObj["name"] = config->boardName;
        boardObj["type"] = getDeviceTypeName(config->type);
        boardObj["initialised"] = config->initialised;
        boardObj["connected"] = config->connected;
    }
    
    // Return the response
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// Handler for getting detailed status of a specific board
void handleGetBoardStatus() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Check if ID parameter is provided
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
        return;
    }
    
    // Get board ID
    uint8_t boardId = server.arg("id").toInt();
    
    // Check if board exists
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Get the board configuration
    BoardConfig* config = &boardConfigs[boardId];
    
    // Create JSON document to hold response
    DynamicJsonDocument doc(4096);
    
    // Add board information
    doc["id"] = boardId;
    doc["name"] = config->boardName;
    doc["type"] = getDeviceTypeName(config->type);
    doc["initialised"] = config->initialised;
    doc["connected"] = config->connected;
    doc["slave_id"] = config->slaveID;
    doc["modbus_port"] = config->modbusPort;
    doc["poll_time"] = config->pollTime;
    
    // Add type-specific information
    switch (config->type) {
        case THERMOCOUPLE_IO: {
            // Get the thermocouple board index
            uint8_t tcIndex = config->boardIndex;
            
            // Add thermocouple-specific information
            JsonObject tcObj = doc.createNestedObject("thermocouple");
            
            // Board status information
            tcObj["status"] = thermocoupleIO_index.tcIO[tcIndex].reg.status;
            tcObj["board_type"] = thermocoupleIO_index.tcIO[tcIndex].reg.boardType;
            tcObj["last_update"] = thermocoupleIO_index.tcIO[tcIndex].lastUpdate;
            tcObj["psu_voltage"] = thermocoupleIO_index.tcIO[tcIndex].Vpsu;
            
            // Error flags
            JsonObject errorObj = tcObj.createNestedObject("errors");
            errorObj["modbus"] = thermocoupleIO_index.tcIO[tcIndex].modbusError;
            errorObj["i2c"] = thermocoupleIO_index.tcIO[tcIndex].I2CError;
            errorObj["psu"] = thermocoupleIO_index.tcIO[tcIndex].PSUError;
            
            // Channel data
            JsonArray channelsArray = tcObj.createNestedArray("channels");
            for (int ch = 0; ch < 8; ch++) {
                JsonObject channelObj = channelsArray.createNestedObject();
                channelObj["number"] = ch;
                channelObj["temperature"] = thermocoupleIO_index.tcIO[tcIndex].reg.temperature[ch];
                channelObj["cold_junction"] = thermocoupleIO_index.tcIO[tcIndex].reg.coldJunction[ch];
                channelObj["delta_junction"] = thermocoupleIO_index.tcIO[tcIndex].reg.deltaJunction[ch];
                channelObj["tc_type"] = thermocoupleIO_index.tcIO[tcIndex].reg.type[ch];
                channelObj["alert_setpoint"] = thermocoupleIO_index.tcIO[tcIndex].reg.alertSP[ch];
                channelObj["alarm_hysteresis"] = thermocoupleIO_index.tcIO[tcIndex].reg.alarmHyst[ch];
                
                // Include the channel name from board configuration
                channelObj["channel_name"] = config->settings.thermocoupleIO.channels[ch].channelName;
                
                // Channel settings
                JsonObject settingsObj = channelObj.createNestedObject("settings");
                settingsObj["alert_enable"] = thermocoupleIO_index.tcIO[tcIndex].reg.alertEnable[ch];
                settingsObj["output_enable"] = thermocoupleIO_index.tcIO[tcIndex].reg.outputEnable[ch];
                settingsObj["alert_latch"] = thermocoupleIO_index.tcIO[tcIndex].reg.alertLatch[ch];
                settingsObj["alert_edge"] = thermocoupleIO_index.tcIO[tcIndex].reg.alertEdge[ch];
                
                // Channel status
                JsonObject statusObj = channelObj.createNestedObject("status");
                statusObj["output_state"] = thermocoupleIO_index.tcIO[tcIndex].reg.outputState[ch];
                statusObj["alarm_state"] = thermocoupleIO_index.tcIO[tcIndex].reg.alarmState[ch];
                statusObj["open_circuit"] = thermocoupleIO_index.tcIO[tcIndex].reg.openCircuit[ch];
                statusObj["short_circuit"] = thermocoupleIO_index.tcIO[tcIndex].reg.shortCircuit[ch];
            }
            break;
        }
        
        // Add other board types here as they are implemented
        default:
            // No type-specific information for other board types yet
            break;
    }
    
    // Return the response
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    // Debug logging to check that doc size is appropriate - don't delete
    //log(LOG_DEBUG, false, "Board status doc size: %d bytes\n", doc.memoryUsage());
    //log(LOG_DEBUG, false, "Board status response size: %d bytes\n", response.length());
}

// Handler for getting thermocouple data for a specific board
void handleGetThermocoupleData() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Check if ID parameter is provided
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
        return;
    }
    
    // Get board ID
    uint8_t boardId = server.arg("id").toInt();
    
    // Check if board exists
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Get the board configuration
    BoardConfig* config = &boardConfigs[boardId];
    
    // Check if board is a thermocouple board
    if (config->type != THERMOCOUPLE_IO) {
        server.send(400, "application/json", "{\"error\":\"Board is not a thermocouple board\"}");
        return;
    }
    
    // Check if board is connected
    if (!config->connected) {
        server.send(400, "application/json", "{\"error\":\"Board is not connected\"}");
        return;
    }
    
    // Get the thermocouple board index
    uint8_t tcIndex = config->boardIndex;
    
    // Create JSON document to hold response
    DynamicJsonDocument doc(2048);
    
    // Add basic board information
    doc["id"] = boardId;
    doc["name"] = config->boardName;
    
    // Add temperature data
    JsonArray temperatureArray = doc.createNestedArray("temperatures");
    JsonArray alarmArray = doc.createNestedArray("alarms");
    JsonArray faultArray = doc.createNestedArray("faults");
    
    for (int ch = 0; ch < 8; ch++) {
        // Temperature data
        temperatureArray.add(thermocoupleIO_index.tcIO[tcIndex].reg.temperature[ch]);
        
        // Alarm state
        alarmArray.add(thermocoupleIO_index.tcIO[tcIndex].reg.alarmState[ch]);
        
        // Fault state (open or short circuit)
        faultArray.add(thermocoupleIO_index.tcIO[tcIndex].reg.openCircuit[ch] || 
                      thermocoupleIO_index.tcIO[tcIndex].reg.shortCircuit[ch]);
    }
    
    // Return the response
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// Handler for resetting a specific channel's latched alarm
void handleResetAlarm() {
    log(LOG_INFO, false, "handleResetAlarm\n");
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Check if ID and channel parameters are provided
    if (!server.hasArg("id") || !server.hasArg("channel")) {
        server.send(400, "application/json", "{\"error\":\"Missing id or channel parameter\"}");
        return;
    }
    
    // Get board ID and channel number
    uint8_t boardId = server.arg("id").toInt();
    uint8_t channel = server.arg("channel").toInt();
    
    // Check if board exists
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Get the board configuration
    BoardConfig* config = &boardConfigs[boardId];
    
    // Check if board is a thermocouple board
    if (config->type != THERMOCOUPLE_IO) {
        server.send(400, "application/json", "{\"error\":\"Board is not a thermocouple board\"}");
        return;
    }
    
    // Check if board is connected
    if (!config->connected) {
        server.send(400, "application/json", "{\"error\":\"Board is not connected\"}");
        return;
    }
    
    // Get the thermocouple board index
    uint8_t tcIndex = config->boardIndex;
    
    // Check if channel is valid
    if (channel < 0 || channel >= 8) {
        server.send(400, "application/json", "{\"error\":\"Invalid channel number\"}");
        return;
    }
    
    // Reset the alarm latch for the specified channel using the provided function
    bool result = thermocouple_latch_reset(tcIndex, channel);
    
    if (result) {
        // Return success response
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        // Return error response
        server.send(500, "application/json", "{\"error\":\"Failed to reset alarm\"}");
    }
}

// Handler for resetting all latched alarms for a board
void handleResetAllAlarms() {
    log(LOG_INFO, false, "handleResetAllAlarms\n");
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Check if ID parameter is provided
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
        return;
    }
    
    // Get board ID
    uint8_t boardId = server.arg("id").toInt();
    
    // Check if board exists
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Get the board configuration
    BoardConfig* config = &boardConfigs[boardId];
    
    // Check if board is a thermocouple board
    if (config->type != THERMOCOUPLE_IO) {
        server.send(400, "application/json", "{\"error\":\"Board is not a thermocouple board\"}");
        return;
    }
    
    // Check if board is connected
    if (!config->connected) {
        server.send(400, "application/json", "{\"error\":\"Board is not connected\"}");
        return;
    }
    
    // Get the thermocouple board index
    uint8_t tcIndex = config->boardIndex;
    
    // Reset all alarm latches using the provided function
    bool result = thermocouple_latch_reset_all(tcIndex);
    
    if (result) {
        // Return success response
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        // Return error response
        server.send(500, "application/json", "{\"error\":\"Failed to reset alarms\"}");
    }
}
