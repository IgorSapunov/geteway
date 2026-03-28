#ifndef SETTINGS_H
#define SETTINGS_H

/**
 * Global Firmware Settings and Configuration
 */

// ==========================================
// System Configuration
// ==========================================
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_NAME "ESP32-Gateway"

// Serial Debugging
#define DEBUG_ENABLED 1
#define DEBUG_SERIAL_BAUD 115200

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

#define MODBUS_RS485_LOG 1
#define RAW_RS485_LOG 0
#define DOOR_STATE_LOG 1
#define L_RELAYS_ADDR 0x24
#define R_RELAYS_ADDR 0x25
#define IO1_PIN 32
#define IO2_PIN 33
#define IO3_PIN 14


#define RXD_485_PIN 16
#define TXD_485_PIN 13  
// ==========================================
// Network / WiFi Settings
// ==========================================
// 0: Station Mode (Connect to Router), 1: Access Point Mode
// NOTE: Do not use WIFI_MODE_AP name here (conflicts with esp-idf WiFi enum).
#define GATEWAY_WIFI_MODE_AP 1

#define WIFI_SSID "Your_SSID"
#define WIFI_PASSWORD "Your_Password"

#define WIFI_AP_SSID "ESP32-Gateway-AP"
#define WIFI_AP_PASSWORD "12345678"

// Web Server Port
#define WEB_SERVER_PORT 80

// ==========================================
// RS485 / Modbus Settings
// ==========================================
#define RS485_BAUD_RATE 115200
#define RS485_CONFIG SERIAL_8N1

#define MODBUS_SLAVE_ID_ENTRY_A 1
#define MODBUS_SLAVE_ID_ENTRY_B 2
#define MODBUS_REG_COUNT 350

// Pin Definitions (Adjust for your specific board)
#define RS485_DIRECT_RX_PIN 32 
#define RS485_DIRECT_TX_PIN 33


// ==========================================
// IO Control Settings
// ==========================================
// I2C Configuration
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// PCF8574 Addresses
#define PCF_ADDR_RELAYS_L 0x24
#define PCF_ADDR_RELAYS_R 0x25
#define PCF_ADDR_INPUTS_L 0x21
#define PCF_ADDR_INPUTS_R 0x22

// Digital Outputs (DO) Mapping (0-15)
// Assuming sequential mapping based on user list
#define DO_GREEN_LAMP_ENTRY_A   0
#define DO_RED_LAMP_ENTRY_A     1
#define DO_GREEN_LAMP_ENTRY_B   2
#define DO_RED_LAMP_ENTRY_B     3
#define DO_STATUS_ENTRY_A       4
#define DO_STATUS_ENTRY_B       5
#define DO_SOUND_ENTER          6
#define DO_SOUND_LEAVE          7
#define DO_SIGNAL_PASS          8

// Digital Inputs (DI) Mapping (0-15)
// Assuming sequential mapping based on user list
#define DI_OPEN_ENTRY_DOOR_A    0
#define DI_OPEN_ENTRY_DOOR_B    1
#define DI_FIRE_SIGNAL          2

// Status LED
#define LED_STATUS_PIN 2
#define LED_ACTIVE_LOW 0 // Set to 1 if LED is active low

// ==========================================
// FreeRTOS Task Configuration
// ==========================================
#define TASK_STACK_SIZE_WEB 4096
#define TASK_PRIORITY_WEB 1

#define TASK_STACK_SIZE_MODBUS 2048
#define TASK_PRIORITY_MODBUS 2

#define TASK_STACK_SIZE_IO 2048
#define TASK_PRIORITY_IO 1

#endif // SETTINGS_H
