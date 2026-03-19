#include "web_interface\WebBuilder.h"
#include "web_interface\WebAction.h"
#include "ModbusRS485.h"
#include "RawRS485.h"
#include "IOControl.h"
#include <GyverDBFile.h>
#include <WiFi.h>
#include <stdio.h>

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
        DoorA, 
        DoorB, 
        Settings, 
    };
    static Tabs tab = Tabs::Control;

    if (b.Tabs("Управление;Дверь A;Дверь B;Настройки", (uint8_t *)&tab)) 
    { 
        b.reload(); 
        return; 
    } 

    switch (tab) 
    { 
    case Tabs::Control: {
        b.beginGroup();
        b.Label("Режим");
        b.Switch(keys::modeAutoEn, "Авто режим");
        b.Switch(keys::modeTestEn, "Тест режим");
        b.endGroup();

        b.beginGroup();
        b.Label("Тест команды");
        const bool testNow = _db ? _db->get(keys::modeTestEn).toBool() : false;
        const bool autoNow = _db ? _db->get(keys::modeAutoEn).toBool() : false;

        if (_db) {
            if (autoNow && testNow) _db->set(keys::modeAutoEn, false);
            if (!autoNow && !testNow) _db->set(keys::modeAutoEn, true);
        }

        const auto setOut = [&](keys key, bool st) {
            if (!_db) return;
            if (auto io = IOControl::instance()) {
                const int ch = _db->get(key).toInt();
                if (ch >= 0 && ch <= 15) io->setOutput((uint8_t)ch, st);
            }
        };

        const auto setCabinClosed = [&]() {
            setOut(keys::doSoundEnter, false);
            setOut(keys::doSignalPass, false);
            setOut(keys::doSoundLeave, false);

            setOut(keys::doRedLampEntryA, true);
            setOut(keys::doGreenLampEntryA, false);
            setOut(keys::doStatusEntryA, false);

            setOut(keys::doRedLampEntryB, true);
            setOut(keys::doGreenLampEntryB, false);
            setOut(keys::doStatusEntryB, false);
        };

        if (b.Button(keys::uiTestOpenAEntryBtn, "Открыть A вход") && testNow) {
            if (auto m = ModbusRS485::instance()) m->writeRegister(MODBUS_SLAVE_ID_ENTRY_A, 326, 1);
            setOut(keys::doSoundLeave, false);
            setOut(keys::doSignalPass, false);
            setOut(keys::doSoundEnter, true);
            setOut(keys::doRedLampEntryA, false);
            setOut(keys::doGreenLampEntryA, true);
            setOut(keys::doStatusEntryA, true);
        }

        if (b.Button(keys::uiTestOpenBEntryBtn, "Открыть B вход") && testNow) {
            if (auto m = ModbusRS485::instance()) m->writeRegister(MODBUS_SLAVE_ID_ENTRY_B, 326, 1);
            setOut(keys::doSoundLeave, false);
            setOut(keys::doSignalPass, false);
            setOut(keys::doSoundEnter, true);
            setOut(keys::doRedLampEntryB, false);
            setOut(keys::doGreenLampEntryB, true);
            setOut(keys::doStatusEntryB, true);
        }

        if (b.Button(keys::uiTestFireBtn, "Пожар") && testNow) {
            if (auto m = ModbusRS485::instance()) {
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_A, 326, 1);
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_A, 325, 1);
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_B, 326, 1);
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_B, 325, 1);
            }
            setOut(keys::doSoundEnter, false);
            setOut(keys::doSignalPass, false);
            setOut(keys::doSoundLeave, false);
            setOut(keys::doRedLampEntryA, false);
            setOut(keys::doGreenLampEntryA, true);
            setOut(keys::doStatusEntryA, true);
            setOut(keys::doRedLampEntryB, false);
            setOut(keys::doGreenLampEntryB, true);
            setOut(keys::doStatusEntryB, true);
        }

        if (b.Button(keys::uiTestNormalBtn, "Нормальный режим") && testNow) {
            if (auto m = ModbusRS485::instance()) {
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_A, 327, 1);
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_B, 327, 1);
            }
            setCabinClosed();
        }

        if (b.Button(keys::uiTestCloseBtn, "Закрыть") && testNow) {
            if (auto m = ModbusRS485::instance()) {
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_A, 327, 1);
                m->writeRegister(MODBUS_SLAVE_ID_ENTRY_B, 327, 1);
            }
            setCabinClosed();
        }
        b.endGroup();

        b.beginGroup();
        b.Label("Тест RS-485 RAW");
        if (auto rs = RawRS485::instance()) {
            uint32_t totalBytes = 0;
            uint8_t lastByte = 0;
            uint32_t sinceMs = 0;
            if (rs->getRxStats(totalBytes, lastByte, sinceMs)) {
                char l1[40];
                snprintf(l1, sizeof(l1), "Всего байт: %lu", (unsigned long)totalBytes);
                b.Label(l1);
                char l2[40];
                snprintf(l2, sizeof(l2), "Последний байт: 0x%02X", (unsigned)lastByte);
                b.Label(l2);
                char l3[40];
                snprintf(l3, sizeof(l3), "Мс с последнего: %lu", (unsigned long)sinceMs);
                b.Label(l3);
            } else {
                b.Label("Всего байт: 0");
                b.Label("Последний байт: нет данных");
                b.Label("Мс с последнего: нет данных");
            }
            uint8_t sig = 0;
            int8_t people = -1;
            if (rs->getLastSignificantByte(sig, people)) {
                char line[40];
                snprintf(line, sizeof(line), "Byte after 0xFF: 0x%02X", (unsigned)sig);
                b.Label(line);
                if (people == 0) b.Label("Люди: 0");
                else if (people == 1) b.Label("Люди: 1");
                else if (people == 2) b.Label("Люди: 2");
                else b.Label("Люди: неизвестно");
            } else {
                b.Label("Byte after 0xFF: нет данных");
                b.Label("Люди: нет данных");
            }
        } else {
            b.Label("RawRS485: not ready");
        }
        b.endGroup();
        break;
    }

    case Tabs::DoorA: {
        if (auto m = ModbusRS485::instance()) {
            static uint16_t edit[MODBUS_REG_COUNT];
            static uint8_t dirty[(MODBUS_REG_COUNT + 7) / 8];

            const auto isDirty = [&](uint16_t addr) -> bool {
                return (dirty[addr >> 3] & (uint8_t)(1u << (addr & 7))) != 0;
            };
            const auto setDirty = [&](uint16_t addr, bool v) {
                const uint8_t mask = (uint8_t)(1u << (addr & 7));
                if (v) dirty[addr >> 3] |= mask;
                else dirty[addr >> 3] &= (uint8_t)~mask;
            };

            for (uint16_t base = 0; base < MODBUS_REG_COUNT; base += 25) {
                b.beginGroup();
                char hdr[32];
                uint16_t end = base + 24;
                if (end >= MODBUS_REG_COUNT) end = MODBUS_REG_COUNT - 1;
                snprintf(hdr, sizeof(hdr), "Регистры %u..%u", (unsigned)base, (unsigned)end);
                b.Label(hdr);

                for (uint16_t addr = base; addr <= end; addr++) {
                    const uint16_t v = m->readCache(MODBUS_SLAVE_ID_ENTRY_A, addr);
                    if (!isDirty(addr)) edit[addr] = v;
                    char lbl[24];
                    snprintf(lbl, sizeof(lbl), "%03u (0x%04X)", (unsigned)addr, (unsigned)v);
                    const size_t wid = (size_t)0xA0000u + (size_t)addr;
                    if (b.Number(wid, lbl, &edit[addr], 0, 65535)) setDirty(addr, true);
                }

                const size_t btnId = (size_t)0xA8000u + (size_t)base;
                if (b.Button(btnId, "Отправить изменения блока")) {
                    for (uint16_t addr = base; addr <= end; addr++) {
                        if (!isDirty(addr)) continue;
                        if (m->queueWriteRegister(MODBUS_SLAVE_ID_ENTRY_A, addr, edit[addr])) {
                            setDirty(addr, false);
                        }
                    }
                    b.reload();
                    b.endGroup();
                    break;
                }
                b.endGroup();
            }
        }
        break;
    }

    case Tabs::DoorB: {
        if (auto m = ModbusRS485::instance()) {
            static uint16_t edit[MODBUS_REG_COUNT];
            static uint8_t dirty[(MODBUS_REG_COUNT + 7) / 8];

            const auto isDirty = [&](uint16_t addr) -> bool {
                return (dirty[addr >> 3] & (uint8_t)(1u << (addr & 7))) != 0;
            };
            const auto setDirty = [&](uint16_t addr, bool v) {
                const uint8_t mask = (uint8_t)(1u << (addr & 7));
                if (v) dirty[addr >> 3] |= mask;
                else dirty[addr >> 3] &= (uint8_t)~mask;
            };

            for (uint16_t base = 0; base < MODBUS_REG_COUNT; base += 25) {
                b.beginGroup();
                char hdr[32];
                uint16_t end = base + 24;
                if (end >= MODBUS_REG_COUNT) end = MODBUS_REG_COUNT - 1;
                snprintf(hdr, sizeof(hdr), "Регистры %u..%u", (unsigned)base, (unsigned)end);
                b.Label(hdr);

                for (uint16_t addr = base; addr <= end; addr++) {
                    const uint16_t v = m->readCache(MODBUS_SLAVE_ID_ENTRY_B, addr);
                    if (!isDirty(addr)) edit[addr] = v;
                    char lbl[24];
                    snprintf(lbl, sizeof(lbl), "%03u (0x%04X)", (unsigned)addr, (unsigned)v);
                    const size_t wid = (size_t)0xB0000u + (size_t)addr;
                    if (b.Number(wid, lbl, &edit[addr], 0, 65535)) setDirty(addr, true);
                }

                const size_t btnId = (size_t)0xB8000u + (size_t)base;
                if (b.Button(btnId, "Отправить изменения блока")) {
                    for (uint16_t addr = base; addr <= end; addr++) {
                        if (!isDirty(addr)) continue;
                        if (m->queueWriteRegister(MODBUS_SLAVE_ID_ENTRY_B, addr, edit[addr])) {
                            setDirty(addr, false);
                        }
                    }
                    b.reload();
                    b.endGroup();
                    break;
                }
                b.endGroup();
            }
        }
        break;
    }

    case Tabs::Settings:
        // DO Mapping
        b.beginGroup();
        b.Label("Digital Outputs Mapping");
        b.Input(keys::doGreenLampEntryA, "Green Lamp Entry A");
        b.Input(keys::doRedLampEntryA, "Red Lamp Entry A");
        b.Input(keys::doGreenLampEntryB, "Green Lamp Entry B");
        b.Input(keys::doRedLampEntryB, "Red Lamp Entry B");
        b.Input(keys::doStatusEntryA, "Status Entry A");
        b.Input(keys::doStatusEntryB, "Status Entry B");
        b.Input(keys::doSoundEnter, "Sound Enter");
        b.Input(keys::doSoundLeave, "Sound Leave");
        b.Input(keys::doSignalPass, "Signal Pass");
        b.endGroup();

        // DI Mapping
        b.beginGroup();
        b.Label("Digital Inputs Mapping");
        b.Input(keys::diOpenEntryDoorA, "Open Entry Door A (DI 0..15)");
        b.Input(keys::diOpenEntryDoorB, "Open Entry Door B (DI 0..15)");
        b.Input(keys::diFireSignal, "Fire Signal (DI 0..15)");
        b.Switch(keys::diFireInvert, "Инверсия пожарного сигнала");
        b.Input(keys::diResetFactory, "Factory Reset (DI 0..15)");
        b.Input(keys::diLowerSensor, "Lower Sensor (DI 0..15)");
        b.endGroup();

        // Modbus Drivers
        b.beginGroup();
        b.Label("Modbus Slaves");
        b.Input(keys::mbSlaveDriver1, "Driver 1 ID");
        b.Input(keys::mbSlaveDriver2, "Driver 2 ID");
        b.endGroup();

        // People Counter
        b.beginGroup();
        b.Label("RS485 People Counter");
        b.Input(keys::byteRecognize0Person, "Code for 0 Person");
        b.Input(keys::byteRecognize1Person, "Code for 1 Person");
        b.Input(keys::byteRecognize2Person, "Code for 2 People");
        b.endGroup();
        
        // WiFi
        b.beginGroup();
        b.Label("WiFi Settings");
        b.Input(keys::wifiApSsid, "AP SSID");
        b.Input(keys::wifiApPass, "AP Password");
        b.Input(keys::wifiApIp, "AP IP");
        b.endGroup();

        // Algorithm
        b.beginGroup();
        b.Label("Algorithm");
        b.Switch(keys::algoCompEn, "Compensation Enabled");
        b.Switch(keys::algoSkipOpenConfirm, "Skip open confirm");
        b.Switch(keys::algoWaitSecondDoorEmpty, "Wait empty before close second");
        b.Switch(keys::algoSecondDoorMaxHoldEn, "Close second by max time");
        b.Input(keys::algoSecondDoorMaxHoldMs, "Second Door Max Hold, ms");
        b.Input(keys::algoOpenSensorToutMs, "Open Sensor Timeout, ms");
        b.Input(keys::algoPersonWaitToutMs, "Wait Person Timeout, ms");
        b.Input(keys::algoSwitchDelayMs, "Delay before open second, ms");
        b.Input(keys::algoSecondDoorHoldMs, "Second Door Close Delay, ms");
        b.endGroup();

    }
}

void WebBuilder::update(sets::Updater &u)
{
    (void)u;
}
