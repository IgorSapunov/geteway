#include "web_interface\WebAction.h"
#include <Arduino.h>
#include <WiFi.h>

String g_wifiList = "";
bool g_wifiScanInProgress = false;
int g_wifiScanProgress = 0;
bool g_wifiScanCompletedReloadPending = false;

void WebAction_startWifiScan() {
    if (g_wifiScanInProgress) return;
    g_wifiScanInProgress = true;
    g_wifiScanProgress = 0;
    g_wifiList = "";
    WiFi.scanNetworks(true); // async scan
}

WebAction::WebAction() {};

void WebAction::update(sets::Updater& u) {};
