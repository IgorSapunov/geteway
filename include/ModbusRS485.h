#ifndef MODBUS_RS485_H
#define MODBUS_RS485_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include <GyverDBFile.h>
#include "Settings.h"

class ModbusRS485 {
public:
    ModbusRS485(GyverDBFile* db);
    static ModbusRS485* instance();
    void begin();
    void loop();

    // Access methods for other modules (Thread-safe)
    uint16_t readCache(uint8_t slaveId, uint16_t address);
    bool writeRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    bool writeRegisters(uint8_t slaveId, uint16_t address, const uint16_t* values, uint16_t count);
    bool queueWriteRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    
    // Get pointer to full array (Use with caution, not thread-safe without external locking)
    uint16_t* getCachePtr(uint8_t slaveId);

private:
    static bool _onTransaction(Modbus::ResultCode event, uint16_t transactionId, void* data);
    bool _scheduleRead(uint8_t slaveId, uint16_t address, uint16_t count, uint16_t* dst);
    bool _scheduleWrite(uint8_t slaveId, uint16_t address, uint16_t value);
    bool _scheduleWriteMulti(uint8_t slaveId, uint16_t address, const uint16_t* values, uint16_t count);

    ModbusRTU mb;
    GyverDBFile* _db;
    
    // Local cache for holding registers
    uint16_t _regsEntryA[MODBUS_REG_COUNT];
    uint16_t _regsEntryB[MODBUS_REG_COUNT];

    bool _busy = false;
    uint32_t _lastPollStartMs = 0;
    uint32_t _lastCfgMs = 0;
    uint8_t _pollSlaveIdx = 0;
    uint16_t _pollAddr = 0;
    uint8_t _slaveIdA = MODBUS_SLAVE_ID_ENTRY_A;
    uint8_t _slaveIdB = MODBUS_SLAVE_ID_ENTRY_B;
};

#endif
