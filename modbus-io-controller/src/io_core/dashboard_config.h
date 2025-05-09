#pragma once

#include "../sys_init.h"
#include "board_config.h"

// LittleFS configuration
#define DASHBOARD_CONFIG_FILENAME "/dashboard_config.json"
#define DASHBOARD_CONFIG_MAGIC_NUMBER 0x67

// Dashboard item structure
struct DashboardItem {
    uint8_t boardIndex;
    uint8_t channelIndex;
    uint8_t displayOrder;
};

// Dashboard configuration structure
struct DashboardConfig {
    uint8_t itemCount;
    DashboardItem items[MAX_BOARDS * 8]; // Maximum possible items (all channels on all boards)
};

// Dashboard configuration manager APIs
void init_dashboard_config(void);
bool loadDashboardConfig(void);
bool saveDashboardConfig(void);
void setupDashboardAPI(void);

// API handlers
void handleGetDashboardItems(void);
void handleSaveDashboardOrder(void);

// Dashboard management functions
bool addDashboardItem(uint8_t boardIndex, uint8_t channelIndex);
bool removeDashboardItem(uint8_t boardIndex, uint8_t channelIndex);
bool updateDashboardOrder(uint8_t *boardIndices, uint8_t *channelIndices, uint8_t *displayOrders, uint8_t count);

// Global variables
extern DashboardConfig dashboardConfig;