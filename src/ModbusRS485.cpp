#include "ModbusRS485.h"
#include "web_interface/WebBuilder.h" // For keys

ModbusRS485::ModbusRS485(GyverDBFile* db) : _db(db) {
    memset(_regsEntryA, 0, sizeof(_regsEntryA));
    memset(_regsEntryB, 0, sizeof(_regsEntryB));
}

void ModbusRS485::begin() {
    // Setup Serial
    // Note: Verify if Serial1 is used correctly on your board. 
    // ESP32 allows remapping.
    Serial1.begin(RS485_BAUD_RATE, RS485_CONFIG, RXD_485_PIN, TXD_485_PIN);
    
    // Setup Modbus
    mb.begin(&Serial1);
    mb.master(); // Set as Master
}

void ModbusRS485::loop() {
    mb.task();

    static uint32_t lastPoll = 0;
    bool isMaster = false;
    int slaveId = 1;

    // Get settings from DB
    if (_db) {
        if (_db->has(keys::rtuIsMaster)) isMaster = _db->get(keys::rtuIsMaster).toBool();
        if (_db->has(keys::rtuSlaveId)) slaveId = _db->get(keys::rtuSlaveId).toInt();
    }

    if (isMaster) {
        if (millis() - lastPoll >= 200) {
            lastPoll = millis();
            uint16_t tmp = 0;
            // Simplified polling logic as requested
            mb.readHreg(slaveId, 0, &tmp, 1);
        }
    }
}

uint16_t ModbusRS485::readCache(uint8_t slaveId, uint16_t address) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
        if (address < MODBUS_REG_COUNT) return _regsEntryA[address];
    } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
        if (address < MODBUS_REG_COUNT) return _regsEntryB[address];
    }
    return 0;
}

bool ModbusRS485::writeRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    // Basic implementation - in a real scenario, this would queue a write command
    // Since we are simplifying to a single loop, we can try to send it directly if idle
    // But for now, just updating local cache
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
        if (address < MODBUS_REG_COUNT) {
            _regsEntryA[address] = value;
            return true;
        }
    } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
        if (address < MODBUS_REG_COUNT) {
            _regsEntryB[address] = value;
            return true;
        }
    }
    return false;
}

uint16_t* ModbusRS485::getCachePtr(uint8_t slaveId) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) return _regsEntryA;
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) return _regsEntryB;
    return nullptr;
}
