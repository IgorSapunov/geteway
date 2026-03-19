#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"
#include "Settings.h"

class IOControl {
public:
    IOControl();
    static IOControl* instance();
    void begin();
    void loop();

    // Outputs
    void setGreenLampEntryA(bool state);
    void setRedLampEntryA(bool state);
    void setGreenLampEntryB(bool state);
    void setRedLampEntryB(bool state);
    void setStatusEntryA(bool state);
    void setStatusEntryB(bool state);
    void setSoundEnter(bool state);
    void setSoundLeave(bool state);
    void setSignalPass(bool state);

    // Inputs
    bool readOpenEntryDoorA();
    bool readOpenEntryDoorB();
    bool readFireSignal();

    // Generic access
    void setOutput(uint8_t id, bool state);
    bool readInput(uint8_t id);

private:
    static IOControl* _instance;
    PCF8574 _relaysL;
    PCF8574 _relaysR;
    PCF8574 _inputsL;
    PCF8574 _inputsR;

    void _writeRelay(uint8_t id, bool state);
    PCF8574& _relayDev(uint8_t id, uint8_t& pin);
    PCF8574& _inputDev(uint8_t id, uint8_t& pin);
};

#endif // IO_CONTROL_H
