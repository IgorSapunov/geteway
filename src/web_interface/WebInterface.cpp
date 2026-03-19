#include "web_interface/WebInterface.h"
#include "Settings.h"
#include <WiFi.h>

WebInterface *WebInterface::instance = nullptr;

WebInterface::WebInterface(GyverDBFile &_db, String *ssid, String *pswd) : db(_db),
                                                                           wifiServer(std::make_unique<WiFiServer>(WEB_PORT_WIFI)), _ssid(ssid), _pswd(pswd)
{
    instance = this;

    builder.setDbPointer(&db);
    builder.setSsidPswd(ssid, pswd);
    settingsGyver.attachDB(&db);
}

void WebInterface::begin()
{
    db.init(keys::wifiApSsid, "GATEWAY_AP");
    db.init(keys::wifiApPass, "12345678");
    db.init(keys::wifiApIp, "192.168.4.1");

    db.init(keys::doGreenLampEntryA, DO_GREEN_LAMP_ENTRY_A);
    db.init(keys::doRedLampEntryA, DO_RED_LAMP_ENTRY_A);
    db.init(keys::doGreenLampEntryB, DO_GREEN_LAMP_ENTRY_B);
    db.init(keys::doRedLampEntryB, DO_RED_LAMP_ENTRY_B);
    db.init(keys::doStatusEntryA, DO_STATUS_ENTRY_A);
    db.init(keys::doStatusEntryB, DO_STATUS_ENTRY_B);
    db.init(keys::doSoundEnter, DO_SOUND_ENTER);
    db.init(keys::doSoundLeave, DO_SOUND_LEAVE);
    db.init(keys::doSignalPass, DO_SIGNAL_PASS);

    db.init(keys::diOpenEntryDoorA, DI_OPEN_ENTRY_DOOR_A);
    db.init(keys::diOpenEntryDoorB, DI_OPEN_ENTRY_DOOR_B);
    db.init(keys::diFireSignal, DI_FIRE_SIGNAL);
    db.init(keys::diFireInvert, false);
    db.init(keys::diResetFactory, 3);
    db.init(keys::diLowerSensor, 4);

    db.init(keys::mbSlaveDriver1, MODBUS_SLAVE_ID_ENTRY_A);
    db.init(keys::mbSlaveDriver2, MODBUS_SLAVE_ID_ENTRY_B);

    db.init(keys::drvACmd, 0);
    db.init(keys::drvAOpenSpeed, 0);
    db.init(keys::drvACloseSpeed, 0);
    db.init(keys::drvAAccel, 0);
    db.init(keys::drvADecel, 0);
    db.init(keys::drvARegAddr, 324);
    db.init(keys::drvARegValue, 0);

    db.init(keys::drvBCmd, 0);
    db.init(keys::drvBOpenSpeed, 0);
    db.init(keys::drvBCloseSpeed, 0);
    db.init(keys::drvBAccel, 0);
    db.init(keys::drvBDecel, 0);
    db.init(keys::drvBRegAddr, 324);
    db.init(keys::drvBRegValue, 0);

    db.init(keys::byteRecognize0Person, 0xFE);
    db.init(keys::byteRecognize1Person, 0xFC);
    db.init(keys::byteRecognize2Person, 0xF8);

    db.init(keys::modeAutoEn, true);
    db.init(keys::modeTestEn, false);

    db.init(keys::algoCompEn, false);
    db.init(keys::algoSkipOpenConfirm, false);
    db.init(keys::algoWaitSecondDoorEmpty, true);
    db.init(keys::algoSecondDoorMaxHoldEn, false);
    db.init(keys::algoSecondDoorMaxHoldMs, 5000);
    db.init(keys::algoOpenSensorToutMs, 5000);
    db.init(keys::algoPersonWaitToutMs, 15000);
    db.init(keys::algoSwitchDelayMs, 100);
    db.init(keys::algoSecondDoorHoldMs, 100);

    const String apSsid = db.get(keys::wifiApSsid).toString();
    const String apPass = db.get(keys::wifiApPass).toString();
    const String apIpStr = db.get(keys::wifiApIp).toString();

    IPAddress apIp;
    apIp.fromString(apIpStr);
    const IPAddress apMask(255, 255, 255, 0);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    WiFi.softAPConfig(apIp, apIp, apMask);

    wifiServer->begin();

    settingsGyver.begin(wifiServer.get());
    settingsGyver.setTitle(title);
    settingsGyver.setVersion(title);
    settingsGyver.onBuild(staticBuild);
    settingsGyver.onUpdate(staticUpdate);
};

void WebInterface::tick()
{
    settingsGyver.tick();
};
void WebInterface::handleClient() {

};
void WebInterface::staticBuild(sets::Builder &b)
{
    if (instance)
        instance->build(b);
}

void WebInterface::staticUpdate(sets::Updater &u)
{
    if (instance)
        instance->update(u);
}

void WebInterface::build(sets::Builder &b)
{
    builder.build(b); // делегируем
}

void WebInterface::update(sets::Updater &u)
{
    action.update(u); // делегируем
    builder.update(u);
    if (g_wifiScanCompletedReloadPending) {
        g_wifiScanCompletedReloadPending = false;
        settingsGyver.reload();
    }
}
