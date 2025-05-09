# Modbus IO system
A (WIP) family of PCBs designed to form a simple and flexible control system. Communication between boards is based on the Modbus RTU protocol but at higher speeds (500kbps). System consists of a master controller with two RS485 ports, on-board SD card for lata logging, and Ethernet interface for web-mamangement and access by external systems via Modbus TCP or MQTT. Externsion boards are addressed automatically when added through the master control board, so all configuration is completed through the web interface. Boards have a standardised set of holding registers in the first 10 FC03 registers (0x0000 -> 0x0009):

## Modbus structure
| Register name | Address | Type     | Size (uint16) | Notes                          |
|---------------|---------|----------|---------------|--------------------------------|
| Status        | 0x0000  | uint16   | 1             | See bit positions below        |
| Board type    | 0x0001  | uint16   | 1             | See type IDs below             |
| Board name    | 0x0002  | char[14] | 7             | User defined, 14 chars max     |
| Slave ID      | 0x0009  | uint16   | 1             | 1 - 244, 245 is config address |

### Board type
Different board types have a unique ID to allow the master controller to confirm the board type before writing configration data. Board types are (so far):

| Board type            | Code   |
|-----------------------|--------|
| Master controller     | 0x0000 |
| Analogue/digital IO   | 0x0001 |
| Thermocouple IO       | 0x0002 |
| RTD IO                | 0x0003 |
| Energy meter          | 0x0004 |

### Status bits
| Bit (MSB -> LSB) | Description                       |
|------------------|-----------------------------------|
| 15-3             | Unassigned (ignore)               |
| 2                | Power supply voltage out of range |
| 1                | Internal device comm error        |
| 0                | Modbus packet error               |

## Development stage
Development is in very early stages as of right now (April 2025).