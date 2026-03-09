#pragma once
#include <Stamp.h>
#include <Arduino.h>
#include <GyverDBFile.h>
#include <SettingsGyver.h>

namespace keys {
    // Temperature
    constexpr size_t hTempChannel = SH("temp_channel");
    constexpr size_t hTempPremises = SH("temp_premises");
    constexpr size_t hHeatExchange = SH("heat_exchange");
    constexpr size_t hTempWater = SH("temp_water");
    constexpr size_t hMinTempWater = SH("min_temp_water");
    constexpr size_t hWarm = SH("warm");

    // Timers
    constexpr size_t hTimeBlowing = SH("time_blowing");
    constexpr size_t hTimePumpMoving = SH("time_pump_moving");
    constexpr size_t hDurOfTimePumpMoving = SH("dur_time_pump_moving");
    constexpr size_t hTimeDumpOpen = SH("time_dump_open");
    constexpr size_t hTimeReadyDiffPres = SH("time_ready_diff_pres");
    constexpr size_t hTimeRecupInSec = SH("time_recup_in_sec");

    // PID
    constexpr size_t hP = SH("P");
    constexpr size_t hI = SH("I");
    constexpr size_t hD = SH("D");
    constexpr size_t hP2 = SH("P2");
    constexpr size_t hI2 = SH("I2");
    constexpr size_t hD2 = SH("D2");

    constexpr size_t hFanSpeed = SH("fan_speed");
    constexpr size_t hHumid = SH("humid");
    constexpr size_t hColdExchange = SH("cold_exchange");
    constexpr size_t hFanRotation = SH("fan_rotation");
    constexpr size_t hHysterSet = SH("hyster_set");

    // Network - WiFi AP
    constexpr size_t wifiApEn = SH("wifi_ap_en");
    constexpr size_t wifiApSsid = SH("wifi_ap_ssid");
    constexpr size_t wifiApPass = SH("wifi_ap_pass");
    constexpr size_t wifiApIp = SH("wifi_ap_ip");
    constexpr size_t wifiApDhcp = SH("wifi_ap_dhcp");
    constexpr size_t wifiApGw = SH("wifi_ap_gw");

    // Network - WiFi Station
    constexpr size_t wifiStEn = SH("wifi_st_en");
    constexpr size_t wifiStSsid = SH("wifi_st_ssid");
    constexpr size_t wifiStPass = SH("wifi_st_pass");
    constexpr size_t wifiStDhcp = SH("wifi_st_dhcp");
    constexpr size_t wifiStIp = SH("wifi_st_ip");
    constexpr size_t wifiStMask = SH("wifi_st_mask");
    constexpr size_t wifiStGw = SH("wifi_st_gw");

    // Network - Ethernet
    constexpr size_t ethDhcp = SH("eth_dhcp");
    constexpr size_t ethIp = SH("eth_ip");
    constexpr size_t ethMask = SH("eth_mask");
    constexpr size_t ethGw = SH("eth_gw");

    // Modbus RTU
    constexpr size_t rtuBaudIdx = SH("rtu_baud_idx");
    constexpr size_t rtuParityIdx = SH("rtu_parity_idx");
    constexpr size_t rtuIsMaster = SH("rtu_is_master");
    constexpr size_t rtuSlaveId = SH("rtu_slave_id");

    // Modbus TCP
    constexpr size_t mbTcpPort = SH("mb_tcp_port");
    constexpr size_t mbTcpSlaveId = SH("mb_tcp_slave_id");
    constexpr size_t mbTcpIsMaster = SH("mb_tcp_is_master");

    // UART
    constexpr size_t uartBaudIdx = SH("uart_baud_idx");
    constexpr size_t uartParityIdx = SH("uart_parity_idx");

    // Sensors
    constexpr size_t sens1Type = SH("sens1_type");
    constexpr size_t sens2Type = SH("sens2_type");
    constexpr size_t sens3Type = SH("sens3_type");
    constexpr size_t sens4Type = SH("sens4_type");

    // RTC
    constexpr size_t rtcUnixSec = SH("rtc_unix_sec");
    constexpr size_t rtcUnixMs = SH("rtc_unix_ms");

    // MQTT
    constexpr size_t mqttHost = SH("mqtt_host");
    constexpr size_t mqttPort = SH("mqtt_port");
    constexpr size_t mqttClientId = SH("mqtt_client_id");
    constexpr size_t mqttUser = SH("mqtt_user");
    constexpr size_t mqttPass = SH("mqtt_pass");
    constexpr size_t mqttTopic = SH("mqtt_topic");

    // RS485 People Counter
    constexpr size_t rs485Code1Person = SH("rs485_code_1_person");
    constexpr size_t rs485Code2People = SH("rs485_code_2_people");
}

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
