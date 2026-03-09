#pragma once

#define LIST_FILTR

#include "WebAction.h"
#include "WebBuilder.h"
#include <SPI.h>
#include <memory>
#include <Arduino.h>
#include <GyverDBFile.h>
#include <SettingsGyver.h>


constexpr const char* title = "Logic box";

#define WEB_PORT_WIFI \
    8080 // WiFi port, in future if you want you can make it changeable

class WebInterface {
  public:
    WebInterface(GyverDBFile& _db, String* ssid, String* pswd);
    void begin();
    void tick();
    void handleClient();


  private:
    void build(sets::Builder& b);
    void update(sets::Updater& u);

    GyverDBFile& db;
    static void  staticBuild(sets::Builder& b);
    static void  staticUpdate(sets::Updater& u);

    static WebInterface*              instance;
    WebBuilder                        builder;
    WebAction                         action;
    std::unique_ptr<WiFiServer>       wifiServer;
    SettingsGyver                     settingsGyver;
    String*                           _ssid;
    String*                           _pswd;

};
