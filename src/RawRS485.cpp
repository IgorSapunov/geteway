#include "RawRS485.h"
#include "Settings.h"
#include "web_interface/WebBuilder.h" // For keys

RawRS485::RawRS485(GyverDBFile* db) : _db(db) {
    // Constructor
}

void RawRS485::begin() {
    // Initialize Raw RS-485 stream
    Serial2.begin(115200, SERIAL_8N1, RS485_DIRECT_RX_PIN, RS485_DIRECT_TX_PIN);
    
    // Initialize default values if not present
    if (!_db->has(keys::rs485Code1Person)) _db->set(keys::rs485Code1Person, 0xFC);
    if (!_db->has(keys::rs485Code2People)) _db->set(keys::rs485Code2People, 0xF8);
}

void RawRS485::loop() {
    static uint32_t lastConfigUpdate = 0;
    static int code1 = 0xFC;
    static int code2 = 0xF8;
    static bool waitingForSignigicant = false;

    // Update config cache every 1 second
    if (millis() - lastConfigUpdate > 1000) {
        lastConfigUpdate = millis();
        if (_db) {
            if (_db->has(keys::rs485Code1Person)) code1 = _db->get(keys::rs485Code1Person).toInt();
            if (_db->has(keys::rs485Code2People)) code2 = _db->get(keys::rs485Code2People).toInt();
        }
    }

    // Process all available bytes
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();

        if (waitingForSignigicant) {
            waitingForSignigicant = false;
            // This is the significant byte
            if (byte == code1) {
                Serial.println("RS485: Detected 1 Person");
                // TODO: Update a counter or trigger an event
            } else if (byte == code2) {
                Serial.println("RS485: Detected 2 People");
                // TODO: Update a counter or trigger an event
            } else {
                Serial.printf("RS485: Unknown significant byte: 0x%02X\n", byte);
            }
        } else {
            if (byte == 0xFF) {
                waitingForSignigicant = true;
            }
        }
    }
}

void RawRS485::processByte(uint8_t byte) {
    // Helper not used currently, logic is in loop for performance/caching
}
