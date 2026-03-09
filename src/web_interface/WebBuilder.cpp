#include "web_interface\WebBuilder.h"
#include "web_interface\WebAction.h"
#include <GyverDBFile.h>
#include <WiFi.h>

WebBuilder::WebBuilder() {};

void WebBuilder::setDbPointer(GyverDBFile* db) {
    _db = db;
}

void WebBuilder::setSsidPswd(String* ssid, String* pswd) {
    _ssid = ssid;
    _pswd = pswd;
}

void WebBuilder::build(sets::Builder &b)
{
    enum Tabs : uint8_t 
    { 
        Control, 
        InputA, 
        InputB, 
        Settings, 
    };
    static Tabs tab = Tabs::Control;

    if (b.Tabs("Управление;Вход А;Вход В;Настройки", (uint8_t *)&tab)) 
    { 
        b.reload(); 
        return; 
    } 

    switch (tab) 
    { 
    case Tabs::Control: 
        b.Label("Управление");
        break;

    case Tabs::InputA:
        b.Label("Вход А");
        break;

    case Tabs::InputB:
        b.Label("Вход В");
        break;

    case Tabs::Settings:
        // RS485 People Counter
        b.beginGroup();
        b.Label("RS485 People Counter");
        b.Input(keys::rs485Code1Person, "Code for 1 Person (0-255)");
        b.Input(keys::rs485Code2People, "Code for 2 People (0-255)");
        b.endGroup();

        // group of temperatures 
        b.beginGroup(); 
    
        b.endGroup();

        // group of timers
        b.beginGroup();
   
        b.endGroup();

        // group of pids and hyster
        b.beginGroup();

        b.endGroup();
        break;
    }
}

void WebBuilder::update(sets::Updater &u)
{
}
