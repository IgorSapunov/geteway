#pragma once
#include <Stamp.h>
#include <Arduino.h>
#include <GyverDBFile.h>
#include <SettingsGyver.h>

// Create hash keys using GyverDB macro - this creates an enum with hashed values
DB_KEYS(keys,
    wifiApSsid,
    wifiApPass,
    wifiApIp,
    
    // Digital Outputs (DO)
    doGreenLampEntryA,
    doRedLampEntryA,
    doGreenLampEntryB,
    doRedLampEntryB,
    doStatusEntryA,
    doStatusEntryB,
    doSoundEnter,
    doSoundLeave,
    doSignalPass,

    // Digital Inputs (DI)
    diOpenEntryDoorA,
    diOpenEntryDoorB,
    diFireSignal,
    diFireInvert,
    diResetFactory,
    diLowerSensor,

    // Modbus Slaves
    mbSlaveDriver1,
    mbSlaveDriver2,

    // China driver (Door control) - Driver A
    drvACmd,
    drvAOpenSpeed,
    drvACloseSpeed,
    drvAAccel,
    drvADecel,
    drvARegAddr,
    drvARegValue,

    // China driver (Door control) - Driver B
    drvBCmd,
    drvBOpenSpeed,
    drvBCloseSpeed,
    drvBAccel,
    drvBDecel,
    drvBRegAddr,
    drvBRegValue,

    // Web UI actions (not stored)
    uiAOpenExitBtn,
    uiAOpenEntryBtn,
    uiACloseBtn,
    uiAReadRegBtn,
    uiAWriteRegBtn,
    uiBOpenExitBtn,
    uiBOpenEntryBtn,
    uiBCloseBtn,
    uiBReadRegBtn,
    uiBWriteRegBtn,
    uiAStatusLbl,
    uiBStatusLbl,
    uiAWriteParamsBtn,
    uiBWriteParamsBtn,

    // People Counter Bytes
    byteRecognize0Person,
    byteRecognize1Person,
    byteRecognize2Person,

    // Modes
    modeAutoEn,
    modeTestEn,

    // Algorithm
    algoCompEn,
    algoSkipOpenConfirm,
    algoWaitSecondDoorEmpty,
    algoSecondDoorMaxHoldEn,
    algoSecondDoorMaxHoldMs,
    algoOpenSensorToutMs,
    algoPersonWaitToutMs,
    algoSwitchDelayMs,
    algoSecondDoorHoldMs,

    // Control buttons (not stored)
    uiTestOpenAEntryBtn,
    uiTestOpenBEntryBtn,
    uiTestFireBtn,
    uiTestNormalBtn,
    uiTestCloseBtn
)

class WebBuilder {

  public:
    WebBuilder();

    void setDbPointer(GyverDBFile* db);
    void setSsidPswd(String* ssid, String* pswd);
    void build(sets::Builder& b);
    void update(sets::Updater& u);
   

  private:
    GyverDBFile* _db = nullptr;
    String*      _ssid = nullptr;
    String*      _pswd = nullptr;
};
