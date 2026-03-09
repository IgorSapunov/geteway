#include "ModbusRS485.h"

ModbusRS485::ModbusRS485() 
    : _pollState(0), _lastPollTime(0), _mutex(NULL) 
{
    memset(_regsEntryA, 0, sizeof(_regsEntryA));
    memset(_regsEntryB, 0, sizeof(_regsEntryB));
}

void ModbusRS485::begin() {
    // Create Mutex
    _mutex = xSemaphoreCreateMutex();

    // Setup Serial
    // Note: Verify if Serial1 is used correctly on your board. 
    // ESP32 allows remapping.
    Serial1.begin(RS485_BAUD_RATE, RS485_CONFIG, RS485_RX_PIN, RS485_TX_PIN);
    
    // Setup Modbus
    mb.begin(&Serial1, RS485_DE_RE_PIN);
    mb.master(); // Set as Master

    // Create Task
    xTaskCreatePinnedToCore(
        _taskModbus,
        "ModbusTask",
        TASK_STACK_SIZE_MODBUS,
        this,
        TASK_PRIORITY_MODBUS,
        NULL,
        1 // Core 1
    );
}

void ModbusRS485::loop() {
    // Logic moved to _taskModbus
    // Keep empty or minimal
}

void ModbusRS485::_taskModbus(void *pvParameters) {
    ModbusRS485* instance = (ModbusRS485*)pvParameters;
    
    for (;;) {
        if (xSemaphoreTake(instance->_mutex, portMAX_DELAY)) {
            instance->mb.task();
            instance->_pollLogic();
            xSemaphoreGive(instance->_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to other tasks
    }
}

void ModbusRS485::_pollLogic() {
    // Basic polling scheduler
    // At 9600 baud, 1 byte takes ~1ms. 
    // A frame of 50 registers (100 bytes data + overhead) takes ~110-120ms.
    // We set interval to 150ms to be safe.
    
    if (millis() - _lastPollTime > 150) {
        _lastPollTime = millis();
        
        uint16_t* targetBuffer;
        uint8_t targetSlave;
        uint16_t startReg;
        uint16_t chunkSize = 50; 
        
        // Total chunks per slave = 350 / 50 = 7
        uint8_t chunksPerSlave = (MODBUS_REG_COUNT + chunkSize - 1) / chunkSize;
        
        // Determine current slave and chunk
        // 0..(chunksPerSlave-1) -> Slave A
        // chunksPerSlave..(2*chunksPerSlave-1) -> Slave B
        
        uint8_t totalStates = chunksPerSlave * 2;
        
        if (_pollState >= totalStates) _pollState = 0;
        
        if (_pollState < chunksPerSlave) {
            targetSlave = MODBUS_SLAVE_ID_ENTRY_A;
            targetBuffer = _regsEntryA;
            startReg = _pollState * chunkSize;
        } else {
            targetSlave = MODBUS_SLAVE_ID_ENTRY_B;
            targetBuffer = _regsEntryB;
            startReg = (_pollState - chunksPerSlave) * chunkSize;
        }
        
        // Adjust size for last chunk
        if (startReg + chunkSize > MODBUS_REG_COUNT) {
            chunkSize = MODBUS_REG_COUNT - startReg;
        }
        
        // Issue Read Command
        // We pass the pointer to the specific offset in our local array
        // The library will write directly to it upon response
        mb.readHreg(targetSlave, startReg, &targetBuffer[startReg], chunkSize);
        
        _pollState++;
    }
}

// Thread-safe access to local cache
uint16_t ModbusRS485::readCache(uint8_t slaveId, uint16_t address) {
    uint16_t value = 0;
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        if (address < MODBUS_REG_COUNT) {
            if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
                value = _regsEntryA[address];
            } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
                value = _regsEntryB[address];
            }
        }
        xSemaphoreGive(_mutex);
    }
    return value;
}

// Immediate write (High Priority)
bool ModbusRS485::writeRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    bool success = false;
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        // Send Write Command immediately
        // This adds to the transaction queue.
        // If the queue is processed in order, it will go out after current pending reads.
        // Since we limit our polling rate (one request every 150ms), the queue is likely empty or has max 1 item.
        // So this should be fast.
        
        // Update local cache optimistically? 
        // Better wait for read back, but for UI feedback we might want to update cache.
        // Let's update cache so UI sees it immediately.
        if (address < MODBUS_REG_COUNT) {
            if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
                _regsEntryA[address] = value;
            } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
                _regsEntryB[address] = value;
            }
        }
        
        success = (mb.writeHreg(slaveId, address, value) != 0);
        
        xSemaphoreGive(_mutex);
    }
    return success;
}

uint16_t* ModbusRS485::getCachePtr(uint8_t slaveId) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) return _regsEntryA;
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) return _regsEntryB;
    return nullptr;
}
