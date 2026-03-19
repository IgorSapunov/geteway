#include <Arduino.h>
#include <LittleFS.h>
#include <GyverDBFile.h>
#include <web_interface/WebInterface.h>
#include <ModbusRS485.h>
#include <RawRS485.h>
#include <IOControl.h>
#include "Settings.h" // For WIFI_SSID, WIFI_PASSWORD

// Global objects
GyverDBFile db(&LittleFS, "/settings.db");
String ssid = WIFI_SSID;
String pswd = WIFI_PASSWORD;

// Instantiate modules
WebInterface webInterface(db, &ssid, &pswd);
ModbusRS485 modbusRS485(&db);
RawRS485 rawRS485(&db);
IOControl ioControl;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Gateway Controller...");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Initialize DB
    db.begin();

    // Initialize modules
    ioControl.begin();
    modbusRS485.begin();
    rawRS485.begin();
    webInterface.begin();

    Serial.println("Gateway Controller Started");
}

void loop() {
    // Main loop
    db.tick(); // Handle DB autosave
    webInterface.handleClient(); // Should handle web server client
    webInterface.tick();   // Handle settings tick
    modbusRS485.loop();
    rawRS485.loop();
    ioControl.loop();
}
