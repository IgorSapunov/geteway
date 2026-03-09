#include "web_interface/WebInterface.h"

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
