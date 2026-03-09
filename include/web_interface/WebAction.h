#pragma once
#include <SettingsGyver.h>
#include <Arduino.h>

extern String g_wifiList;
extern bool g_wifiScanInProgress;
extern int g_wifiScanProgress;
extern bool g_wifiScanCompletedReloadPending;

void WebAction_startWifiScan();

class WebAction {
  public:
    WebAction();

    void update(sets::Updater& u);

  private:
    size_t   holdingsSize = 0;
};
