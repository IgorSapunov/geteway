#include "RawRS485.h"
#include "Settings.h"
#include "web_interface/WebBuilder.h" // For keys

RawRS485* RawRS485::_instance = nullptr;

RawRS485::RawRS485(GyverDBFile* db) : _db(db) {
    _instance = this;
}

RawRS485* RawRS485::instance() {
    return _instance;
}

uint8_t RawRS485::takePeopleEvent() {
    uint8_t v = _peopleEvent;
    _peopleEvent = 0;
    return v;
}

bool RawRS485::getLastSignificantByte(uint8_t& byte, int8_t& people) const {
    if (!_hasLastSig) return false;
    byte = _lastSigByte;
    people = _lastPeople;
    return true;
}

bool RawRS485::getRxStats(uint32_t& totalBytes, uint8_t& lastByte, uint32_t& msSinceLastRx) const {
    totalBytes = _rxTotal;
    if (!_hasLastByte) return false;
    lastByte = _lastByte;
    msSinceLastRx = millis() - _lastRxMs;
    return true;
}

void RawRS485::begin() {
    // Initialize Raw RS-485 stream
    Serial2.begin(115200, SERIAL_8N1, RS485_DIRECT_RX_PIN, RS485_DIRECT_TX_PIN);

    if (_db) {
        _db->init(keys::byteRecognize0Person, 0xFE);
        _db->init(keys::byteRecognize1Person, 0xFC);
        _db->init(keys::byteRecognize2Person, 0xF8);
    }
}

void RawRS485::loop() {
    static uint32_t lastConfigUpdate = 0;
    static int code0 = 0xFE;
    static int code1 = 0xFC;
    static int code2 = 0xF8;
    static bool waitingForSignigicant = false;
#if RAW_RS485_LOG
    static uint32_t lastPrintMs = 0;
    static uint8_t lineCount = 0;
#endif

    // Update config cache every 1 second
    if (millis() - lastConfigUpdate > 1000) {
        lastConfigUpdate = millis();
        if (_db) {
            if (_db->has(keys::byteRecognize0Person)) code0 = _db->get(keys::byteRecognize0Person).toInt();
            if (_db->has(keys::byteRecognize1Person)) code1 = _db->get(keys::byteRecognize1Person).toInt();
            if (_db->has(keys::byteRecognize2Person)) code2 = _db->get(keys::byteRecognize2Person).toInt();
        }
    }

    // Process all available bytes
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        _rxTotal++;
        _hasLastByte = true;
        _lastByte = byte;
        _lastRxMs = millis();

#if RAW_RS485_LOG
        const uint32_t now = millis();
        if (lineCount && now - lastPrintMs > 50) {
            Serial.println();
            lineCount = 0;
        }
        Serial.printf("%02X ", (unsigned)byte);
        lineCount++;
        lastPrintMs = now;
        if (byte == 0xFF || lineCount >= 16) {
            Serial.println();
            lineCount = 0;
        }
#endif

        if (waitingForSignigicant) {
            waitingForSignigicant = false;
            _hasLastSig = true;
            _lastSigByte = byte;
            _lastPeople = -1;
            // This is the significant byte
            if (byte == (uint8_t)code0) {
#if RAW_RS485_LOG
                Serial.println("RS485: Detected 0 Person");
#endif
                _lastPeople = 0;
            } else if (byte == code1) {
#if RAW_RS485_LOG
                Serial.println("RS485: Detected 1 Person");
#endif
                _peopleEvent = 1;
                _lastPeople = 1;
            } else if (byte == code2) {
#if RAW_RS485_LOG
                Serial.println("RS485: Detected 2 People");
#endif
                _peopleEvent = 2;
                _lastPeople = 2;
            } else {
#if RAW_RS485_LOG
                Serial.printf("RS485: Unknown significant byte: 0x%02X\n", byte);
#endif
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
