#ifndef MODBUS_RS485_H
#define MODBUS_RS485_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include "Settings.h"

class ModbusRS485 {
public:
    ModbusRS485();
    void begin();
    void loop(); // Kept for compatibility, but logic moved to task

    // Access methods for other modules (Thread-safe)
    uint16_t readCache(uint8_t slaveId, uint16_t address);
    bool writeRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    
    // Get pointer to full array (Use with caution, not thread-safe without external locking)
    uint16_t* getCachePtr(uint8_t slaveId);

private:
    ModbusRTU mb;
    SemaphoreHandle_t _mutex;
    
    // Local cache for holding registers
    uint16_t _regsEntryA[MODBUS_REG_COUNT];
    uint16_t _regsEntryB[MODBUS_REG_COUNT];

    // Polling state
    uint32_t _lastPollTime;
    uint8_t _pollState; 
    
    static void _taskModbus(void *pvParameters);
    void _pollLogic();
};

#endif
