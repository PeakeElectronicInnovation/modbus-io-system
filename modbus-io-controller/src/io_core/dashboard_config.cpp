#include "dashboard_config.h"
#include "board_config.h"
#include "../utils/logger.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Global variables
DashboardConfig dashboardConfig;
extern WebServer server;

// Initialize dashboard configuration
void init_dashboard_config(void) {
    // Load existing dashboard configuration from LittleFS
    if (!loadDashboardConfig()) {
        // If configuration doesn't exist or is invalid, initialize with defaults
        log(LOG_INFO, false, "Invalid dashboard configuration, starting with empty configuration\n");
        dashboardConfig.itemCount = 0;
        memset(dashboardConfig.items, 0, sizeof(dashboardConfig.items));
        saveDashboardConfig();
    }
    
    // Set up API endpoints
    setupDashboardAPI();
}

// Load dashboard configuration from LittleFS
bool loadDashboardConfig() {
    log(LOG_INFO, false, "Loading dashboard configuration\n");
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Check if config file exists
    if (!LittleFS.exists(DASHBOARD_CONFIG_FILENAME)) {
        log(LOG_WARNING, true, "Dashboard config file not found\n");
        return false;
    }
    
    // Open config file
    File configFile = LittleFS.open(DASHBOARD_CONFIG_FILENAME, "r");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open dashboard config file\n");
        return false;
    }
    
    // Allocate a buffer to store contents of the file
    DynamicJsonDocument doc(4096); // Increased buffer for 80 dashboard items
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        log(LOG_WARNING, true, "Failed to parse dashboard config file: %s\n", error.c_str());
        return false;
    }
    
    // Check magic number
    uint8_t magicNumber = doc["magic_number"] | 0;
    log(LOG_INFO, false, "Dashboard config magic number: %x\n", magicNumber);
    if (magicNumber != DASHBOARD_CONFIG_MAGIC_NUMBER) {
        log(LOG_WARNING, true, "Invalid dashboard config magic number\n");
        return false;
    }
    
    // Get item count and chart configuration
    dashboardConfig.itemCount = doc["item_count"] | 0;
    dashboardConfig.chartVisible = doc["chart_visible"] | false;
    dashboardConfig.chartItemCount = doc["chart_item_count"] | 0;
    
    // Parse dashboard items
    JsonArray items = doc["items"];
    
    // Reset all dashboard items to ensure clean state
    memset(dashboardConfig.items, 0, sizeof(dashboardConfig.items));
    
    // Load each dashboard item
    uint8_t index = 0;
    for (JsonObject item : items) {
        if (index >= dashboardConfig.itemCount) break;
        
        dashboardConfig.items[index].boardIndex = item["board_index"] | 0;
        dashboardConfig.items[index].channelIndex = item["channel_index"] | 0;
        dashboardConfig.items[index].displayOrder = item["display_order"] | index;
        dashboardConfig.items[index].showInChart = item["show_in_chart"] | false;
        
        index++;
    }
    
    log(LOG_INFO, false, "Loaded %d dashboard items\n", dashboardConfig.itemCount);
    return true;
}

// Save dashboard configuration to LittleFS
bool saveDashboardConfig() {
    log(LOG_INFO, true, "Saving dashboard configuration (count: %d):\n", dashboardConfig.itemCount);
    
    // Check if LittleFS is mounted
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS\n");
        return false;
    }
    
    // Create JSON document
    DynamicJsonDocument doc(4096); // Increased buffer for 80 dashboard items
    
    // Add magic number, item count, and chart configuration
    doc["magic_number"] = DASHBOARD_CONFIG_MAGIC_NUMBER;
    doc["item_count"] = dashboardConfig.itemCount;
    doc["chart_visible"] = dashboardConfig.chartVisible;
    doc["chart_item_count"] = dashboardConfig.chartItemCount;
    
    // Create items array
    JsonArray items = doc.createNestedArray("items");
    
    // Add each item to the JSON document
    for (uint8_t i = 0; i < dashboardConfig.itemCount; i++) {
        JsonObject item = items.createNestedObject();
        
        item["board_index"] = dashboardConfig.items[i].boardIndex;
        item["channel_index"] = dashboardConfig.items[i].channelIndex;
        item["display_order"] = dashboardConfig.items[i].displayOrder;
        item["show_in_chart"] = dashboardConfig.items[i].showInChart;
    }
    
    // Open file for writing
    File configFile = LittleFS.open(DASHBOARD_CONFIG_FILENAME, "w");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open dashboard config file for writing\n");
        return false;
    }
    
    // Serialize JSON to file
    if (serializeJson(doc, configFile) == 0) {
        log(LOG_WARNING, true, "Failed to write dashboard config to file\n");
        configFile.close();
        return false;
    }
    
    // Debug output to validate config
    String debugOutput;
    serializeJson(doc, debugOutput);
    log(LOG_INFO, false, "Dashboard config saved: %s\n", debugOutput.c_str());
    
    // Close file
    configFile.close();
    
    // Return success
    return true;
}

// Set up API endpoints for dashboard configuration
void setupDashboardAPI() {
    server.on("/api/dashboard/items", HTTP_GET, handleGetDashboardItems);
    server.on("/api/dashboard/order", HTTP_POST, handleSaveDashboardOrder);
}

// Handle GET request for dashboard items
void handleGetDashboardItems() {
    // Create JSON document for response
    DynamicJsonDocument doc(8192); // Large buffer for 80 dashboard items with full metadata
    
    // Create items array
    JsonArray items = doc.createNestedArray("items");
    
    // Keep track of dashboard-enabled channels that aren't in the dashboard config
    bool needsUpdate = false;
    
    // First, add all items already in the dashboard config
    for (uint8_t i = 0; i < dashboardConfig.itemCount; i++) {
        uint8_t boardIndex = dashboardConfig.items[i].boardIndex;
        uint8_t channelIndex = dashboardConfig.items[i].channelIndex;
        
        // Verify this board and channel still exist and are enabled for dashboard
        bool valid = false;
        
        if (boardIndex < getBoardCount()) {
            BoardConfig* board = getBoard(boardIndex);
            if (board) {
                if (board->type == THERMOCOUPLE_IO && 
                    channelIndex < 8 && 
                    board->settings.thermocoupleIO.channels[channelIndex].showOnDashboard) {
                    valid = true;
                }
                // Add more board types as needed
            }
        }
        
        if (valid) {
            // Add to response
            JsonObject item = items.createNestedObject();
            
            // Get board for adding details
            BoardConfig* board = getBoard(boardIndex);
            
            item["board_index"] = boardIndex;
            item["channel_index"] = channelIndex;
            item["board_name"] = board->boardName;
            item["channel_name"] = board->settings.thermocoupleIO.channels[channelIndex].channelName;
            item["display_order"] = dashboardConfig.items[i].displayOrder;
            item["show_in_chart"] = dashboardConfig.items[i].showInChart;
            item["board_type"] = board->type;
        } else {
            // This item is no longer valid, mark for update
            needsUpdate = true;
        }
    }
    
    // Now check for any dashboard-enabled channels not in the config
    for (uint8_t boardIndex = 0; boardIndex < getBoardCount(); boardIndex++) {
        BoardConfig* board = getBoard(boardIndex);
        
        if (board->type == THERMOCOUPLE_IO) {
            for (uint8_t channelIndex = 0; channelIndex < 8; channelIndex++) {
                if (board->settings.thermocoupleIO.channels[channelIndex].showOnDashboard) {
                    // Check if this channel is already in the dashboard config
                    bool found = false;
                    for (uint8_t i = 0; i < dashboardConfig.itemCount; i++) {
                        if (dashboardConfig.items[i].boardIndex == boardIndex && 
                            dashboardConfig.items[i].channelIndex == channelIndex) {
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        // Add this channel to the dashboard config and response
                        if (dashboardConfig.itemCount < MAX_DASHBOARD_ITEMS) {
                            dashboardConfig.items[dashboardConfig.itemCount].boardIndex = boardIndex;
                            dashboardConfig.items[dashboardConfig.itemCount].channelIndex = channelIndex;
                            dashboardConfig.items[dashboardConfig.itemCount].displayOrder = dashboardConfig.itemCount;
                            
                            // Add to response
                            JsonObject item = items.createNestedObject();
                            item["board_index"] = boardIndex;
                            item["board_name"] = board->boardName;
                            item["channel_index"] = channelIndex;
                            item["display_order"] = dashboardConfig.itemCount;
                            item["board_type"] = board->type;
                            item["channel_name"] = board->settings.thermocoupleIO.channels[channelIndex].channelName;
                            
                            dashboardConfig.itemCount++;
                            needsUpdate = true;
                        }
                    }
                }
            }
        }
        // Add more board types as needed
    }
    
    // If we made changes to the dashboard config, save it
    if (needsUpdate) {
        saveDashboardConfig();
    }
    
    // Add chart configuration to response
    doc["chart_visible"] = dashboardConfig.chartVisible;
    
    // Serialize response
    String response;
    serializeJson(doc, response);
    
    // Send response
    server.send(200, "application/json", response);
}

// Handle POST request for saving dashboard order
void handleSaveDashboardOrder() {
    // Check if request has the correct content type
    if (server.hasArg("plain")) {
        // Parse JSON request
        DynamicJsonDocument doc(8196); // Large buffer for 80 dashboard items
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (error) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Check for items array
        if (!doc.containsKey("items")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing items array\"}");
            return;
        }
        
        JsonArray items = doc["items"];
        
        // Extract chart configuration
        bool chartVisible = doc["chart_visible"] | false;
        JsonArray chartItems = doc["chart_items"];
        
        // Temporary arrays to hold the new order
        uint8_t boardIndices[MAX_DASHBOARD_ITEMS];
        uint8_t channelIndices[MAX_DASHBOARD_ITEMS];
        uint8_t displayOrders[MAX_DASHBOARD_ITEMS];
        bool showInChart[MAX_DASHBOARD_ITEMS];
        uint8_t count = 0;
        
        // Process each item
        for (JsonObject item : items) {
            if (count >= MAX_DASHBOARD_ITEMS) break;
            
            // Extract values
            uint8_t boardIndex = item["board_index"] | 0;
            uint8_t channelIndex = item["channel_index"] | 0;
            uint8_t displayOrder = item["display_order"] | count;
            
            // Check if this item should be in chart
            bool inChart = false;
            for (JsonObject chartItem : chartItems) {
                if ((chartItem["board_index"] | 255) == boardIndex && 
                    (chartItem["channel_index"] | 255) == channelIndex) {
                    inChart = true;
                    break;
                }
            }
            
            // Store in temporary arrays
            boardIndices[count] = boardIndex;
            channelIndices[count] = channelIndex;
            displayOrders[count] = displayOrder;
            showInChart[count] = inChart;
            
            count++;
        }
        
        // Update chart visibility
        dashboardConfig.chartVisible = chartVisible;
        
        // Update dashboard order with chart settings
        if (updateDashboardOrderWithChart(boardIndices, channelIndices, displayOrders, showInChart, count)) {
            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to update dashboard order\"}");
        }
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request\"}");
    }
}

// Add a dashboard item
bool addDashboardItem(uint8_t boardIndex, uint8_t channelIndex) {
    // Check if item already exists
    for (uint8_t i = 0; i < dashboardConfig.itemCount; i++) {
        if (dashboardConfig.items[i].boardIndex == boardIndex && 
            dashboardConfig.items[i].channelIndex == channelIndex) {
            // Item already exists
            return true;
        }
    }
    
    // Check if we have room for another item
    if (dashboardConfig.itemCount >= MAX_DASHBOARD_ITEMS) {
        return false;
    }
    
    // Add new item
    dashboardConfig.items[dashboardConfig.itemCount].boardIndex = boardIndex;
    dashboardConfig.items[dashboardConfig.itemCount].channelIndex = channelIndex;
    dashboardConfig.items[dashboardConfig.itemCount].displayOrder = dashboardConfig.itemCount;
    dashboardConfig.itemCount++;
    
    // Save config
    return saveDashboardConfig();
}

// Remove a dashboard item
bool removeDashboardItem(uint8_t boardIndex, uint8_t channelIndex) {
    bool found = false;
    uint8_t foundIndex = 0;
    
    // Find the item
    for (uint8_t i = 0; i < dashboardConfig.itemCount; i++) {
        if (dashboardConfig.items[i].boardIndex == boardIndex && 
            dashboardConfig.items[i].channelIndex == channelIndex) {
            found = true;
            foundIndex = i;
            break;
        }
    }
    
    if (!found) {
        return false;
    }
    
    // Remove the item by shifting all items after it
    for (uint8_t i = foundIndex; i < dashboardConfig.itemCount - 1; i++) {
        dashboardConfig.items[i] = dashboardConfig.items[i + 1];
    }
    
    dashboardConfig.itemCount--;
    
    // Save config
    return saveDashboardConfig();
}

// Update dashboard order
bool updateDashboardOrder(uint8_t *boardIndices, uint8_t *channelIndices, uint8_t *displayOrders, uint8_t count) {
    // Reset dashboard config
    dashboardConfig.itemCount = 0;
    memset(dashboardConfig.items, 0, sizeof(dashboardConfig.items));
    
    // Add items in new order
    for (uint8_t i = 0; i < count; i++) {
        if (i >= MAX_DASHBOARD_ITEMS) break;
        
        dashboardConfig.items[i].boardIndex = boardIndices[i];
        dashboardConfig.items[i].channelIndex = channelIndices[i];
        dashboardConfig.items[i].displayOrder = displayOrders[i];
        dashboardConfig.items[i].showInChart = false; // Default to not in chart
        dashboardConfig.itemCount++;
    }
    
    // Save config
    return saveDashboardConfig();
}

// Update dashboard order with chart settings
bool updateDashboardOrderWithChart(uint8_t *boardIndices, uint8_t *channelIndices, uint8_t *displayOrders, bool *showInChart, uint8_t count) {
    // Reset dashboard config
    dashboardConfig.itemCount = 0;
    memset(dashboardConfig.items, 0, sizeof(dashboardConfig.items));
    
    // Count chart items
    uint8_t chartCount = 0;
    
    // Add items in new order
    for (uint8_t i = 0; i < count; i++) {
        if (i >= MAX_DASHBOARD_ITEMS) break;
        
        dashboardConfig.items[i].boardIndex = boardIndices[i];
        dashboardConfig.items[i].channelIndex = channelIndices[i];
        dashboardConfig.items[i].displayOrder = displayOrders[i];
        dashboardConfig.items[i].showInChart = showInChart[i];
        
        if (showInChart[i]) {
            chartCount++;
        }
        
        dashboardConfig.itemCount++;
    }
    
    // Update chart item count
    dashboardConfig.chartItemCount = chartCount;
    
    // Save config
    return saveDashboardConfig();
}
