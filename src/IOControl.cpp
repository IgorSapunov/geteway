#include "IOControl.h"

IOControl* IOControl::_instance = nullptr;

IOControl::IOControl() 
    : _relaysL(PCF_ADDR_RELAYS_L), _relaysR(PCF_ADDR_RELAYS_R),
      _inputsL(PCF_ADDR_INPUTS_L), _inputsR(PCF_ADDR_INPUTS_R) 
{
    _instance = this;
}

IOControl* IOControl::instance() {
    return _instance;
}

void IOControl::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Initialize Relay Expanders
    _relaysL.begin();
    _relaysR.begin();
    
    // Initialize Input Expanders (PCF8574 starts as input/HIGH by default usually, but ensuring it)
    _inputsL.begin();
    _inputsR.begin();
    
    // Ensure inputs are set to HIGH (Input mode for PCF8574)
    // Note: RobTillaart library begin() might already do this, but being explicit is safe
    // PCF8574 IOs are quasi-bidirectional. Writing 1 enables weak pull-up (Input).
    // Writing 0 drives it LOW (Output).
    // So for inputs, we want all 1s.
    _inputsL.write8(0xFF);
    _inputsR.write8(0xFF);
    
    _relaysL.write8(0xFF);
    _relaysR.write8(0xFF);
    
    // Configure Status LED
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, !LED_ACTIVE_LOW); // Turn off initially
}

void IOControl::loop() {
    // Poll inputs or update states if needed
    // Currently nothing to loop for basic IO
}

// ==========================================
// Generic Access
// ==========================================

void IOControl::setOutput(uint8_t id, bool state) {
    if (id >= 16) return;
    uint8_t pin = 0;
    PCF8574& dev = _relayDev(id, pin);
    dev.write(pin, !state);
}

bool IOControl::readInput(uint8_t id) {
    if (id >= 16) return true;
    uint8_t pin = 0;
    PCF8574& dev = _inputDev(id, pin);
    return dev.read(pin);
}

// ==========================================
// Named Output Methods
// ==========================================

void IOControl::setGreenLampEntryA(bool state) {
    setOutput(DO_GREEN_LAMP_ENTRY_A, state);
}

void IOControl::setRedLampEntryA(bool state) {
    setOutput(DO_RED_LAMP_ENTRY_A, state);
}

void IOControl::setGreenLampEntryB(bool state) {
    setOutput(DO_GREEN_LAMP_ENTRY_B, state);
}

void IOControl::setRedLampEntryB(bool state) {
    setOutput(DO_RED_LAMP_ENTRY_B, state);
}

void IOControl::setStatusEntryA(bool state) {
    setOutput(DO_STATUS_ENTRY_A, state);
}

void IOControl::setStatusEntryB(bool state) {
    setOutput(DO_STATUS_ENTRY_B, state);
}

void IOControl::setSoundEnter(bool state) {
    setOutput(DO_SOUND_ENTER, state);
}

void IOControl::setSoundLeave(bool state) {
    setOutput(DO_SOUND_LEAVE, state);
}

void IOControl::setSignalPass(bool state) {
    setOutput(DO_SIGNAL_PASS, state);
}

// ==========================================
// Named Input Methods
// ==========================================

bool IOControl::readOpenEntryDoorA() {
    return readInput(DI_OPEN_ENTRY_DOOR_A);
}

bool IOControl::readOpenEntryDoorB() {
    return readInput(DI_OPEN_ENTRY_DOOR_B);
}

bool IOControl::readFireSignal() {
    return readInput(DI_FIRE_SIGNAL);
}

// Private helper (not strictly needed if using library methods directly, but kept for structure)
void IOControl::_writeRelay(uint8_t id, bool state) {
    setOutput(id, state);
}

PCF8574& IOControl::_relayDev(uint8_t id, uint8_t& pin) {
    if (id < 8) {
        pin = id;
        return _relaysL;
    }
    pin = id - 8;
    return _relaysR;
}

PCF8574& IOControl::_inputDev(uint8_t id, uint8_t& pin) {
    if (id < 8) {
        pin = id;
        return _inputsL;
    }
    pin = id - 8;
    return _inputsR;
}
