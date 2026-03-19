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
        b.Label("Выходы (DO)");
        b.Input(keys::doGreenLampEntryA, "Зел. лампа A");
        b.Input(keys::doRedLampEntryA, "Красн. лампа A");
        b.Input(keys::doGreenLampEntryB, "Зел. лампа B");
        b.Input(keys::doRedLampEntryB, "Красн. лампа B");
        b.Input(keys::doStatusEntryA, "Статус A");
        b.Input(keys::doStatusEntryB, "Статус B");
        b.Input(keys::doSoundEnter, "Звук вход");
        b.Input(keys::doSoundLeave, "Звук выход");
        b.Input(keys::doSignalPass, "Сигнал проход");
        b.endGroup();

        // DI Mapping
        b.beginGroup();
        b.Label("Входы (DI)");
        b.Input(keys::diOpenEntryDoorA, "Откр. дверь A (DI)");
        b.Input(keys::diOpenEntryDoorB, "Откр. дверь B (DI)");
        b.Input(keys::diFireSignal, "Пожар (DI)");
        b.Switch(keys::diFireInvert, "Инверсия пожара");
        b.Input(keys::diResetFactory, "Сброс настроек (DI)");
        b.Input(keys::diLowerSensor, "Нижний датчик (DI)");
        b.endGroup();

        // Modbus Drivers
        b.beginGroup();
        b.Label("Modbus ID");
        b.Input(keys::mbSlaveDriver1, "ID драйвера A");
        b.Input(keys::mbSlaveDriver2, "ID драйвера B");
        b.endGroup();

        // People Counter
        b.beginGroup();
        b.Label("RS-485 счётчик");
        b.Input(keys::byteRecognize0Person, "Код 0 чел");
        b.Input(keys::byteRecognize1Person, "Код 1 чел");
        b.Input(keys::byteRecognize2Person, "Код 2 чел");
        b.endGroup();
        
        // WiFi
        b.beginGroup();
        b.Label("WiFi AP");
        b.Input(keys::wifiApSsid, "SSID AP");
        b.Input(keys::wifiApPass, "Пароль AP");
        b.Input(keys::wifiApIp, "IP AP");
        b.endGroup();

        // Algorithm
        b.beginGroup();
        b.Label("Алгоритмы");
        b.Switch(keys::algoCompEn, "Алгоритм компенсации");
        b.Switch(keys::algoSkipOpenConfirm, "Без подтвержд. откр");
        b.Switch(keys::algoWaitSecondDoorEmpty, "Ждать пусто перед закр д2");
        b.Switch(keys::algoSecondDoorMaxHoldEn, "Закр д2 по макс времени");
        b.Input(keys::algoSecondDoorMaxHoldMs, "Макс держать д2, мс");
        b.Input(keys::algoOpenSensorToutMs, "Таймаут открытия, мс");
        b.Input(keys::algoPersonWaitToutMs, "Таймаут ожидания, мс");
        b.Input(keys::algoSwitchDelayMs, "Задержка перед откр д2,мс");
        b.Input(keys::algoSecondDoorHoldMs, "Задержка перед закр д2,мс");
        b.endGroup();

    }
}

void WebBuilder::update(sets::Updater &u)
{
    (void)u;
}
