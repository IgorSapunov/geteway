#ifndef RAW_RS485_H
#define RAW_RS485_H

#include <Arduino.h>
#include <GyverDBFile.h>

class RawRS485 {
public:
    RawRS485(GyverDBFile* db);
    static RawRS485* instance();
    void begin();
    void loop();
    uint8_t takePeopleEvent();
    bool getLastSignificantByte(uint8_t& byte, int8_t& people) const;
    bool getRxStats(uint32_t& totalBytes, uint8_t& lastByte, uint32_t& msSinceLastRx) const;

private:
    static RawRS485* _instance;
    GyverDBFile* _db;
    uint8_t _peopleEvent = 0;
    uint8_t _lastSigByte = 0;
    int8_t _lastPeople = -1;
    bool _hasLastSig = false;
    uint32_t _rxTotal = 0;
    uint8_t _lastByte = 0;
    bool _hasLastByte = false;
    uint32_t _lastRxMs = 0;
    void processByte(uint8_t byte);
};

#endif
