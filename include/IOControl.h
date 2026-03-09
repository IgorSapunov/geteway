#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include <Arduino.h>
#include <Wire.h>
#include "PCF8574.h"
#include "Settings.h"

class IOControl {
public:
    IOControl();
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
    PCF8574 _relaysL;
    PCF8574 _relaysR;
    PCF8574 _inputsL;
    PCF8574 _inputsR;

    // Helper to map index to specific PCF and pin
    void _writeRelay(uint8_t id, bool state);
};

#endif // IO_CONTROL_H
