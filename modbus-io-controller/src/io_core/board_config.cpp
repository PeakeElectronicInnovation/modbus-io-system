#include "board_config.h"
#include "io_core.h"
#include <WebServer.h>

// Global variables
BoardConfig boardConfigs[MAX_BOARDS];
uint8_t boardCount = 0;

// Initialise board configuration
void init_board_config(void) {
    // Load existing configuration from LittleFS
    if (!loadBoardConfig()) {
        // If configuration doesn't exist or is invalid, initialise with defaults
        log(LOG_INFO, false, "Invalid board configuration, starting with empty configuration\n");
        boardCount = 0;
        saveBoardConfig();
    }
    
    // Set up API endpoints
    setupBoardConfigAPI();
}

// Load board configuration from LittleFS
bool loadBoardConfig() {
    // Use binary format for loading
    return loadBoardConfigBinary();
}

// Save board configuration to LittleFS
bool saveBoardConfig() {
    // Use binary format for saving
    return saveBoardConfigBinary();
}

// Set up API endpoints for board configuration
void setupBoardConfigAPI() {
    log(LOG_INFO, false, "Setting up board configuration API\n");
    // Global OPTIONS handler for CORS preflight requests
    server.on("/api/boards", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
    
    // Register API endpoints with CORS headers
    server.on("/api/boards", HTTP_GET, handleGetBoardConfig);
    server.on("/api/boards", HTTP_POST, handleAddBoard);
    server.on("/api/boards", HTTP_PUT, handleUpdateBoard);
    server.on("/api/boards/delete", HTTP_GET, handleDeleteBoard);
    
    // Also handle OPTIONS for the /api/boards/all endpoint
    server.on("/api/boards/all", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
    
    server.on("/api/boards/all", HTTP_GET, handleGetAllBoards);
    
    // Also handle OPTIONS for the /api/boards/delete endpoint
    server.on("/api/boards/delete", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, DELETE, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
    
    // Register the initialise endpoint
    log(LOG_INFO, false, "Registering /api/boards/initialise endpoint\n");
    
    // Also handle OPTIONS for the /api/boards/initialise endpoint
    server.on("/api/boards/initialise", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
    
    server.on("/api/boards/initialise", HTTP_GET, handleInitialiseBoard);
    
    // Register export endpoint
    server.on("/api/boards/export", HTTP_GET, handleExportConfig);
    
    // Register import endpoint - with CORS headers
    server.on("/api/boards/import", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(200);
    });
    
    // For file upload, we need to define both the handler for the endpoint and the file upload handler
    server.on("/api/boards/import", HTTP_POST, []() {
        // This handler will be called after the file upload is complete
        server.send(200, "text/plain", "Configuration imported successfully");
    }, handleImportConfig);
    
    log(LOG_INFO, false, "Board API setup complete\n");
}

void handleGetBoardConfig() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    // If there are no boards, return an empty array
    if (boardCount == 0) {
        server.send(200, "application/json", "{\"boards\":[]}");
        return;
    }
    
    // Create JSON response with smaller buffer (was 4096, now optimized)
    DynamicJsonDocument doc(2048);
    JsonArray boards = doc.createNestedArray("boards");
    
    // Add each board to the JSON response
    for (uint8_t i = 0; i < boardCount; i++) {
        JsonObject board = boards.createNestedObject();
        
        // Add common board properties
        board["id"] = i; // Board ID for frontend reference
        board["name"] = boardConfigs[i].boardName;
        board["type"] = boardConfigs[i].type;
        board["type_name"] = getDeviceTypeName(boardConfigs[i].type);
        board["board_index"] = boardConfigs[i].boardIndex;
        board["slave_id"] = boardConfigs[i].slaveID;
        board["modbus_port"] = boardConfigs[i].modbusPort;
        board["poll_time"] = boardConfigs[i].pollTime;
        board["record_interval"] = boardConfigs[i].recordInterval;
        board["initialised"] = boardConfigs[i].initialised; // Add initialization status
        board["connected"] = boardConfigs[i].connected; // Add connection status
        
        // Add board-specific settings based on type
        if (boardConfigs[i].type == THERMOCOUPLE_IO) {
            JsonArray channels = board.createNestedArray("channels");
            
            for (int j = 0; j < 8; j++) {
                JsonObject channel = channels.createNestedObject();
                
                channel["alert_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEnable;
                channel["output_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].outputEnable;
                channel["alert_latch"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertLatch;
                channel["alert_edge"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEdge;
                channel["tc_type"] = boardConfigs[i].settings.thermocoupleIO.channels[j].tcType;
                channel["alert_setpoint"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertSetpoint;
                channel["alert_hysteresis"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertHysteresis;
                channel["channel_name"] = boardConfigs[i].settings.thermocoupleIO.channels[j].channelName;
                channel["record_temperature"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordTemperature;
                channel["record_cold_junction"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordColdJunction;
                channel["record_status"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordStatus;
                channel["show_on_dashboard"] = boardConfigs[i].settings.thermocoupleIO.channels[j].showOnDashboard;
                channel["monitor_fault"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorFault;
                channel["monitor_alarm"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorAlarm;
            }
        }
        // Add more board types as needed
    }
    
    // Debug the response
    String debugResponse;
    serializeJson(doc, debugResponse);
    
    // Send response
    String response;
    serializeJson(doc, response);
    log(LOG_DEBUG, false, "handleGetBoardConfig API response size: %d, doc size: %d\n", response.length(), doc.memoryUsage());
    server.send(200, "application/json", response);
}

void handleAddBoard() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Handle OPTIONS preflight request
    if (server.method() == HTTP_OPTIONS) {
        server.send(200);
        return;
    }
    
    // Debug request info
    String requestData = server.arg("plain");
    log(LOG_INFO, false, "Add board request body length: %d\n", requestData.length());
    if (requestData.length() > 0) {
        // Only print part of the request to avoid log overflow
        String truncated = requestData.substring(0, min(200, (int)requestData.length()));
        log(LOG_INFO, false, "Add board request (truncated): %s\n", truncated.c_str());
    } else {
        log(LOG_WARNING, true, "Empty request body\n");
        server.send(400, "application/json", "{\"error\":\"Empty request body\"}");
        return;
    }
    
    // Check if we've reached the maximum number of boards
    if (boardCount >= MAX_BOARDS) {
        log(LOG_WARNING, true, "Maximum number of boards reached\n");
        server.send(400, "application/json", "{\"error\":\"Maximum number of boards reached\"}");
        return;
    }
    
    // Parse JSON request with increased buffer size for full board configs
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, requestData);
    
    if (error) {
        log(LOG_WARNING, true, "JSON parse error: %s\n", error.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON: " + String(error.c_str()) + "\"}");
        return;
    }
    
    // Get board data from JSON
    // Note: When frontend sends numeric types as strings, ArduinoJson
    // automatically converts them to the appropriate types
    const char* name = doc["name"] | "Unnamed";
    int type = doc["type"] | 0;
    int modbusPort = doc["modbus_port"] | 0;
    int pollTime = doc["poll_time"] | 15000;
    int recordInterval = doc["record_interval"] | 15000;
    
    log(LOG_INFO, true, "Adding board: %s, type: %d, port: %d, poll: %d, record: %d\n", 
        name, type, modbusPort, pollTime, recordInterval);
    
    // Create new board config at the end of the array
    BoardConfig newBoard;
    strlcpy(newBoard.boardName, name, MAX_BOARD_NAME_LENGTH);
    newBoard.type = (deviceType_t)type;
    newBoard.boardIndex = boardCount; // Use boardCount as index
    newBoard.slaveID = 1; // Default slave ID
    newBoard.modbusPort = modbusPort;
    newBoard.pollTime = pollTime;
    newBoard.recordInterval = recordInterval;
    newBoard.initialised = false; // New boards are not initialised by default
    newBoard.connected = false; // New boards are not connected by default
    
    // Handle board type specific settings
    if (type == THERMOCOUPLE_IO) {
        JsonArray channels = doc["channels"];
        if (channels) {
            int channelIndex = 0;
            for (JsonObject channel : channels) {
                if (channelIndex >= 8) break;
                
                newBoard.settings.thermocoupleIO.channels[channelIndex].alertEnable = channel["alert_enable"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].outputEnable = channel["output_enable"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].alertLatch = channel["alert_latch"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].alertEdge = channel["alert_edge"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].tcType = channel["tc_type"] | 0;
                newBoard.settings.thermocoupleIO.channels[channelIndex].alertSetpoint = channel["alert_setpoint"] | 0.0f;
                newBoard.settings.thermocoupleIO.channels[channelIndex].alertHysteresis = channel["alert_hysteresis"] | 0;
                strlcpy(newBoard.settings.thermocoupleIO.channels[channelIndex].channelName, channel["channel_name"] | "", 33);
                newBoard.settings.thermocoupleIO.channels[channelIndex].recordTemperature = channel["record_temperature"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].recordColdJunction = channel["record_cold_junction"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].recordStatus = channel["record_status"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].showOnDashboard = channel["show_on_dashboard"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].monitorFault = channel["monitor_fault"] | false;
                newBoard.settings.thermocoupleIO.channels[channelIndex].monitorAlarm = channel["monitor_alarm"] | false;
                
                channelIndex++;
            }
        } else {
            log(LOG_WARNING, true, "No channels array found for THERMOCOUPLE_IO board\n");
            // Initialise with default values
            for (int j = 0; j < 8; j++) {
                newBoard.settings.thermocoupleIO.channels[j].alertEnable = false;
                newBoard.settings.thermocoupleIO.channels[j].outputEnable = false;
                newBoard.settings.thermocoupleIO.channels[j].alertLatch = false;
                newBoard.settings.thermocoupleIO.channels[j].alertEdge = false;
                newBoard.settings.thermocoupleIO.channels[j].tcType = 0;
                newBoard.settings.thermocoupleIO.channels[j].alertSetpoint = 0.0f;
                newBoard.settings.thermocoupleIO.channels[j].alertHysteresis = 0;
                strlcpy(newBoard.settings.thermocoupleIO.channels[j].channelName, "", 33);
                newBoard.settings.thermocoupleIO.channels[j].recordTemperature = false;
                newBoard.settings.thermocoupleIO.channels[j].recordColdJunction = false;
                newBoard.settings.thermocoupleIO.channels[j].recordStatus = false;
                newBoard.settings.thermocoupleIO.channels[j].showOnDashboard = false;
                newBoard.settings.thermocoupleIO.channels[j].monitorFault = false;
                newBoard.settings.thermocoupleIO.channels[j].monitorAlarm = false;
            }
        }
    }

    log(LOG_DEBUG, false, "handleAddBoard API doc size: %d\n", doc.memoryUsage());
    
    // Add the new board
    if (addBoard(newBoard)) {
        // Apply the board configuration immediately
        apply_board_configs();
        log(LOG_INFO, false, "Applied board configurations\n");
        
        // Create response with the new board
        DynamicJsonDocument responseDoc(1024);
        responseDoc["success"] = true;
        responseDoc["message"] = "Board added successfully";
        responseDoc["id"] = boardCount - 1; // ID of the newly added board
        
        String response;
        serializeJson(responseDoc, response);
        server.send(200, "application/json", response);
        
        log(LOG_INFO, true, "Board added successfully, new count: %d\n", boardCount);
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
    }
}

void handleUpdateBoard() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "PUT, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Handle OPTIONS preflight request
    if (server.method() == HTTP_OPTIONS) {
        server.send(200);
        return;
    }
    
    // Debug request info
    String requestData = server.arg("plain");
    log(LOG_INFO, false, "Update board request body length: %d\n", requestData.length());
    if (requestData.length() > 0) {
        // Only print part of the request to avoid log overflow
        String truncated = requestData.substring(0, min(200, (int)requestData.length()));
        log(LOG_INFO, false, "Update board request (truncated): %s\n", truncated.c_str());
    } else {
        log(LOG_WARNING, true, "Empty request body\n");
        server.send(400, "application/json", "{\"error\":\"Empty request body\"}");
        return;
    }
    
    // Parse request body with increased buffer size for full board configs
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, requestData);
    
    if (error) {
        log(LOG_WARNING, true, "JSON parse error: %s\n", error.c_str());
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // Get board ID
    uint8_t boardId = doc["id"] | 255;
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Create updated board config (preserve board index and slave ID)
    BoardConfig updatedBoard = boardConfigs[boardId];
    
    // Update common board properties (except index and slave ID which are assigned by system)
    if (doc.containsKey("name")) {
        strlcpy(updatedBoard.boardName, doc["name"], MAX_BOARD_NAME_LENGTH);
    }
    
    if (doc.containsKey("modbus_port")) {
        updatedBoard.modbusPort = doc["modbus_port"];
    }
    
    if (doc.containsKey("poll_time")) {
        uint32_t pollTime = doc["poll_time"];
        if (pollTime < 1000) pollTime = 1000; // Minimum 1 second
        if (pollTime > 3600000) pollTime = 3600000; // Maximum 1 hour
        updatedBoard.pollTime = pollTime;
    }

    if (doc.containsKey("record_interval")) {
        uint32_t recordInterval = doc["record_interval"];
        if (recordInterval < 15000) recordInterval = 15000; // Minimum 15 seconds
        if (recordInterval > 3600000) recordInterval = 3600000; // Maximum 1 hour
        updatedBoard.recordInterval = recordInterval;
    }
    
    // Update board-specific settings based on type
    if (updatedBoard.type == THERMOCOUPLE_IO && doc.containsKey("channels")) {
        JsonArray channels = doc["channels"];
        int channelIndex = 0;
        
        for (JsonObject channel : channels) {
            if (channelIndex >= 8) break;
            
            if (channel.containsKey("alert_enable")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].alertEnable = channel["alert_enable"];
            }
            
            if (channel.containsKey("output_enable")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].outputEnable = channel["output_enable"];
            }
            
            if (channel.containsKey("alert_latch")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].alertLatch = channel["alert_latch"];
            }
            
            if (channel.containsKey("alert_edge")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].alertEdge = channel["alert_edge"];
            }
            
            if (channel.containsKey("tc_type")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].tcType = channel["tc_type"];
            }
            
            if (channel.containsKey("alert_setpoint")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].alertSetpoint = channel["alert_setpoint"];
            }
            
            if (channel.containsKey("alert_hysteresis")) {
                // Ensure hysteresis is within range (0-255)
                uint16_t hysteresis = channel["alert_hysteresis"];
                if (hysteresis > 255) hysteresis = 255;
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].alertHysteresis = (uint8_t)hysteresis;
            }

            if (channel.containsKey("channel_name")) {
                strlcpy(updatedBoard.settings.thermocoupleIO.channels[channelIndex].channelName, channel["channel_name"] | "", 33);
            }

            if (channel.containsKey("record_temperature")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].recordTemperature = channel["record_temperature"];
            }
            
            if (channel.containsKey("record_cold_junction")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].recordColdJunction = channel["record_cold_junction"];
            }
            
            if (channel.containsKey("record_status")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].recordStatus = channel["record_status"];
            }
            
            if (channel.containsKey("show_on_dashboard")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].showOnDashboard = channel["show_on_dashboard"];
            }
            
            if (channel.containsKey("monitor_fault")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].monitorFault = channel["monitor_fault"];
            }
            
            if (channel.containsKey("monitor_alarm")) {
                updatedBoard.settings.thermocoupleIO.channels[channelIndex].monitorAlarm = channel["monitor_alarm"];
            }
            
            channelIndex++;
        }
    }
    // Add more board types as needed

    log(LOG_DEBUG, false, "handleUpdateBoard API doc size: %d\n", doc.memoryUsage());
    
    // Update board
    if (updateBoard(boardId, updatedBoard)) {
        // Apply the board configuration immediately
        apply_board_configs();
        log(LOG_INFO, true, "Applied updated board configurations\n");
        
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Board updated successfully\"}");
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to update board\"}");
    }
}

void handleDeleteBoard() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Handle OPTIONS preflight request
    if (server.method() == HTTP_OPTIONS) {
        server.send(200);
        return;
    }
    
    // Get board ID from request parameter
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Missing board ID\"}");
        return;
    }
    
    uint8_t boardId = server.arg("id").toInt();
    log(LOG_INFO, true, "Delete board request for ID: %d\n", boardId);
    
    if (boardId >= boardCount) {
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Delete board
    if (deleteBoard(boardId)) {
        // Apply the updated board configuration immediately
        apply_board_configs();
        log(LOG_INFO, true, "Applied board configurations after deletion\n");
        
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Board deleted successfully\"}");
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to delete board\"}");
    }
}

void handleGetAllBoards() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // If there are no boards, return an empty array
    if (boardCount == 0) {
        server.send(200, "application/json", "{\"boards\":[]}");
        return;
    }
    
    // Create JSON response
    DynamicJsonDocument doc(32768);
    JsonArray boards = doc.createNestedArray("boards");
    
    // Add each board to the JSON response
    for (uint8_t i = 0; i < boardCount; i++) {
        JsonObject board = boards.createNestedObject();
        
        // Add common board properties
        board["id"] = i; // Board ID for frontend reference
        board["name"] = boardConfigs[i].boardName;
        board["type"] = boardConfigs[i].type;
        board["type_name"] = getDeviceTypeName(boardConfigs[i].type);
        board["board_index"] = boardConfigs[i].boardIndex;
        board["slave_id"] = boardConfigs[i].slaveID;
        board["modbus_port"] = boardConfigs[i].modbusPort;
        board["poll_time"] = boardConfigs[i].pollTime;
        board["record_interval"] = boardConfigs[i].recordInterval;
        board["initialised"] = boardConfigs[i].initialised; // Add initialization status
        board["connected"] = boardConfigs[i].connected; // Add connection status
        
        // Add board-specific settings based on type
        if (boardConfigs[i].type == THERMOCOUPLE_IO) {
            JsonArray channels = board.createNestedArray("channels");
            
            for (int j = 0; j < 8; j++) {
                JsonObject channel = channels.createNestedObject();
                
                channel["alert_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEnable;
                channel["output_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].outputEnable;
                channel["alert_latch"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertLatch;
                channel["alert_edge"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEdge;
                channel["tc_type"] = boardConfigs[i].settings.thermocoupleIO.channels[j].tcType;
                channel["alert_setpoint"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertSetpoint;
                channel["alert_hysteresis"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertHysteresis;
                channel["channel_name"] = boardConfigs[i].settings.thermocoupleIO.channels[j].channelName;
                channel["record_temperature"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordTemperature;
                channel["record_cold_junction"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordColdJunction;
                channel["record_status"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordStatus;
                channel["show_on_dashboard"] = boardConfigs[i].settings.thermocoupleIO.channels[j].showOnDashboard;
                channel["monitor_fault"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorFault;
                channel["monitor_alarm"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorAlarm;
            }
        }
        // Add more board types as needed
    }
    
    // Debug the response
    String debugResponse;
    serializeJson(doc, debugResponse);
    
    // Send response
    String response;
    serializeJson(doc, response);
    log(LOG_DEBUG, false, "handleGetAllBoards API response size: %d, doc size: %d\n", response.length(), doc.memoryUsage());
    server.send(200, "application/json", response);
}

void handleExportConfig() {
    log(LOG_INFO, true, "Exporting board configuration from binary storage\n");
    
    // Check if we have any boards to export
    if (boardCount == 0) {
        server.send(200, "application/json", "{\"boards\":[]}");
        log(LOG_INFO, true, "No boards to export\n");
        return;
    }
    
    // Create JSON response using the same logic as handleGetBoardConfig
    DynamicJsonDocument doc(32768);
    JsonArray boards = doc.createNestedArray("boards");
    
    // Add each board to the JSON response
    for (uint8_t i = 0; i < boardCount; i++) {
        JsonObject board = boards.createNestedObject();
        
        // Add common board properties
        board["id"] = i; // Board ID for frontend reference
        board["name"] = boardConfigs[i].boardName;
        board["type"] = boardConfigs[i].type;
        board["type_name"] = getDeviceTypeName(boardConfigs[i].type);
        board["board_index"] = boardConfigs[i].boardIndex;
        board["slave_id"] = boardConfigs[i].slaveID;
        board["modbus_port"] = boardConfigs[i].modbusPort;
        board["poll_time"] = boardConfigs[i].pollTime;
        board["record_interval"] = boardConfigs[i].recordInterval;
        board["initialised"] = boardConfigs[i].initialised;
        board["connected"] = boardConfigs[i].connected;
        
        // Add board-specific settings based on type
        if (boardConfigs[i].type == THERMOCOUPLE_IO) {
            JsonArray channels = board.createNestedArray("channels");
            
            for (int j = 0; j < 8; j++) {
                JsonObject channel = channels.createNestedObject();
                
                // Add channel properties
                channel["alert_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEnable;
                channel["output_enable"] = boardConfigs[i].settings.thermocoupleIO.channels[j].outputEnable;
                channel["alert_latch"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertLatch;
                channel["alert_edge"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertEdge;
                channel["tc_type"] = boardConfigs[i].settings.thermocoupleIO.channels[j].tcType;
                channel["alert_setpoint"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertSetpoint;
                channel["alert_hysteresis"] = boardConfigs[i].settings.thermocoupleIO.channels[j].alertHysteresis;
                channel["channel_name"] = boardConfigs[i].settings.thermocoupleIO.channels[j].channelName;
                channel["record_temperature"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordTemperature;
                channel["record_cold_junction"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordColdJunction;
                channel["record_status"] = boardConfigs[i].settings.thermocoupleIO.channels[j].recordStatus;
                channel["show_on_dashboard"] = boardConfigs[i].settings.thermocoupleIO.channels[j].showOnDashboard;
                channel["monitor_fault"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorFault;
                channel["monitor_alarm"] = boardConfigs[i].settings.thermocoupleIO.channels[j].monitorAlarm;
            }
        }
    }
    
    // Convert JSON to string
    String jsonString;
    serializeJsonPretty(doc, jsonString);
    
    // Set headers for file download
    server.sendHeader("Content-Disposition", "attachment; filename=\"board_config.json\"");
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Content-Length", String(jsonString.length()));
    
    // Send JSON content
    server.send(200, "application/json", jsonString);
    
    log(LOG_INFO, true, "Board configuration exported successfully (%d boards)\n", boardCount);
}

void handleImportConfig() {
    static File configFile;
    static bool uploadSuccess = false;
    static String importMode = "overwrite"; // Default to overwrite mode
    
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        log(LOG_INFO, true, "Upload started: %s\n", upload.filename.c_str());
        
        // Reset import mode to default (will be set properly in UPLOAD_FILE_END)
        importMode = "overwrite";
        
        // Check if LittleFS is mounted
        if (!LittleFS.begin()) {
            log(LOG_WARNING, true, "Failed to mount LittleFS\n");
            return;
        }
        
        // Create/Open the file for writing
        configFile = LittleFS.open(BOARD_CONFIG_FILENAME, "w");
        if (!configFile) {
            log(LOG_WARNING, true, "Failed to open board config file for writing\n");
            return;
        }
        
        uploadSuccess = true;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (configFile) {
            // Write the chunk of data to the file
            size_t written = configFile.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                log(LOG_WARNING, true, "Failed to write file chunk: wrote %d of %d bytes\n", 
                    written, upload.currentSize);
                uploadSuccess = false;
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        log(LOG_INFO, true, "Upload complete: %d bytes\n", upload.totalSize);
        
        // Extract import mode from URL parameters (sent as query parameter)
        if (server.hasArg("import_mode")) {
            importMode = server.arg("import_mode");
            log(LOG_INFO, true, "Import mode: %s\n", importMode.c_str());
        } else {
            importMode = "overwrite"; // Default mode
            log(LOG_INFO, true, "Import mode not specified, using default: overwrite\n");
        }
        
        // Close the file
        if (configFile) {
            configFile.close();
        }
        
        if (uploadSuccess) {
            // Now validate the file and load it
            if (!LittleFS.begin()) {
                log(LOG_WARNING, true, "Failed to mount LittleFS for validation\n");
                return;
            }
            
            // Open config file for reading
            File validationFile = LittleFS.open(BOARD_CONFIG_FILENAME, "r");
            if (!validationFile) {
                log(LOG_WARNING, true, "Failed to open board config file for validation\n");
                return;
            }
            
            // Parse and validate JSON format
            DynamicJsonDocument doc(32768);
            DeserializationError error = deserializeJson(doc, validationFile);
            validationFile.close();
            
            if (error) {
                String errorMsg = "Failed to parse config file: " + String(error.c_str());
                log(LOG_WARNING, true, "%s\n", errorMsg.c_str());
                uploadSuccess = false;
                return;
            }
            
            // Check if this is the new export format (has "boards" array) or old format (has magic_number)
            bool isNewFormat = doc.containsKey("boards");
            bool isOldFormat = doc.containsKey("magic_number");
            
            if (isNewFormat) {
                log(LOG_INFO, true, "Importing new format configuration file\n");
                
                // Parse the new format directly
                JsonArray boards = doc["boards"];
                if (!boards) {
                    log(LOG_WARNING, true, "Invalid config file: missing boards array\n");
                    uploadSuccess = false;
                    return;
                }
                
                // Clear existing board configurations
                boardCount = 0;
                memset(boardConfigs, 0, sizeof(boardConfigs));
                
                // Import each board
                for (JsonObject board : boards) {
                    if (boardCount >= MAX_BOARDS) {
                        log(LOG_WARNING, true, "Too many boards in import file, stopping at %d\n", MAX_BOARDS);
                        break;
                    }
                    
                    // Create new board config
                    BoardConfig newBoard;
                    memset(&newBoard, 0, sizeof(BoardConfig));
                    
                    // Import basic board properties
                    strlcpy(newBoard.boardName, board["name"] | "Imported Board", MAX_BOARD_NAME_LENGTH);
                    newBoard.type = (deviceType_t)(board["type"] | 0);
                    newBoard.boardIndex = boardCount;
                    newBoard.slaveID = board["slave_id"] | 1;
                    newBoard.modbusPort = board["modbus_port"] | 0;
                    newBoard.pollTime = board["poll_time"] | 15000;
                    newBoard.recordInterval = board["record_interval"] | 15000;
                    
                    // Set initialised flag based on import mode
                    if (importMode == "online") {
                        newBoard.initialised = true;  // Set as initialised for online mode
                        // Set the slave ID as in-use
                        setSlaveIDInUse(newBoard.slaveID, newBoard.modbusPort);

                        log(LOG_INFO, true, "Board %s set as initialised (online mode), slave ID %d\n", newBoard.boardName, newBoard.slaveID);
                    } else {
                        newBoard.initialised = false; // Reset for new environment (overwrite mode)
                        log(LOG_INFO, true, "Board %s set as not initialised (overwrite mode)\n", newBoard.boardName);
                    }
                    newBoard.connected = false;   // Reset connection status for new environment
                    
                    // Import board-specific settings
                    if (newBoard.type == THERMOCOUPLE_IO && board.containsKey("channels")) {
                        JsonArray channels = board["channels"];
                        int channelIndex = 0;
                        
                        for (JsonObject channel : channels) {
                            if (channelIndex >= 8) break;
                            
                            newBoard.settings.thermocoupleIO.channels[channelIndex].alertEnable = channel["alert_enable"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].outputEnable = channel["output_enable"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].alertLatch = channel["alert_latch"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].alertEdge = channel["alert_edge"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].tcType = channel["tc_type"] | 0;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].alertSetpoint = channel["alert_setpoint"] | 0.0;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].alertHysteresis = channel["alert_hysteresis"] | 0;
                            strlcpy(newBoard.settings.thermocoupleIO.channels[channelIndex].channelName, 
                                   channel["channel_name"] | "", 33);
                            newBoard.settings.thermocoupleIO.channels[channelIndex].recordTemperature = channel["record_temperature"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].recordColdJunction = channel["record_cold_junction"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].recordStatus = channel["record_status"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].showOnDashboard = channel["show_on_dashboard"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].monitorFault = channel["monitor_fault"] | false;
                            newBoard.settings.thermocoupleIO.channels[channelIndex].monitorAlarm = channel["monitor_alarm"] | false;
                            
                            channelIndex++;
                        }
                    }
                    
                    // Add board to configuration
                    boardConfigs[boardCount] = newBoard;
                    boardCount++;
                }
                
                // Save the imported configuration in binary format
                if (!saveBoardConfig()) {
                    log(LOG_WARNING, true, "Failed to save imported configuration\n");
                    uploadSuccess = false;
                    return;
                }
                
                log(LOG_INFO, true, "Successfully imported %d boards from new format\n", boardCount);
                
            } else if (isOldFormat) {
                log(LOG_INFO, true, "Importing old format configuration file\n");
                
                // Validate the magic number for old format
                uint8_t magicNumber = doc["magic_number"] | 0;
                if (magicNumber != BOARD_CONFIG_MAGIC_NUMBER) {
                    log(LOG_WARNING, true, "Invalid config file: wrong magic number\n");
                    uploadSuccess = false;
                    return;
                }
                
                // Load the board configuration using old method
                if (!loadBoardConfig()) {
                    log(LOG_WARNING, true, "Failed to load imported configuration\n");
                    uploadSuccess = false;
                    return;
                }
            } else {
                log(LOG_WARNING, true, "Invalid config file: unrecognized format\n");
                uploadSuccess = false;
                return;
            }
            
            // Set initialised flag based on import mode
            for (uint8_t i = 0; i < boardCount; i++) {
                if (importMode == "online") {
                    boardConfigs[i].initialised = true;  // Keep boards as initialised for online mode
                    log(LOG_INFO, true, "Board %d (%s) set as initialised (online mode)\n", i, boardConfigs[i].boardName);
                } else {
                    boardConfigs[i].initialised = false; // Reset for overwrite mode
                    log(LOG_INFO, true, "Board %d (%s) set as not initialised (overwrite mode)\n", i, boardConfigs[i].boardName);
                }
                boardConfigs[i].connected = false; // Reset connection status for new environment
            }
            
            // Save the configuration with the updated initialised flags
            saveBoardConfig();
            
            // If import mode is "online", apply board configurations to start communications
            if (importMode == "online") {
                log(LOG_INFO, true, "Applying board configurations for online mode\n");
                apply_board_configs(); // This function sets up the boards for communication
            }
            
            log(LOG_INFO, true, "Board configuration imported successfully (mode: %s)\n", importMode.c_str());
        } else {
            log(LOG_WARNING, true, "Upload failed - not validating config\n");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        log(LOG_WARNING, true, "Upload aborted\n");
        uploadSuccess = false;
        if (configFile) {
            configFile.close();
        }
    }
}

// Initialise a board by assigning it an address through the Modbus assignment protocol
void handleInitialiseBoard() {
    log(LOG_INFO, false, "handleInitialiseBoard called\n");
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Debug info for API call
    log(LOG_INFO, false, "Board initialisation API called\n");
    log(LOG_INFO, false, "API has %d arguments\n", server.args());
    for (int i = 0; i < server.args(); i++) {
        log(LOG_INFO, false, "Arg[%d] = %s, Value = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    
    // Check if ID parameter is provided
    if (!server.hasArg("id")) {
        log(LOG_WARNING, true, "Missing board ID parameter\n");
        server.send(400, "application/json", "{\"error\":\"Missing board ID parameter\"}");
        return;
    }
    
    // Get board ID
    uint8_t boardId = server.arg("id").toInt();
    log(LOG_INFO, false, "Initialising board with ID: %d\n", boardId);
    
    // Check if board exists
    if (boardId >= boardCount) {
        log(LOG_WARNING, true, "Board ID %d not found (max: %d)\n", boardId, boardCount);
        server.send(404, "application/json", "{\"error\":\"Board not found\"}");
        return;
    }
    
    // Get the board's configured Modbus port
    uint8_t port = boardConfigs[boardId].modbusPort;
    
    log(LOG_INFO, false, "Board is on Modbus port %d\n", port);
    
    log(LOG_INFO, false, "Attempting to assign address on port %d\n", port);
    
    // Attempt to initialise the board using the correct Modbus configuration
    modbusConfig_t *busCfg = &modbusConfig[port];
    
    
    // Print current ID assignments
    log(LOG_INFO, false, "Current assigned IDs on port %d:\n", port);
    for (int i = 1; i < 10; i++) {
        log(LOG_INFO, false, "%d: %s ", i, busCfg->idAssigned[i] ? "assigned" : "free");
    }
    log(LOG_INFO, false, "\n");
    
    // This function communicates with a board in address assignment mode (address 245)
    // and assigns it a new slave ID
    uint8_t assigned_id = assign_address(busCfg);
    
    log(LOG_INFO, false, "assign_address() returned: %d\n", assigned_id);
    
    if (assigned_id > 0) {
        // Update the board's initialised flag and slave ID
        boardConfigs[boardId].initialised = true;
        boardConfigs[boardId].slaveID = assigned_id;
        
        // Save the configuration
        if (saveBoardConfig()) {
            log(LOG_INFO, true, "Board initialised successfully with slave ID: %d\n", assigned_id);
            
            // Create JSON response
            DynamicJsonDocument responseDoc(256);
            responseDoc["success"] = true;
            responseDoc["message"] = "Board initialised successfully";
            responseDoc["slave_id"] = assigned_id;
            
            String response;
            serializeJson(responseDoc, response);
            log(LOG_INFO, false, "Sending success response: %s\n", response.c_str());
            server.send(200, "application/json", response);
            
            // Apply the configuration immediately
            apply_board_configs();
        } else {
            log(LOG_WARNING, true, "Failed to save board configuration after initialisation\n");
            server.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
        }
    } else {
        log(LOG_WARNING, true, "Failed to initialise board - no board in assignment mode detected\n");
        server.send(500, "application/json", "{\"error\":\"Failed to initialise board. Ensure the board is in Address Assignment Mode (blue LED lit).\"}");
    }
}

// Board management functions
bool addBoard(BoardConfig newBoard) {
    // Check if we've reached the maximum number of boards
    if (boardCount >= MAX_BOARDS) {
        return false;
    }
    
    // Set a default value for the initialised field
    // New boards are not initialised by default
    newBoard.initialised = false;
    newBoard.connected = false;
    
    // Set board index and copy to our array
    newBoard.boardIndex = boardCount;
    boardConfigs[boardCount] = newBoard;
    boardCount++;
    
    // Save configuration
    return saveBoardConfig();
}

bool updateBoard(uint8_t index, BoardConfig updatedBoard) {
    // Check if the board exists
    if (index >= boardCount) {
        return false;
    }
    
    // Update the board
    boardConfigs[index] = updatedBoard;
    
    // Save to LittleFS
    return saveBoardConfig();
}

bool deleteBoard(uint8_t index) {
    // Check if the board exists
    if (index >= boardCount) {
        return false;
    }
    
    // Remove the board by shifting all boards after it
    for (uint8_t i = index; i < boardCount - 1; i++) {
        boardConfigs[i] = boardConfigs[i + 1];
    }
    
    // Decrement board count
    boardCount--;
    
    // Save to LittleFS
    return saveBoardConfig();
}

uint8_t getBoardCount() {
    return boardCount;
}

BoardConfig* getBoard(uint8_t index) {
    if (index >= boardCount) {
        return NULL;
    }
    
    return &boardConfigs[index];
}

// Helper functions
uint8_t assignBoardIndex(deviceType_t type) {
    // Find the next available board index for the given type
    bool used[MAX_BOARDS] = {false};
    
    // Mark used indices
    for (uint8_t i = 0; i < boardCount; i++) {
        if (boardConfigs[i].type == type && boardConfigs[i].boardIndex < MAX_BOARDS) {
            used[boardConfigs[i].boardIndex] = true;
        }
    }
    
    // Find first unused index
    for (uint8_t i = 0; i < MAX_BOARDS; i++) {
        if (!used[i]) {
            return i;
        }
    }
    
    // If all indices are used, return the maximum as a fallback
    return MAX_BOARDS - 1;
}

uint8_t assignSlaveID(uint8_t modbusPort) {
    // Find the next available slave ID for the given Modbus port
    bool used[247] = {false}; // Modbus allows slave IDs 1-247
    
    // Mark used slave IDs
    for (uint8_t i = 0; i < boardCount; i++) {
        if (boardConfigs[i].modbusPort == modbusPort && 
            boardConfigs[i].slaveID > 0 && 
            boardConfigs[i].slaveID <= 247) {
            used[boardConfigs[i].slaveID - 1] = true;
        }
    }
    
    // Find first unused slave ID
    for (uint8_t i = 0; i < 247; i++) {
        if (!used[i]) {
            return i + 1; // Slave IDs start at 1
        }
    }
    
    // If all slave IDs are used, return the maximum as a fallback
    return 247;
}

// Helper function to get device type name
const char* getDeviceTypeName(deviceType_t type) {
    switch (type) {
        case ANALOGUE_DIGITAL_IO:
            return "Analogue/Digital IO";
        case THERMOCOUPLE_IO:
            return "Thermocouple IO";
        case RTD_IO:
            return "RTD IO";
        case ENERGY_METER:
            return "Energy Meter";
        default:
            return "Unknown";
    }
}

// Binary serialization functions

// Pack boolean flags from BoardConfig channel into a 16-bit value
uint16_t packChannelFlags(const BoardConfig* board, int channelIndex) {
    uint16_t flags = 0;
    const auto& channel = board->settings.thermocoupleIO.channels[channelIndex];
    
    if (channel.alertEnable) flags |= (1 << TC_FLAG_ALERT_ENABLE);
    if (channel.outputEnable) flags |= (1 << TC_FLAG_OUTPUT_ENABLE);
    if (channel.alertLatch) flags |= (1 << TC_FLAG_ALERT_LATCH);
    if (channel.alertEdge) flags |= (1 << TC_FLAG_ALERT_EDGE);
    if (channel.recordTemperature) flags |= (1 << TC_FLAG_RECORD_TEMP);
    if (channel.recordColdJunction) flags |= (1 << TC_FLAG_RECORD_CJ);
    if (channel.recordStatus) flags |= (1 << TC_FLAG_RECORD_STATUS);
    if (channel.showOnDashboard) flags |= (1 << TC_FLAG_SHOW_DASHBOARD);
    if (channel.monitorFault) flags |= (1 << TC_FLAG_MONITOR_FAULT);
    if (channel.monitorAlarm) flags |= (1 << TC_FLAG_MONITOR_ALARM);
    
    return flags;
}

// Unpack 16-bit flags into BoardConfig channel booleans
void unpackChannelFlags(uint16_t flags, BoardConfig* board, int channelIndex) {
    auto& channel = board->settings.thermocoupleIO.channels[channelIndex];
    
    channel.alertEnable = (flags & (1 << TC_FLAG_ALERT_ENABLE)) != 0;
    channel.outputEnable = (flags & (1 << TC_FLAG_OUTPUT_ENABLE)) != 0;
    channel.alertLatch = (flags & (1 << TC_FLAG_ALERT_LATCH)) != 0;
    channel.alertEdge = (flags & (1 << TC_FLAG_ALERT_EDGE)) != 0;
    channel.recordTemperature = (flags & (1 << TC_FLAG_RECORD_TEMP)) != 0;
    channel.recordColdJunction = (flags & (1 << TC_FLAG_RECORD_CJ)) != 0;
    channel.recordStatus = (flags & (1 << TC_FLAG_RECORD_STATUS)) != 0;
    channel.showOnDashboard = (flags & (1 << TC_FLAG_SHOW_DASHBOARD)) != 0;
    channel.monitorFault = (flags & (1 << TC_FLAG_MONITOR_FAULT)) != 0;
    channel.monitorAlarm = (flags & (1 << TC_FLAG_MONITOR_ALARM)) != 0;
}

// Pack a thermocouple channel from BoardConfig to binary format
void packThermocoupleChannel(const BoardConfig* board, int channelIndex, BinaryThermocoupleChannel* binaryChannel) {
    const auto& channel = board->settings.thermocoupleIO.channels[channelIndex];
    
    binaryChannel->flags = packChannelFlags(board, channelIndex);
    binaryChannel->tcType = channel.tcType;
    binaryChannel->alertSetpoint = channel.alertSetpoint;
    binaryChannel->alertHysteresis = channel.alertHysteresis;
    strncpy(binaryChannel->channelName, channel.channelName, sizeof(binaryChannel->channelName) - 1);
    binaryChannel->channelName[sizeof(binaryChannel->channelName) - 1] = '\0';
}

// Unpack a thermocouple channel from binary format to BoardConfig
void unpackThermocoupleChannel(const BinaryThermocoupleChannel* binaryChannel, BoardConfig* board, int channelIndex) {
    auto& channel = board->settings.thermocoupleIO.channels[channelIndex];
    
    unpackChannelFlags(binaryChannel->flags, board, channelIndex);
    channel.tcType = binaryChannel->tcType;
    channel.alertSetpoint = binaryChannel->alertSetpoint;
    channel.alertHysteresis = binaryChannel->alertHysteresis;
    strncpy(channel.channelName, binaryChannel->channelName, sizeof(channel.channelName) - 1);
    channel.channelName[sizeof(channel.channelName) - 1] = '\0';
}

// Save board configuration in binary format
bool saveBoardConfigBinary() {
    log(LOG_INFO, true, "Saving board configuration in binary format (count: %d)\n", boardCount);
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Open file for writing
    File configFile = LittleFS.open(BINARY_CONFIG_FILENAME, "w");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open binary config file for writing\n");
        return false;
    }
    
    // Write header
    BinaryBoardHeader header;
    header.magic = BINARY_CONFIG_MAGIC;
    header.version = BINARY_CONFIG_VERSION;
    header.boardCount = boardCount;
    header.reserved = 0;
    
    if (configFile.write((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        log(LOG_WARNING, true, "Failed to write binary config header\n");
        configFile.close();
        return false;
    }
    
    // Write each board configuration
    for (uint8_t i = 0; i < boardCount; i++) {
        BinaryBoardConfig binaryBoard;
        
        // Copy basic board information
        strncpy(binaryBoard.boardName, boardConfigs[i].boardName, sizeof(binaryBoard.boardName) - 1);
        binaryBoard.boardName[sizeof(binaryBoard.boardName) - 1] = '\0';
        binaryBoard.type = boardConfigs[i].type;
        binaryBoard.boardIndex = boardConfigs[i].boardIndex;
        binaryBoard.slaveID = boardConfigs[i].slaveID;
        binaryBoard.modbusPort = boardConfigs[i].modbusPort;
        binaryBoard.pollTime = boardConfigs[i].pollTime;
        binaryBoard.recordInterval = boardConfigs[i].recordInterval;
        
        // Pack board flags
        binaryBoard.flags = 0;
        if (boardConfigs[i].initialised) binaryBoard.flags |= 0x01;
        if (boardConfigs[i].connected) binaryBoard.flags |= 0x02;
        
        // Clear reserved bytes
        memset(binaryBoard.reserved, 0, sizeof(binaryBoard.reserved));
        
        // Pack board-specific settings
        if (boardConfigs[i].type == THERMOCOUPLE_IO) {
            for (int j = 0; j < 8; j++) {
                packThermocoupleChannel(&boardConfigs[i], j, &binaryBoard.settings.thermocoupleChannels[j]);
            }
        } else {
            // For other board types, clear the settings area
            memset(binaryBoard.settings.rawData, 0, sizeof(binaryBoard.settings.rawData));
        }
        
        // Write board configuration
        if (configFile.write((uint8_t*)&binaryBoard, sizeof(binaryBoard)) != sizeof(binaryBoard)) {
            log(LOG_WARNING, true, "Failed to write binary config for board %d\n", i);
            configFile.close();
            return false;
        }
    }
    
    configFile.close();
    
    // Calculate and log file size
    size_t fileSize = sizeof(BinaryBoardHeader) + (boardCount * sizeof(BinaryBoardConfig));
    log(LOG_INFO, true, "Binary config saved: %d bytes (%d boards)\n", fileSize, boardCount);
    
    return true;
}

// Load board configuration from binary format
bool loadBoardConfigBinary() {
    log(LOG_INFO, true, "Loading board configuration from binary format\n");
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Check if binary config file exists
    if (!LittleFS.exists(BINARY_CONFIG_FILENAME)) {
        log(LOG_INFO, true, "Binary config file does not exist, starting with empty configuration\n");
        boardCount = 0;
        return true;
    }
    
    // Open file for reading
    File configFile = LittleFS.open(BINARY_CONFIG_FILENAME, "r");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open binary config file for reading\n");
        return false;
    }
    
    // Read and validate header
    BinaryBoardHeader header;
    if (configFile.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        log(LOG_WARNING, true, "Failed to read binary config header\n");
        configFile.close();
        return false;
    }
    
    if (header.magic != BINARY_CONFIG_MAGIC) {
        log(LOG_WARNING, true, "Invalid binary config magic number: 0x%02X\n", header.magic);
        configFile.close();
        return false;
    }
    
    if (header.version != BINARY_CONFIG_VERSION) {
        log(LOG_WARNING, true, "Unsupported binary config version: %d\n", header.version);
        configFile.close();
        return false;
    }
    
    if (header.boardCount > MAX_BOARDS) {
        log(LOG_WARNING, true, "Too many boards in config: %d (max %d)\n", header.boardCount, MAX_BOARDS);
        configFile.close();
        return false;
    }
    
    // Read board configurations
    boardCount = header.boardCount;
    for (uint8_t i = 0; i < boardCount; i++) {
        BinaryBoardConfig binaryBoard;
        
        if (configFile.read((uint8_t*)&binaryBoard, sizeof(binaryBoard)) != sizeof(binaryBoard)) {
            log(LOG_WARNING, true, "Failed to read binary config for board %d\n", i);
            configFile.close();
            boardCount = i; // Partial load
            return false;
        }
        
        // Unpack basic board information
        strncpy(boardConfigs[i].boardName, binaryBoard.boardName, sizeof(boardConfigs[i].boardName) - 1);
        boardConfigs[i].boardName[sizeof(boardConfigs[i].boardName) - 1] = '\0';
        boardConfigs[i].type = (deviceType_t)binaryBoard.type;
        boardConfigs[i].boardIndex = binaryBoard.boardIndex;
        boardConfigs[i].slaveID = binaryBoard.slaveID;
        boardConfigs[i].modbusPort = binaryBoard.modbusPort;
        boardConfigs[i].pollTime = binaryBoard.pollTime;
        boardConfigs[i].recordInterval = binaryBoard.recordInterval;
        
        // Unpack board flags
        boardConfigs[i].initialised = (binaryBoard.flags & 0x01) != 0;
        boardConfigs[i].connected = (binaryBoard.flags & 0x02) != 0;
        
        // Unpack board-specific settings
        if (boardConfigs[i].type == THERMOCOUPLE_IO) {
            for (int j = 0; j < 8; j++) {
                unpackThermocoupleChannel(&binaryBoard.settings.thermocoupleChannels[j], &boardConfigs[i], j);
            }
        }
        
        log(LOG_DEBUG, false, "Loaded board %d: %s (type %d)\n", i, boardConfigs[i].boardName, boardConfigs[i].type);
    }
    
    configFile.close();
    
    log(LOG_INFO, true, "Binary config loaded: %d boards\n", boardCount);
    return true;
}
