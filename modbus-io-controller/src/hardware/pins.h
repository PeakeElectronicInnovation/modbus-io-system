#pragma once

// Hardware pin definitions

// Ethernet pins
#define PIN_ETH_MISO 0
#define PIN_ETH_CS 1
#define PIN_ETH_SCK 2
#define PIN_ETH_MOSI 3
#define PIN_ETH_RST 4
#define PIN_ETH_IRQ 5

// RTC pins
#define PIN_RTC_SDA 6
#define PIN_RTC_SCL 7

// SD card pins
#define PIN_SDIO_CLK 10
#define PIN_SDIO_CMD 11
#define PIN_SDIO_D0 12
#define PIN_SD_SCK 10
#define PIN_SD_MOSI 11
#define PIN_SD_MISO 12
#define PIN_SD_D1 13
#define PIN_SD_D2 14
#define PIN_SD_CS 15
#define PIN_SD_CD 18

// RS485 bus 1 pins
#define PIN_RS485_TX_1 16
#define PIN_RS485_RX_1 17

// RS485 bus 2 pins
#define PIN_RS485_TX_2 8
#define PIN_RS485_RX_2 9

// WS2812b LED pin
#define PIN_LED_DAT 19

// Local outputs
#define PIN_ALM_LED 20
#define PIN_ALM_SOUNDER 21

// Power supply feedback
#define PIN_PSU_FB 29   // ADC 4

// Spare GPIO pins
#define PIN_GPIO_22 22
#define PIN_GPIO_23 23
#define PIN_GPIO_24 24
#define PIN_GPIO_25 25
#define PIN_GPIO_26 26  // ADC 1
#define PIN_GPIO_27 27  // ADC 2
#define PIN_GPIO_28 28  // ADC 3