#include "modbus_tcp.h"
#include "../io_core/io_core.h"
#include "network.h"
#include "../io_core/board_config.h"

// Global variables
ModbusTCPServer modbusServer;
ModbusTCPConfig modbusTCPConfig;

ModbusTCPServer::ModbusTCPServer() : _server(nullptr), _running(false) {
    // Initialize client connections
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        _clients[i].active = false;
        _clients[i].lastActivity = 0;
        _clients[i].clientIP = "";
    }
    
    // Default configuration
    _config.port = MODBUS_TCP_DEFAULT_PORT;
    _config.enabled = true;
}

ModbusTCPServer::~ModbusTCPServer() {
    stop();
}

bool ModbusTCPServer::begin(uint16_t port) {
    log(LOG_INFO, true, "ModbusTCPServer::begin() called with port %d\n", port);
    
    if (_running) {
        log(LOG_INFO, true, "Server already running, stopping first\n");
        stop();
    }
    
    _config.port = port;
    _server = new WiFiServer(port);
    
    if (!_server) {
        log(LOG_ERROR, true, "Failed to create Modbus TCP server\n");
        return false;
    }
    
    _server->begin();
    _running = true;
    
    log(LOG_INFO, true, "Modbus TCP server started on port %d (config port: %d)\n", port, _config.port);
    return true;
}

void ModbusTCPServer::stop() {
    if (_server) {
        disconnectAllClients();
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    _running = false;
    log(LOG_INFO, true, "Modbus TCP server stopped\n");
}

void ModbusTCPServer::poll() {
    if (!_running || !_config.enabled) {
        return;
    }
    
    acceptNewClients();
    processClientRequests();
    cleanupInactiveClients();
}

void ModbusTCPServer::acceptNewClients() {
    if (!_server) return;
    
    WiFiClient newClient = _server->accept();
    if (newClient) {
        int slot = findFreeClientSlot();
        if (slot >= 0) {
            _clients[slot].client = newClient;
            _clients[slot].active = true;
            _clients[slot].lastActivity = millis();
            _clients[slot].connectionTime = millis();
            _clients[slot].clientIP = newClient.remoteIP().toString();
            
            log(LOG_INFO, true, "Modbus TCP client connected from %s (slot %d)\n", 
                _clients[slot].clientIP.c_str(), slot);
        } else {
            // No free slots, reject the connection
            newClient.stop();
            log(LOG_WARNING, true, "Modbus TCP client rejected - maximum connections reached\n");
        }
    }
}

void ModbusTCPServer::processClientRequests() {
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        if (_clients[i].active && _clients[i].client.connected()) {
            if (_clients[i].client.available()) {
                _clients[i].lastActivity = millis();
                processModbusRequest(_clients[i]);
            }
        }
    }
}

void ModbusTCPServer::cleanupInactiveClients() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        if (_clients[i].active) {
            // Check if client is still connected (primary disconnect detection)
            if (!_clients[i].client.connected()) {
                log(LOG_INFO, true, "Modbus TCP client %s disconnected (slot %d, connected for %lu ms)\n", 
                    _clients[i].clientIP.c_str(), i, currentTime - _clients[i].connectionTime);
                _clients[i].client.stop();
                _clients[i].active = false;
                _clients[i].clientIP = "";
                _clients[i].connectionTime = 0;
            }
            // Check for timeout (only if no activity for extended period)
            else if (currentTime - _clients[i].lastActivity > MODBUS_TCP_TIMEOUT) {
                log(LOG_WARNING, true, "Modbus TCP client %s timed out after %lu ms of inactivity (slot %d)\n", 
                    _clients[i].clientIP.c_str(), MODBUS_TCP_TIMEOUT, i);
                _clients[i].client.stop();
                _clients[i].active = false;
                _clients[i].clientIP = "";
                _clients[i].connectionTime = 0;
            }
        }
    }
}

int ModbusTCPServer::findFreeClientSlot() {
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        if (!_clients[i].active) {
            return i;
        }
    }
    return -1;
}

bool ModbusTCPServer::processModbusRequest(ModbusClientConnection& client) {
    if (client.client.available() < 7) {
        return false; // Need at least MBAP header
    }
    
    // Read MBAP header
    ModbusMBAPHeader header;
    uint8_t headerBytes[7];
    client.client.readBytes(headerBytes, 7);
    
    header.transactionId = (headerBytes[0] << 8) | headerBytes[1];
    header.protocolId = (headerBytes[2] << 8) | headerBytes[3];
    header.length = (headerBytes[4] << 8) | headerBytes[5];
    header.unitId = headerBytes[6];
    
    // Validate protocol ID
    if (header.protocolId != 0) {
        sendModbusException(client, header.transactionId, header.unitId, 0, MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE);
        return false;
    }
    
    // Read PDU (Protocol Data Unit)
    uint16_t pduLength = header.length - 1; // Subtract unit ID
    if (pduLength > 253) { // Maximum PDU length
        sendModbusException(client, header.transactionId, header.unitId, 0, MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE);
        return false;
    }
    
    uint8_t pdu[253];
    if (client.client.readBytes(pdu, pduLength) != pduLength) {
        return false;
    }
    
    // Forward to RTU if unit ID is not 0xFF (TCP broadcast)
    if (header.unitId != 0xFF && header.unitId != 0) {
        uint8_t rtuResponse[256];
        uint16_t rtuResponseLength;
        uint8_t pduResponse[256];
        uint16_t pduResponseLength;
        
        // Handle read request using cached data
        if (handleReadRequest(header.unitId, pdu[0], (pdu[1] << 8) | pdu[2], (pdu[3] << 8) | pdu[4], pduResponse, pduResponseLength)) {
            // Send successful response back to TCP client
            uint8_t tcpResponse[260]; // MBAP + PDU response
            
            // Build MBAP header for response
            tcpResponse[0] = (header.transactionId >> 8) & 0xFF;
            tcpResponse[1] = header.transactionId & 0xFF;
            tcpResponse[2] = 0; // Protocol ID high byte
            tcpResponse[3] = 0; // Protocol ID low byte
            tcpResponse[4] = ((pduResponseLength + 1) >> 8) & 0xFF; // Length high byte
            tcpResponse[5] = (pduResponseLength + 1) & 0xFF; // Length low byte
            tcpResponse[6] = header.unitId; // Unit ID
            
            // Copy PDU response
            memcpy(&tcpResponse[7], pduResponse, pduResponseLength);
            
            sendModbusResponse(client, tcpResponse, 7 + pduResponseLength);
            return true;
        } else {
            // RTU communication failed
            sendModbusException(client, header.transactionId, header.unitId, pdu[0], MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE);
            return false;
        }
    }
    
    // Handle TCP-specific requests (unit ID 0xFF or 0)
    sendModbusException(client, header.transactionId, header.unitId, pdu[0], MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
    return false;
}

void ModbusTCPServer::sendModbusResponse(ModbusClientConnection& client, uint8_t* response, uint16_t length) {
    client.client.write(response, length);
    client.client.flush();
}

void ModbusTCPServer::sendModbusException(ModbusClientConnection& client, uint16_t transactionId, uint8_t unitId, uint8_t functionCode, uint8_t exceptionCode) {
    uint8_t response[9];
    
    // MBAP header
    response[0] = (transactionId >> 8) & 0xFF;
    response[1] = transactionId & 0xFF;
    response[2] = 0; // Protocol ID high byte
    response[3] = 0; // Protocol ID low byte
    response[4] = 0; // Length high byte
    response[5] = 3; // Length low byte (unit ID + function code + exception code)
    response[6] = unitId;
    
    // Exception response
    response[7] = functionCode | 0x80; // Set exception bit
}

bool ModbusTCPServer::handleReadRequest(uint8_t slaveId, uint8_t functionCode, uint16_t startAddress, 
                                       uint16_t quantity, uint8_t* response, uint16_t& responseLength) {
    // Find the board with matching slave ID
    int boardIndex = -1;
    for (int i = 0; i < 16; i++) {
        if (thermocoupleIO_index.tcIO[i].slaveID == slaveId && thermocoupleIO_index.tcIO[i].configInitialised) {
            boardIndex = i;
            break;
        }
    }
    
    if (boardIndex == -1) {
        // Slave not found
        return false;
    }
    
    thermocoupleIO_t* board = &thermocoupleIO_index.tcIO[boardIndex];
    
    // Build response based on function code
    response[0] = functionCode;
    
    switch (functionCode) {
        case 0x01: // Read Coils
            if (startAddress + quantity > 32) return false;
            response[1] = (quantity + 7) / 8; // Byte count
            responseLength = 2 + response[1];
            
            // Pack coil data into bytes
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t byteIndex = 2 + (i / 8);
                uint16_t bitIndex = i % 8;
                if (byteIndex >= responseLength) response[byteIndex] = 0;
                
                bool coilValue = false;
                if (startAddress + i < 8) {
                    coilValue = board->reg.alertEnable[startAddress + i];
                } else if (startAddress + i < 16) {
                    coilValue = board->reg.outputEnable[startAddress + i - 8];
                } else if (startAddress + i < 24) {
                    coilValue = board->reg.alertLatch[startAddress + i - 16];
                } else if (startAddress + i < 32) {
                    coilValue = board->reg.alertEdge[startAddress + i - 24];
                }
                
                if (coilValue) {
                    response[byteIndex] |= (1 << bitIndex);
                }
            }
            break;
            
        case 0x02: // Read Discrete Inputs
            if (startAddress + quantity > 32) return false;
            response[1] = (quantity + 7) / 8; // Byte count
            responseLength = 2 + response[1];
            
            // Pack discrete input data into bytes
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t byteIndex = 2 + (i / 8);
                uint16_t bitIndex = i % 8;
                if (byteIndex >= responseLength) response[byteIndex] = 0;
                
                bool inputValue = false;
                if (startAddress + i < 8) {
                    inputValue = board->reg.outputState[startAddress + i];
                } else if (startAddress + i < 16) {
                    inputValue = board->reg.alarmState[startAddress + i - 8];
                } else if (startAddress + i < 24) {
                    inputValue = board->reg.openCircuit[startAddress + i - 16];
                } else if (startAddress + i < 32) {
                    inputValue = board->reg.shortCircuit[startAddress + i - 24];
                }
                
                if (inputValue) {
                    response[byteIndex] |= (1 << bitIndex);
                }
            }
            break;
            
        case 0x03: // Read Holding Registers
            if (startAddress + quantity > 42) return false;
            response[1] = quantity * 2; // Byte count
            responseLength = 2 + response[1];
            
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t regAddress = startAddress + i;
                uint16_t value = 0;
                
                if (regAddress == 0) {
                    value = board->reg.status;
                } else if (regAddress == 1) {
                    value = board->reg.boardType;
                } else if (regAddress >= 2 && regAddress <= 8) {
                    // Board name (7 registers)
                    uint16_t nameIndex = (regAddress - 2) * 2;
                    if (nameIndex < 14) {
                        value = (board->reg.boardName[nameIndex] << 8) | board->reg.boardName[nameIndex + 1];
                    }
                } else if (regAddress == 9) {
                    value = board->reg.slaveID;
                } else if (regAddress >= 10 && regAddress <= 17) {
                    // Thermocouple types
                    value = board->reg.type[regAddress - 10];
                } else if (regAddress >= 18 && regAddress <= 33) {
                    // Alert setpoints (float values, 2 registers each)
                    uint16_t channelIndex = (regAddress - 18) / 2;
                    uint16_t registerPart = (regAddress - 18) % 2;
                    if (channelIndex < 8) {
                        uint32_t floatBits = *(uint32_t*)&board->reg.alertSP[channelIndex];
                        value = (registerPart == 0) ? (floatBits >> 16) : (floatBits & 0xFFFF);
                    }
                } else if (regAddress >= 34 && regAddress <= 41) {
                    // Alarm hysteresis
                    value = board->reg.alarmHyst[regAddress - 34];
                }
                
                // Pack as big-endian
                response[2 + i * 2] = (value >> 8) & 0xFF;
                response[2 + i * 2 + 1] = value & 0xFF;
            }
            break;
            
        case 0x04: // Read Input Registers
            if (startAddress + quantity > 48) return false;
            response[1] = quantity * 2; // Byte count
            responseLength = 2 + response[1];
            
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t regAddress = startAddress + i;
                uint16_t value = 0;
                
                if (regAddress >= 0 && regAddress <= 15) {
                    // Temperature values (float, 2 registers each)
                    uint16_t channelIndex = regAddress / 2;
                    uint16_t registerPart = regAddress % 2;
                    if (channelIndex < 8) {
                        uint32_t floatBits = *(uint32_t*)&board->reg.temperature[channelIndex];
                        value = (registerPart == 0) ? (floatBits >> 16) : (floatBits & 0xFFFF);
                    }
                } else if (regAddress >= 16 && regAddress <= 31) {
                    // Cold junction values
                    uint16_t channelIndex = (regAddress - 16) / 2;
                    uint16_t registerPart = (regAddress - 16) % 2;
                    if (channelIndex < 8) {
                        uint32_t floatBits = *(uint32_t*)&board->reg.coldJunction[channelIndex];
                        value = (registerPart == 0) ? (floatBits >> 16) : (floatBits & 0xFFFF);
                    }
                } else if (regAddress >= 32 && regAddress <= 47) {
                    // Delta junction values
                    uint16_t channelIndex = (regAddress - 32) / 2;
                    uint16_t registerPart = (regAddress - 32) % 2;
                    if (channelIndex < 8) {
                        uint32_t floatBits = *(uint32_t*)&board->reg.deltaJunction[channelIndex];
                        value = (registerPart == 0) ? (floatBits >> 16) : (floatBits & 0xFFFF);
                    }
                }
                
                // Pack as big-endian
                response[2 + i * 2] = (value >> 8) & 0xFF;
                response[2 + i * 2 + 1] = value & 0xFF;
            }
            break;
            
        default:
            return false; // Unsupported function code
    }
    
    return true;
}

uint16_t ModbusTCPServer::calculateCRC16(uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

int ModbusTCPServer::getConnectedClientCount() {
    int count = 0;
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        if (_clients[i].active) {
            count++;
        }
    }
    return count;
}

String ModbusTCPServer::getClientInfo(int index) {
    if (index >= 0 && index < MAX_MODBUS_CLIENTS && _clients[index].active) {
        uint32_t connectionDuration = millis() - _clients[index].connectionTime;
        uint32_t lastActivityTime = millis() - _clients[index].lastActivity;
        
        String info = "IP: " + _clients[index].clientIP;
        info += ", Connected: " + String(connectionDuration / 1000) + "s";
        info += ", Last Activity: " + String(lastActivityTime / 1000) + "s ago";
        return info;
    }
    return "";
}

uint16_t ModbusTCPServer::swapBytes(uint16_t value) {
    return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

void ModbusTCPServer::disconnectAllClients() {
    for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
        if (_clients[i].active) {
            _clients[i].client.stop();
            _clients[i].active = false;
            _clients[i].clientIP = "";
        }
    }
}

void ModbusTCPServer::setEnabled(bool enabled) {
    if (enabled != _config.enabled) {
        log(LOG_INFO, true, "Modbus TCP enabled changing from %s to %s\n", 
            _config.enabled ? "true" : "false", enabled ? "true" : "false");
        _config.enabled = enabled;
        
        if (!enabled && _running) {
            log(LOG_INFO, true, "Stopping Modbus TCP server (disabled)\n");
            stop();
        } else if (enabled && !_running) {
            log(LOG_INFO, true, "Starting Modbus TCP server (enabled) on port %d\n", _config.port);
            begin(_config.port);
        }
    }
}

bool ModbusTCPServer::isEnabled() const {
    return _config.enabled;
}

void ModbusTCPServer::setPort(uint16_t port) {
    if (port != _config.port) {
        log(LOG_INFO, true, "Modbus TCP port changing from %d to %d\n", _config.port, port);
        _config.port = port;
        if (_running) {
            // Restart server with new port
            log(LOG_INFO, true, "Restarting Modbus TCP server with new port %d\n", port);
            stop();
            begin(port);
        } else {
            log(LOG_INFO, true, "Modbus TCP server not running, port will be applied on next start\n");
        }
    } else {
        log(LOG_DEBUG, true, "Modbus TCP port unchanged: %d\n", port);
    }
}

uint16_t ModbusTCPServer::getPort() const {
    return _config.port;
}

// Global functions implementation
void init_modbus_tcp() {
    log(LOG_INFO, true, "Initializing Modbus TCP...\n");
    
    // Use network configuration instead of separate Modbus TCP config
    modbusTCPConfig.port = networkConfig.modbusTcpPort;
    modbusTCPConfig.enabled = true; // Always enabled, controlled by network config
    
    log(LOG_INFO, true, "Using network config: port=%d, enabled=%s\n", 
        modbusTCPConfig.port, modbusTCPConfig.enabled ? "true" : "false");
    
    if (modbusTCPConfig.enabled) {
        log(LOG_INFO, true, "Starting Modbus TCP server on port %d\n", modbusTCPConfig.port);
        if (modbusServer.begin(modbusTCPConfig.port)) {
            log(LOG_INFO, true, "Modbus TCP server initialized successfully on port %d\n", modbusTCPConfig.port);
        } else {
            log(LOG_ERROR, true, "Failed to initialize Modbus TCP server on port %d\n", modbusTCPConfig.port);
        }
    } else {
        log(LOG_INFO, true, "Modbus TCP server disabled in config\n");
    }
}

void manage_modbus_tcp() {
    modbusServer.poll();
}

bool loadModbusTCPConfig() {
    // Set defaults
    modbusTCPConfig.port = MODBUS_TCP_DEFAULT_PORT;
    modbusTCPConfig.enabled = true;
    
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS for Modbus TCP config\n");
        return false;
    }
    
    if (!LittleFS.exists("/modbus_tcp_config.json")) {
        log(LOG_INFO, true, "Modbus TCP config file not found, using defaults\n");
        LittleFS.end();
        saveModbusTCPConfig(); // Create default config file
        return true;
    }
    
    File configFile = LittleFS.open("/modbus_tcp_config.json", "r");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open Modbus TCP config file\n");
        LittleFS.end();
        return false;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    LittleFS.end();
    
    if (error) {
        log(LOG_WARNING, true, "Failed to parse Modbus TCP config file: %s\n", error.c_str());
        return false;
    }
    
    uint16_t configPort = doc["port"] | MODBUS_TCP_DEFAULT_PORT;
    bool configEnabled = doc["enabled"] | true;
    
    log(LOG_INFO, true, "Modbus TCP config file values: port=%d, enabled=%s\n", 
        configPort, configEnabled ? "true" : "false");
    
    modbusTCPConfig.port = configPort;
    modbusTCPConfig.enabled = configEnabled;
    
    log(LOG_INFO, true, "Modbus TCP config loaded: port=%d, enabled=%s\n", 
        modbusTCPConfig.port, modbusTCPConfig.enabled ? "true" : "false");
    
    return true;
}

void saveModbusTCPConfig() {
    log(LOG_INFO, true, "Saving Modbus TCP config: port=%d, enabled=%s\n", 
        modbusTCPConfig.port, modbusTCPConfig.enabled ? "true" : "false");
    
    if (!LittleFS.begin()) {
        log(LOG_WARNING, true, "Failed to mount LittleFS for saving Modbus TCP config\n");
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["port"] = modbusTCPConfig.port;
    doc["enabled"] = modbusTCPConfig.enabled;
    
    File configFile = LittleFS.open("/modbus_tcp_config.json", "w");
    if (!configFile) {
        log(LOG_WARNING, true, "Failed to open Modbus TCP config file for writing\n");
        LittleFS.end();
        return;
    }
    
    size_t bytesWritten = serializeJson(doc, configFile);
    if (bytesWritten == 0) {
        log(LOG_WARNING, true, "Failed to write Modbus TCP config file\n");
    } else {
        log(LOG_INFO, true, "Modbus TCP config saved successfully (%d bytes)\n", bytesWritten);
    }
    
    configFile.close();
    LittleFS.end();
}

void setupModbusTCPAPI() {
    // Get Modbus TCP status
    server.on("/api/modbus-tcp/status", HTTP_GET, []() {
        StaticJsonDocument<512> doc;
        
        doc["enabled"] = modbusTCPConfig.enabled;
        doc["port"] = networkConfig.modbusTcpPort; // Use network config port
        doc["running"] = modbusServer.isEnabled();
        doc["connectedClients"] = modbusServer.getConnectedClientCount();
        
        JsonArray clients = doc.createNestedArray("clients");
        for (int i = 0; i < MAX_MODBUS_CLIENTS; i++) {
            String clientInfo = modbusServer.getClientInfo(i);
            if (clientInfo.length() > 0) {
                clients.add(clientInfo);
            }
        }
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    // Update Modbus TCP configuration
    server.on("/api/modbus-tcp/config", HTTP_POST, []() {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"No data received\"}");
            return;
        }
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (error) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        // Update configuration
        if (doc.containsKey("port")) {
            uint16_t newPort = doc["port"];
            log(LOG_INFO, true, "Modbus TCP config update: port change requested to %d\n", newPort);
            if (newPort >= 1 && newPort <= 65535) {
                uint16_t oldPort = networkConfig.modbusTcpPort;
                networkConfig.modbusTcpPort = newPort;
                modbusTCPConfig.port = newPort; // Update local config too
                log(LOG_INFO, true, "Modbus TCP config: port updated from %d to %d\n", oldPort, newPort);
                modbusServer.setPort(newPort);
                
                // Save network configuration
                saveNetworkConfig();
            } else {
                log(LOG_WARNING, true, "Modbus TCP config: invalid port %d rejected\n", newPort);
            }
        }
        
        if (doc.containsKey("enabled")) {
            bool newEnabled = doc["enabled"];
            log(LOG_INFO, true, "Modbus TCP config update: enabled change requested to %s\n", newEnabled ? "true" : "false");
            modbusTCPConfig.enabled = newEnabled;
            modbusServer.setEnabled(modbusTCPConfig.enabled);
        }
        
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Modbus TCP configuration updated\"}");
    });
}
