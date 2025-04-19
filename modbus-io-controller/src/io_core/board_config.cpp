#include "board_config.h"
#include "io_core.h"
#include <WebServer.h>

// Global variables
BoardConfig boardConfigs[MAX_BOARDS];
uint8_t boardCount = 0;

// Initialize board configuration
void init_board_config(void) {
    // Load existing configuration from LittleFS
    if (!loadBoardConfig()) {
        // If configuration doesn't exist or is invalid, initialize with defaults
        log(LOG_INFO, false, "Invalid board configuration, starting with empty configuration\n");
        boardCount = 0;
        saveBoardConfig();
    }
    
    // Set up API endpoints
    setupBoardConfigAPI();
}

// Load board configuration from LittleFS
bool loadBoardConfig() {
    log(LOG_INFO, true, "Loading board configuration:\n");
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Check if config file exists
    if (!LittleFS.exists(BOARD_CONFIG_FILENAME)) {
        log(LOG_WARNING, true, "Board config file not found\n");
        return false;
    }
    
    // Open config file
    File configFile = LittleFS.open(BOARD_CONFIG_FILENAME, "r");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open board config file\n");
        return false;
    }
    
    // Allocate a buffer to store contents of the file
    DynamicJsonDocument doc(4096); // Larger buffer for multiple board configurations
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        log(LOG_WARNING, true, "Failed to parse board config file: %s\n", error.c_str());
        return false;
    }
    
    // Check magic number
    uint8_t magicNumber = doc["magic_number"] | 0;
    log(LOG_INFO, true, "Board config magic number: %x\n", magicNumber);
    if (magicNumber != BOARD_CONFIG_MAGIC_NUMBER) {
        log(LOG_WARNING, true, "Invalid board config magic number\n");
        return false;
    }
    
    // Get board count
    boardCount = doc["board_count"] | 0;
    if (boardCount > MAX_BOARDS) {
        log(LOG_WARNING, true, "Board count exceeds maximum, truncating\n");
        boardCount = MAX_BOARDS;
    }
    
    // Parse board configurations
    JsonArray boards = doc["boards"];
    
    // Reset all board configs to ensure clean state
    memset(boardConfigs, 0, sizeof(boardConfigs));
    
    // Load each board configuration
    uint8_t index = 0;
    for (JsonObject board : boards) {
        if (index >= boardCount) break;
        
        // Load common board properties
        strlcpy(boardConfigs[index].boardName, board["name"] | "Unnamed", MAX_BOARD_NAME_LENGTH);
        boardConfigs[index].type = (deviceType_t)(board["type"] | 0);
        boardConfigs[index].boardIndex = board["index"] | 0;
        boardConfigs[index].slaveID = board["slave_id"] | 0;
        boardConfigs[index].modbusPort = board["modbus_port"] | 0;
        boardConfigs[index].pollTime = board["poll_time"] | 1000;
        
        // Load board-specific settings based on type
        if (boardConfigs[index].type == THERMOCOUPLE_IO) {
            JsonArray channels = board["channels"];
            int channelIndex = 0;
            
            for (JsonObject channel : channels) {
                if (channelIndex >= 8) break;
                
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].alertEnable = channel["alert_enable"] | false;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].outputEnable = channel["output_enable"] | false;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].alertLatch = channel["alert_latch"] | false;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].alertEdge = channel["alert_edge"] | false;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].tcType = channel["tc_type"] | 0;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].alertSetpoint = channel["alert_setpoint"] | 0.0f;
                boardConfigs[index].settings.thermocoupleIO.channels[channelIndex].alertHysteresis = channel["alert_hysteresis"] | 0;
                
                channelIndex++;
            }
        }
        // Add more board types as needed
        
        index++;
    }
    
    log(LOG_INFO, true, "Loaded %d board configurations\n", boardCount);
    return true;
}

// Save board configuration to LittleFS
bool saveBoardConfig() {
    log(LOG_INFO, true, "Saving board configuration (count: %d):\n", boardCount);
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Create JSON document
    DynamicJsonDocument doc(4096); // Larger buffer for multiple board configurations
    
    // Add magic number and board count
    doc["magic_number"] = BOARD_CONFIG_MAGIC_NUMBER;
    doc["board_count"] = boardCount;
    
    // Create boards array
    JsonArray boards = doc.createNestedArray("boards");
    
    // Add each board to the JSON document
    for (uint8_t i = 0; i < boardCount; i++) {
        JsonObject board = boards.createNestedObject();
        
        // Add common board properties
        board["name"] = boardConfigs[i].boardName;
        board["type"] = boardConfigs[i].type;
        board["index"] = boardConfigs[i].boardIndex;
        board["slave_id"] = boardConfigs[i].slaveID;
        board["modbus_port"] = boardConfigs[i].modbusPort;
        board["poll_time"] = boardConfigs[i].pollTime;
        
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
            }
        }
        // Add more board types as needed
    }
    
    // Open file for writing
    File configFile = LittleFS.open(BOARD_CONFIG_FILENAME, "w");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open board config file for writing\n");
        return false;
    }
    
    // Serialize JSON to file
    if (serializeJson(doc, configFile) == 0) {
        log(LOG_WARNING, true, "Failed to write board config to file\n");
        configFile.close();
        return false;
    }
    
    // Debug output to validate config
    String debugOutput;
    serializeJson(doc, debugOutput);
    log(LOG_INFO, true, "Board config saved: %s\n", debugOutput.c_str());
    
    // Close file
    configFile.close();
    
    // IMPORTANT: Do NOT call LittleFS.end() here as it will cause "File not found" errors
    // when serving web content
    
    return true;
}

// Set up API endpoints for board configuration
void setupBoardConfigAPI() {
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
}

void handleGetBoardConfig() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Print board count for debugging
    log(LOG_INFO, false, "Serving board configurations, count: %d\n", boardCount);
    
    // If there are no boards, return an empty array
    if (boardCount == 0) {
        log(LOG_INFO, true, "No boards configured, returning empty array\n");
        server.send(200, "application/json", "{\"boards\":[]}");
        return;
    }
    
    // Create JSON response
    DynamicJsonDocument doc(4096);
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
            }
        }
        // Add more board types as needed
    }
    
    // Debug the response
    String debugResponse;
    serializeJson(doc, debugResponse);
    log(LOG_INFO, false, "API Response: %s\n", debugResponse.c_str());
    
    // Send response
    String response;
    serializeJson(doc, response);
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
    log(LOG_INFO, true, "Add board request body length: %d\n", requestData.length());
    if (requestData.length() > 0) {
        // Only print part of the request to avoid log overflow
        String truncated = requestData.substring(0, min(200, (int)requestData.length()));
        log(LOG_INFO, true, "Add board request (truncated): %s\n", truncated.c_str());
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
    
    // Parse JSON request
    DynamicJsonDocument doc(2048);
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
    int pollTime = doc["poll_time"] | 1000;
    
    log(LOG_INFO, true, "Adding board: %s, type: %d, port: %d, poll: %d\n", 
        name, type, modbusPort, pollTime);
    
    // Create new board config at the end of the array
    strlcpy(boardConfigs[boardCount].boardName, name, MAX_BOARD_NAME_LENGTH);
    boardConfigs[boardCount].type = (deviceType_t)type;
    boardConfigs[boardCount].boardIndex = boardCount; // Use boardCount as index
    boardConfigs[boardCount].slaveID = 1; // Default slave ID
    boardConfigs[boardCount].modbusPort = modbusPort;
    boardConfigs[boardCount].pollTime = pollTime;
    
    // Handle board type specific settings
    if (type == THERMOCOUPLE_IO) {
        JsonArray channels = doc["channels"];
        if (channels) {
            int channelIndex = 0;
            for (JsonObject channel : channels) {
                if (channelIndex >= 8) break;
                
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].alertEnable = channel["alert_enable"] | false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].outputEnable = channel["output_enable"] | false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].alertLatch = channel["alert_latch"] | false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].alertEdge = channel["alert_edge"] | false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].tcType = channel["tc_type"] | 0;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].alertSetpoint = channel["alert_setpoint"] | 0.0f;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[channelIndex].alertHysteresis = channel["alert_hysteresis"] | 0;
                
                channelIndex++;
            }
        } else {
            log(LOG_WARNING, true, "No channels array found for THERMOCOUPLE_IO board\n");
            // Initialize with default values
            for (int j = 0; j < 8; j++) {
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].alertEnable = false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].outputEnable = false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].alertLatch = false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].alertEdge = false;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].tcType = 0;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].alertSetpoint = 0.0f;
                boardConfigs[boardCount].settings.thermocoupleIO.channels[j].alertHysteresis = 0;
            }
        }
    }
    
    // Increment board count after successfully adding the board
    boardCount++;
    
    // Save configuration to LittleFS
    if (!saveBoardConfig()) {
        log(LOG_WARNING, true, "Failed to save board configuration\n");
        server.send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
        return;
    }
    
    // Apply the board configuration immediately
    apply_board_configs();
    log(LOG_INFO, true, "Applied board configurations\n");
    
    // Create response with the new board
    DynamicJsonDocument responseDoc(1024);
    responseDoc["success"] = true;
    responseDoc["message"] = "Board added successfully";
    responseDoc["id"] = boardCount - 1; // ID of the newly added board
    
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
    
    log(LOG_INFO, true, "Board added successfully, new count: %d\n", boardCount);
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
    log(LOG_INFO, true, "Update board request body length: %d\n", requestData.length());
    if (requestData.length() > 0) {
        // Only print part of the request to avoid log overflow
        String truncated = requestData.substring(0, min(200, (int)requestData.length()));
        log(LOG_INFO, true, "Update board request (truncated): %s\n", truncated.c_str());
    } else {
        log(LOG_WARNING, true, "Empty request body\n");
        server.send(400, "application/json", "{\"error\":\"Empty request body\"}");
        return;
    }
    
    // Parse request body
    DynamicJsonDocument doc(2048);  // Increase buffer size from 1024 to 2048
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
        updatedBoard.pollTime = doc["poll_time"];
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
            
            channelIndex++;
        }
    }
    // Add more board types as needed
    
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
    
    // Print board count for debugging
    log(LOG_INFO, false, "Serving board configurations, count: %d\n", boardCount);
    
    // If there are no boards, return an empty array
    if (boardCount == 0) {
        log(LOG_INFO, true, "No boards configured, returning empty array\n");
        server.send(200, "application/json", "{\"boards\":[]}");
        return;
    }
    
    // Create JSON response
    DynamicJsonDocument doc(4096);
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
            }
        }
        // Add more board types as needed
    }
    
    // Debug the response
    String debugResponse;
    serializeJson(doc, debugResponse);
    log(LOG_INFO, false, "API Response: %s\n", debugResponse.c_str());
    
    // Send response
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// Board management functions
bool addBoard(BoardConfig newBoard) {
    // Check if we've reached the maximum number of boards
    if (boardCount >= MAX_BOARDS) {
        return false;
    }
    
    // Add the new board
    boardConfigs[boardCount] = newBoard;
    boardCount++;
    
    // Save to LittleFS
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
        case THERMOCOUPLE_IO:
            return "Thermocouple IO";
        case UNIVERSAL_IN:
            return "Universal Input";
        case DIGITAL_IO:
            return "Digital IO";
        case POWER_METER:
            return "Power Meter";
        default:
            return "Unknown";
    }
}
