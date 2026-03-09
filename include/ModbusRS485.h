#ifndef MODBUS_RS485_H
#define MODBUS_RS485_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include <GyverDBFile.h>
#include "Settings.h"

class ModbusRS485 {
public:
    ModbusRS485(GyverDBFile* db);
    void begin();
    void loop();

    // Access methods for other modules (Thread-safe)
    uint16_t readCache(uint8_t slaveId, uint16_t address);
    bool writeRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    
    // Get pointer to full array (Use with caution, not thread-safe without external locking)
    uint16_t* getCachePtr(uint8_t slaveId);

private:
    ModbusRTU mb;
    GyverDBFile* _db;
    
    // Local cache for holding registers
    uint16_t _regsEntryA[MODBUS_REG_COUNT];
    uint16_t _regsEntryB[MODBUS_REG_COUNT];
};

#endif
