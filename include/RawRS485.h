#ifndef RAW_RS485_H
#define RAW_RS485_H

#include <Arduino.h>
#include <GyverDBFile.h>

class RawRS485 {
public:
    RawRS485(GyverDBFile* db);
    void begin();
    void loop();

private:
    GyverDBFile* _db;
    void processByte(uint8_t byte);
};

#endif
