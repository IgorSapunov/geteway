#include "ModbusRS485.h"
#include "web_interface/WebBuilder.h" // For keys
#include "IOControl.h"
#include "RawRS485.h"

static ModbusRS485* s_modbusRS485 = nullptr;

namespace {
static const char* rcName(Modbus::ResultCode rc) {
    switch (rc) {
        case Modbus::EX_SUCCESS: return "OK";
        case Modbus::EX_ILLEGAL_FUNCTION: return "ILLEGAL_FUNCTION";
        case Modbus::EX_ILLEGAL_ADDRESS: return "ILLEGAL_ADDRESS";
        case Modbus::EX_ILLEGAL_VALUE: return "ILLEGAL_VALUE";
        case Modbus::EX_SLAVE_FAILURE: return "SLAVE_FAILURE";
        case Modbus::EX_ACKNOWLEDGE: return "ACK";
        case Modbus::EX_SLAVE_DEVICE_BUSY: return "SLAVE_BUSY";
        case Modbus::EX_MEMORY_PARITY_ERROR: return "MEM_PARITY";
        case Modbus::EX_PATH_UNAVAILABLE: return "PATH_UNAVAIL";
        case Modbus::EX_DEVICE_FAILED_TO_RESPOND: return "DEVICE_NO_RESP";
        case Modbus::EX_TIMEOUT: return "TIMEOUT";
        case Modbus::EX_GENERAL_FAILURE: return "GENERAL_FAILURE";
        case Modbus::EX_DATA_MISMACH: return "DATA_MISMACH";
        case Modbus::EX_UNEXPECTED_RESPONSE: return "UNEXPECTED_RESPONSE";
        case Modbus::EX_CONNECTION_LOST: return "CONNECTION_LOST";
        case Modbus::EX_CANCEL: return "CANCEL";
        case Modbus::EX_PASSTHROUGH: return "PASSTHROUGH";
        case Modbus::EX_FORCE_PROCESS: return "FORCE_PROCESS";
        default: return "ERR";
    }
}

struct PendingWrite {
    uint8_t slave;
    uint16_t addr;
    uint16_t value;
};

static constexpr uint8_t kWriteQSize = 64;
static PendingWrite s_writeQ[kWriteQSize];
static uint8_t s_writeQHead = 0;
static uint8_t s_writeQTail = 0;
static bool s_writeQBusy = false;

static inline bool qEmpty() {
    return s_writeQHead == s_writeQTail;
}
static inline bool qFull() {
    return (uint8_t)(s_writeQTail + 1) == s_writeQHead;
}
static inline void qPush(const PendingWrite& w) {
    s_writeQ[s_writeQTail] = w;
    s_writeQTail = (uint8_t)(s_writeQTail + 1);
}
static inline PendingWrite& qFront() {
    return s_writeQ[s_writeQHead];
}
static inline void qPop() {
    s_writeQHead = (uint8_t)(s_writeQHead + 1);
}
}  // namespace

ModbusRS485::ModbusRS485(GyverDBFile* db) : _db(db) {
    s_modbusRS485 = this;
    memset(_regsEntryA, 0, sizeof(_regsEntryA));
    memset(_regsEntryB, 0, sizeof(_regsEntryB));
}

ModbusRS485* ModbusRS485::instance() {
    return s_modbusRS485;
}

void ModbusRS485::begin() {
    // Setup Serial
    // Note: Verify if Serial1 is used correctly on your board. 
    // ESP32 allows remapping.
    Serial1.begin(RS485_BAUD_RATE, RS485_CONFIG, RXD_485_PIN, TXD_485_PIN);
    
    // Setup Modbus
    mb.begin(&Serial1);
    mb.master(); // Set as Master
}

bool ModbusRS485::_onTransaction(Modbus::ResultCode event, uint16_t transactionId, void* data) {
    (void)transactionId;
    (void)data;
#if MODBUS_RS485_LOG
    Serial.print("ModbusRTU ");
    Serial.print(rcName(event));
    Serial.print(" rc=0x");
    Serial.print((uint8_t)event, HEX);
    Serial.print(" tr=");
    Serial.print(transactionId);
    Serial.print(" src=");
    if (s_modbusRS485) {
        Serial.println((uint32_t)s_modbusRS485->mb.eventSource());
    } else {
        Serial.println("?");
    }
#endif
    if (s_modbusRS485) s_modbusRS485->_busy = false;
    s_writeQBusy = false;
    return true;
}

bool ModbusRS485::_scheduleRead(uint8_t slaveId, uint16_t address, uint16_t count, uint16_t* dst) {
    if (_busy || slaveId == 0 || dst == nullptr || count == 0) return false;
    uint16_t tr = mb.readHreg(slaveId, address, dst, count, _onTransaction);
    if (!tr) return false;
    _busy = true;
    _lastPollStartMs = millis();
    return true;
}

bool ModbusRS485::_scheduleWrite(uint8_t slaveId, uint16_t address, uint16_t value) {
    if (_busy || slaveId == 0) return false;
    uint16_t tr = mb.writeHreg(slaveId, address, value, _onTransaction);
    if (!tr) return false;
    _busy = true;
    _lastPollStartMs = millis();
    return true;
}

bool ModbusRS485::_scheduleWriteMulti(uint8_t slaveId, uint16_t address, const uint16_t* values, uint16_t count) {
    if (_busy || slaveId == 0 || values == nullptr || count == 0) return false;
    uint16_t tr = mb.writeHreg(slaveId, address, (uint16_t*)values, count, _onTransaction);
    if (!tr) return false;
    _busy = true;
    _lastPollStartMs = millis();
    return true;
}

void ModbusRS485::loop() {
    mb.task();

    if (!_busy && !s_writeQBusy && !qEmpty()) {
        PendingWrite& w = qFront();
        const uint8_t phy = (w.slave == MODBUS_SLAVE_ID_ENTRY_A) ? _slaveIdA : _slaveIdB;
        if (_scheduleWrite(phy, w.addr, w.value)) {
            s_writeQBusy = true;
            qPop();
        }
    }

    enum AlgoPhase : uint8_t {
        AlgoIdle = 0,
        AlgoFireSend,
        AlgoFireHold,
        AlgoFireExitSend,
        AlgoOpenStartSend,
        AlgoOpenStartWait,
        AlgoWaitPerson,
        AlgoCloseStartSend,
        AlgoWaitOpenSecondDelay,
        AlgoOpenSecondSend,
        AlgoOpenSecondWait,
        AlgoHoldSecond,
        AlgoCloseSecondSend,
    };

    static AlgoPhase phase = AlgoIdle;
    static uint32_t tPhase = 0;
    static uint8_t startDoor = 0;
    static uint8_t secondDoor = 1;
    static uint16_t startOpenMask = 0x0002;
    static uint16_t secondOpenMask = 0x0001;
    static bool openSecondAfterClose = false;
    static uint8_t fireStep = 0;

    static int chOpenA = DI_OPEN_ENTRY_DOOR_A;
    static int chOpenB = DI_OPEN_ENTRY_DOOR_B;
    static int chFire = DI_FIRE_SIGNAL;
    static bool fireInvert = false;
    static uint32_t toutOpenSensor = 5000;
    static uint32_t toutPersonWait = 15000;
    static uint32_t toutSwitchDelay = 1500;
    static uint32_t toutSecondCloseDelay = 100;
    static uint32_t toutSecondMaxHold = 5000;
    static bool compEn = false;
    static bool skipOpenConfirm = false;
    static bool waitSecondEmpty = false;
    static bool secondMaxHoldEn = false;
    static bool modeAuto = true;
    static bool modeTest = false;
    static bool secondHadPeople = false;
    static uint32_t secondOpenMs = 0;
    static uint32_t secondEmptySinceMs = 0;
    static uint32_t openSecondAtMs = 0;
    static uint32_t enterPulseOffMs = 0;
    static uint32_t passPulseOffMs = 0;

    static bool prevReqA = false;
    static bool prevReqB = false;
    static bool leaveAlert = false;

    static int chGreenA = DO_GREEN_LAMP_ENTRY_A;
    static int chRedA = DO_RED_LAMP_ENTRY_A;
    static int chGreenB = DO_GREEN_LAMP_ENTRY_B;
    static int chRedB = DO_RED_LAMP_ENTRY_B;
    static int chStatusA = DO_STATUS_ENTRY_A;
    static int chStatusB = DO_STATUS_ENTRY_B;
    static int chSoundEnter = DO_SOUND_ENTER;
    static int chSoundLeave = DO_SOUND_LEAVE;
    static int chSignalPass = DO_SIGNAL_PASS;

    // Get settings from DB
    if (_db) {
        if (millis() - _lastCfgMs >= 1000) {
            _lastCfgMs = millis();
            if (_db->has(keys::mbSlaveDriver1)) _slaveIdA = (uint8_t)_db->get(keys::mbSlaveDriver1).toInt();
            if (_db->has(keys::mbSlaveDriver2)) _slaveIdB = (uint8_t)_db->get(keys::mbSlaveDriver2).toInt();
            if (_db->has(keys::diOpenEntryDoorA)) chOpenA = _db->get(keys::diOpenEntryDoorA).toInt();
            if (_db->has(keys::diOpenEntryDoorB)) chOpenB = _db->get(keys::diOpenEntryDoorB).toInt();
            if (_db->has(keys::diFireSignal)) chFire = _db->get(keys::diFireSignal).toInt();
            if (_db->has(keys::diFireInvert)) fireInvert = _db->get(keys::diFireInvert).toBool();
            if (_db->has(keys::algoOpenSensorToutMs)) toutOpenSensor = (uint32_t)_db->get(keys::algoOpenSensorToutMs).toInt();
            if (_db->has(keys::algoPersonWaitToutMs)) toutPersonWait = (uint32_t)_db->get(keys::algoPersonWaitToutMs).toInt();
            if (_db->has(keys::algoSwitchDelayMs)) toutSwitchDelay = (uint32_t)_db->get(keys::algoSwitchDelayMs).toInt();
            if (_db->has(keys::algoSecondDoorHoldMs)) toutSecondCloseDelay = (uint32_t)_db->get(keys::algoSecondDoorHoldMs).toInt();
            if (_db->has(keys::algoCompEn)) compEn = _db->get(keys::algoCompEn).toBool();
            if (_db->has(keys::algoSkipOpenConfirm)) skipOpenConfirm = _db->get(keys::algoSkipOpenConfirm).toBool();
            if (_db->has(keys::algoWaitSecondDoorEmpty)) waitSecondEmpty = _db->get(keys::algoWaitSecondDoorEmpty).toBool();
            if (_db->has(keys::algoSecondDoorMaxHoldEn)) secondMaxHoldEn = _db->get(keys::algoSecondDoorMaxHoldEn).toBool();
            if (_db->has(keys::algoSecondDoorMaxHoldMs)) toutSecondMaxHold = (uint32_t)_db->get(keys::algoSecondDoorMaxHoldMs).toInt();
            if (_db->has(keys::modeAutoEn)) modeAuto = _db->get(keys::modeAutoEn).toBool();
            if (_db->has(keys::modeTestEn)) modeTest = _db->get(keys::modeTestEn).toBool();

            if (_db->has(keys::doGreenLampEntryA)) chGreenA = _db->get(keys::doGreenLampEntryA).toInt();
            if (_db->has(keys::doRedLampEntryA)) chRedA = _db->get(keys::doRedLampEntryA).toInt();
            if (_db->has(keys::doGreenLampEntryB)) chGreenB = _db->get(keys::doGreenLampEntryB).toInt();
            if (_db->has(keys::doRedLampEntryB)) chRedB = _db->get(keys::doRedLampEntryB).toInt();
            if (_db->has(keys::doStatusEntryA)) chStatusA = _db->get(keys::doStatusEntryA).toInt();
            if (_db->has(keys::doStatusEntryB)) chStatusB = _db->get(keys::doStatusEntryB).toInt();
            if (_db->has(keys::doSoundEnter)) chSoundEnter = _db->get(keys::doSoundEnter).toInt();
            if (_db->has(keys::doSoundLeave)) chSoundLeave = _db->get(keys::doSoundLeave).toInt();
            if (_db->has(keys::doSignalPass)) chSignalPass = _db->get(keys::doSignalPass).toInt();
        }
    }

    if (_busy && millis() - _lastPollStartMs > 250) _busy = false;

    IOControl* io = IOControl::instance();
    RawRS485* rs = RawRS485::instance();

    const auto setOut = [&](int ch, bool st) {
        if (!io) return;
        if (ch < 0 || ch > 15) return;
        io->setOutput((uint8_t)ch, st);
    };

    bool reqA = false;
    bool reqB = false;
    bool fire = false;

    if (io) {
        if (chOpenA >= 0 && chOpenA <= 15) reqA = !io->readInput((uint8_t)chOpenA);
        if (chOpenB >= 0 && chOpenB <= 15) reqB = !io->readInput((uint8_t)chOpenB);
        if (chFire >= 0 && chFire <= 15) {
            fire = !io->readInput((uint8_t)chFire);
            if (fireInvert) fire = !fire;
        }
    }

    const bool autoActive = modeAuto && !modeTest;
    if (!autoActive) {
        reqA = false;
        reqB = false;
    }

    const bool riseA = reqA && !prevReqA;
    const bool riseB = reqB && !prevReqB;
    prevReqA = reqA;
    prevReqB = reqB;

    const auto doorSlave = [](uint8_t door) -> uint8_t {
        return (door == 0) ? MODBUS_SLAVE_ID_ENTRY_A : MODBUS_SLAVE_ID_ENTRY_B;
    };

    const auto statusReg = [&](uint8_t door) -> uint16_t {
        return readCache(doorSlave(door), 324);
    };

#if DOOR_STATE_LOG
    const auto doorName = [](uint8_t door) -> const char* { return (door == 0) ? "A" : "B"; };
    const auto logDoorCmd = [&](const char* action, uint8_t door, uint8_t slaveId, uint16_t reg, uint16_t value) {
        Serial.print("CMD: ");
        Serial.print(action);
        Serial.print(" door=");
        Serial.print(doorName(door));
        Serial.print(" slave=");
        Serial.print((unsigned)slaveId);
        Serial.print(" reg=");
        Serial.print((unsigned)reg);
        Serial.print(" val=");
        Serial.println((unsigned)value);
    };
    {
        static bool prevAOpen = false;
        static bool prevBOpen = false;
        const uint16_t stA = statusReg(0);
        const uint16_t stB = statusReg(1);
        const bool aOpen = (stA & 0x0003u) != 0;
        const bool bOpen = (stB & 0x0003u) != 0;
        if (aOpen != prevAOpen) {
            Serial.println(aOpen ? "Door A OPEN" : "Door A CLOSED");
            prevAOpen = aOpen;
        }
        if (bOpen != prevBOpen) {
            Serial.println(bOpen ? "Door B OPEN" : "Door B CLOSED");
            prevBOpen = bOpen;
        }
    }
#endif

    const uint32_t now = millis();

    if (enterPulseOffMs && (int32_t)(now - enterPulseOffMs) >= 0) {
        setOut(chSoundEnter, false);
        enterPulseOffMs = 0;
    }
    if (passPulseOffMs && (int32_t)(now - passPulseOffMs) >= 0) {
        setOut(chSignalPass, false);
        passPulseOffMs = 0;
    }

    if (!autoActive && !fire) {
        phase = AlgoIdle;
        openSecondAfterClose = false;
        fireStep = 0;
        leaveAlert = false;
        openSecondAtMs = 0;
        secondOpenMs = 0;
        secondEmptySinceMs = 0;
        secondHadPeople = false;
    }

    if (fire) {
        if (phase != AlgoFireSend && phase != AlgoFireHold) {
            phase = AlgoFireSend;
            fireStep = 0;
            tPhase = now;
            leaveAlert = false;
        }
    } else {
        if (phase == AlgoFireHold) {
            phase = AlgoFireExitSend;
            fireStep = 0;
            tPhase = now;
            leaveAlert = false;
        }
    }

    if (phase == AlgoIdle) {
        if (autoActive) {
            setOut(chRedA, true);
            setOut(chGreenA, false);
            setOut(chStatusA, false);
            setOut(chRedB, true);
            setOut(chGreenB, false);
            setOut(chStatusB, false);
            setOut(chSoundEnter, false);
            setOut(chSignalPass, false);
            setOut(chSoundLeave, false);
            leaveAlert = false;
        }

        if (autoActive && !fire && !(reqA && reqB) && !(riseA && riseB)) {
            if (riseA && !reqB) {
                startDoor = 0;
                secondDoor = 1;
                startOpenMask = 0x0002;
                secondOpenMask = 0x0001;
                phase = AlgoOpenStartSend;
                tPhase = now;
            } else if (riseB && !reqA) {
                startDoor = 1;
                secondDoor = 0;
                startOpenMask = 0x0002;
                secondOpenMask = 0x0001;
                phase = AlgoOpenStartSend;
                tPhase = now;
            }
        }
    } else if (phase == AlgoOpenStartWait) {
        if (skipOpenConfirm) {
            phase = AlgoWaitPerson;
            tPhase = now;
        } else
        if (statusReg(startDoor) & startOpenMask) {
            phase = AlgoWaitPerson;
            tPhase = now;
        } else if (now - tPhase >= toutOpenSensor) {
            openSecondAfterClose = false;
            phase = AlgoCloseStartSend;
        }
    } else if (phase == AlgoWaitPerson) {
        uint8_t sig = 0;
        int8_t people = 0;
        const bool hasPeople = rs ? rs->getLastSignificantByte(sig, people) : false;
        if (hasPeople) {
            if (compEn && people > 0) people = 1;
            if (people > 0) {
                secondDoor = (startDoor == 0) ? 1 : 0;
                setOut(chSignalPass, true);
                passPulseOffMs = now + 10;
                openSecondAfterClose = true;
                openSecondAtMs = now + toutSwitchDelay;
                phase = AlgoCloseStartSend;
                tPhase = now;
            }
        }
        if (phase == AlgoWaitPerson && now - tPhase >= toutPersonWait) {
            openSecondAfterClose = false;
            phase = AlgoCloseStartSend;
        }
    } else if (phase == AlgoOpenSecondWait) {
        if (skipOpenConfirm || (statusReg(secondDoor) & secondOpenMask) || (now - tPhase >= toutOpenSensor)) {
            phase = AlgoHoldSecond;
            tPhase = now;
            secondOpenMs = now;
            secondEmptySinceMs = 0;
            secondHadPeople = false;
        }
    } else if (phase == AlgoHoldSecond) {
        uint8_t sig = 0;
        int8_t people = 0;
        const bool hasPeople = rs ? rs->getLastSignificantByte(sig, people) : false;
        if (hasPeople) {
            if (compEn && people > 0) people = 1;
            if (people > 0) {
                secondHadPeople = true;
                secondEmptySinceMs = 0;
            } else if (people == 0) {
                if (!waitSecondEmpty) {
                    if (secondOpenMs && now - secondOpenMs >= toutSecondCloseDelay) {
                        phase = AlgoCloseSecondSend;
                    }
                } else if (secondHadPeople) {
                    if (!secondEmptySinceMs) secondEmptySinceMs = now;
                    if (now - secondEmptySinceMs >= toutSecondCloseDelay) {
                        phase = AlgoCloseSecondSend;
                    }
                }
            }
        }
        if (phase == AlgoHoldSecond && secondMaxHoldEn && secondOpenMs && now - secondOpenMs >= toutSecondMaxHold) {
            phase = AlgoCloseSecondSend;
        }
    }

    if (!_busy) {
        if (phase == AlgoWaitOpenSecondDelay) {
            if (openSecondAtMs && (int32_t)(now - openSecondAtMs) >= 0) {
                openSecondAtMs = 0;
                phase = AlgoOpenSecondSend;
            }
        }

        if (phase == AlgoFireSend) {
            bool sent = false;
            if (fireStep == 0) sent = _scheduleWrite(_slaveIdA, 326, 1);
            else if (fireStep == 1) sent = _scheduleWrite(_slaveIdA, 325, 1);
            else if (fireStep == 2) sent = _scheduleWrite(_slaveIdB, 326, 1);
            else if (fireStep == 3) sent = _scheduleWrite(_slaveIdB, 325, 1);

            if (sent) {
#if DOOR_STATE_LOG
                if (fireStep == 0) logDoorCmd("FIRE_OPEN(326)", 0, _slaveIdA, 326, 1);
                else if (fireStep == 1) logDoorCmd("FIRE_OPEN(325)", 0, _slaveIdA, 325, 1);
                else if (fireStep == 2) logDoorCmd("FIRE_OPEN(326)", 1, _slaveIdB, 326, 1);
                else if (fireStep == 3) logDoorCmd("FIRE_OPEN(325)", 1, _slaveIdB, 325, 1);
#endif
                setOut(chRedA, false);
                setOut(chGreenA, true);
                setOut(chStatusA, true);
                setOut(chRedB, false);
                setOut(chGreenB, true);
                setOut(chStatusB, true);
                setOut(chSoundEnter, false);
                setOut(chSignalPass, false);
                setOut(chSoundLeave, false);
                fireStep++;
                if (fireStep >= 4) phase = AlgoFireHold;
                return;
            }
        } else if (phase == AlgoFireExitSend) {
            bool sent = false;
            if (fireStep == 0) sent = _scheduleWrite(_slaveIdA, 327, 1);
            else if (fireStep == 1) sent = _scheduleWrite(_slaveIdB, 327, 1);

            if (sent) {
#if DOOR_STATE_LOG
                if (fireStep == 0) logDoorCmd("FIRE_CLOSE", 0, _slaveIdA, 327, 1);
                else if (fireStep == 1) logDoorCmd("FIRE_CLOSE", 1, _slaveIdB, 327, 1);
#endif
                fireStep++;
                if (fireStep >= 2) {
                    phase = AlgoIdle;
                }
                setOut(chRedA, true);
                setOut(chGreenA, false);
                setOut(chStatusA, false);
                setOut(chRedB, true);
                setOut(chGreenB, false);
                setOut(chStatusB, false);
                setOut(chSoundEnter, false);
                setOut(chSignalPass, false);
                setOut(chSoundLeave, false);
                return;
            }
        } else if (phase == AlgoOpenStartSend) {
            const uint8_t phy = (doorSlave(startDoor) == MODBUS_SLAVE_ID_ENTRY_A) ? _slaveIdA : _slaveIdB;
            if (_scheduleWrite(phy, 326, 1)) {
#if DOOR_STATE_LOG
                logDoorCmd("OPEN_START(326)", startDoor, phy, 326, 1);
#endif
                phase = AlgoOpenStartWait;
                tPhase = now;
                if (autoActive) {
                    setOut(chSoundLeave, false);
                    setOut(chSignalPass, false);
                    setOut(chSoundEnter, true);
                    enterPulseOffMs = now + 10;
                    if (startDoor == 0) {
                        setOut(chRedA, false);
                        setOut(chGreenA, true);
                        setOut(chStatusA, true);
                        setOut(chRedB, true);
                        setOut(chGreenB, false);
                        setOut(chStatusB, false);
                    } else {
                        setOut(chRedB, false);
                        setOut(chGreenB, true);
                        setOut(chStatusB, true);
                        setOut(chRedA, true);
                        setOut(chGreenA, false);
                        setOut(chStatusA, false);
                    }
                }
                return;
            }
        } else if (phase == AlgoCloseStartSend) {
            const uint8_t phy = (doorSlave(startDoor) == MODBUS_SLAVE_ID_ENTRY_A) ? _slaveIdA : _slaveIdB;
            if (_scheduleWrite(phy, 327, 1)) {
#if DOOR_STATE_LOG
                logDoorCmd("CLOSE_START", startDoor, phy, 327, 1);
#endif
                if (autoActive) {
                    if (startDoor == 0) {
                        setOut(chRedA, true);
                        setOut(chGreenA, false);
                        setOut(chStatusA, false);
                    } else {
                        setOut(chRedB, true);
                        setOut(chGreenB, false);
                        setOut(chStatusB, false);
                    }
                }
                if (openSecondAfterClose) {
                    openSecondAfterClose = false;
                    phase = AlgoWaitOpenSecondDelay;
                } else {
                    phase = AlgoIdle;
                }
                tPhase = now;
                return;
            }
        } else if (phase == AlgoOpenSecondSend) {
            const uint8_t phy = (doorSlave(secondDoor) == MODBUS_SLAVE_ID_ENTRY_A) ? _slaveIdA : _slaveIdB;
            if (_scheduleWrite(phy, 325, 1)) {
#if DOOR_STATE_LOG
                logDoorCmd("OPEN_SECOND(325)", secondDoor, phy, 325, 1);
#endif
                secondHadPeople = false;
                secondOpenMs = 0;
                secondEmptySinceMs = 0;
                phase = AlgoOpenSecondWait;
                tPhase = now;
                if (autoActive) {
                    setOut(chSoundEnter, false);
                    if (secondDoor == 0) {
                        setOut(chRedA, false);
                        setOut(chGreenA, true);
                        setOut(chStatusA, true);
                    } else {
                        setOut(chRedB, false);
                        setOut(chGreenB, true);
                        setOut(chStatusB, true);
                    }
                }
                return;
            }
        } else if (phase == AlgoCloseSecondSend) {
            const uint8_t phy = (doorSlave(secondDoor) == MODBUS_SLAVE_ID_ENTRY_A) ? _slaveIdA : _slaveIdB;
            if (_scheduleWrite(phy, 327, 1)) {
#if DOOR_STATE_LOG
                logDoorCmd("CLOSE_SECOND", secondDoor, phy, 327, 1);
#endif
                if (autoActive) {
                    setOut(chSignalPass, false);
                    setOut(chSoundEnter, false);
                    if (leaveAlert) {
                        setOut(chSoundLeave, false);
                        leaveAlert = false;
                    }
                    if (secondDoor == 0) {
                        setOut(chRedA, true);
                        setOut(chGreenA, false);
                        setOut(chStatusA, false);
                    } else {
                        setOut(chRedB, true);
                        setOut(chGreenB, false);
                        setOut(chStatusB, false);
                    }
                }
                phase = AlgoIdle;
                tPhase = now;
                return;
            }
        }
    }

    const uint8_t slaveId = (_pollSlaveIdx == 0) ? _slaveIdA : _slaveIdB;
    uint16_t* cache = (_pollSlaveIdx == 0) ? _regsEntryA : _regsEntryB;

    const uint16_t maxAddr = MODBUS_REG_COUNT;
    const uint16_t blockSize = 20;
    if (_pollAddr >= maxAddr) {
        _pollAddr = 0;
        _pollSlaveIdx = (_pollSlaveIdx == 0) ? 1 : 0;
        return;
    }
    uint16_t cnt = maxAddr - _pollAddr;
    if (cnt > blockSize) cnt = blockSize;

    uint16_t tryCnt = cnt;
    bool ok = _scheduleRead(slaveId, _pollAddr, tryCnt, cache + _pollAddr);
    if (!ok && tryCnt > 1) {
        tryCnt = (uint16_t)(tryCnt / 2);
        ok = _scheduleRead(slaveId, _pollAddr, tryCnt, cache + _pollAddr);
    }
    if (ok) {
        _pollAddr += tryCnt;
    } else {
        _pollAddr = 0;
        _pollSlaveIdx = (_pollSlaveIdx == 0) ? 1 : 0;
    }
}

uint16_t ModbusRS485::readCache(uint8_t slaveId, uint16_t address) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
        if (address < MODBUS_REG_COUNT) return _regsEntryA[address];
    } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
        if (address < MODBUS_REG_COUNT) return _regsEntryB[address];
    }
    return 0;
}

bool ModbusRS485::writeRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
        if (address < MODBUS_REG_COUNT) {
            _regsEntryA[address] = value;
            return _scheduleWrite(_slaveIdA, address, value);
        }
    } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
        if (address < MODBUS_REG_COUNT) {
            _regsEntryB[address] = value;
            return _scheduleWrite(_slaveIdB, address, value);
        }
    }
    return false;
}

bool ModbusRS485::writeRegisters(uint8_t slaveId, uint16_t address, const uint16_t* values, uint16_t count) {
    if (!values || !count) return false;
    if (address + count > MODBUS_REG_COUNT) return false;

    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) {
        for (uint16_t i = 0; i < count; i++) _regsEntryA[address + i] = values[i];
        return _scheduleWriteMulti(_slaveIdA, address, values, count);
    } else if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) {
        for (uint16_t i = 0; i < count; i++) _regsEntryB[address + i] = values[i];
        return _scheduleWriteMulti(_slaveIdB, address, values, count);
    }
    return false;
}

bool ModbusRS485::queueWriteRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    if (slaveId != MODBUS_SLAVE_ID_ENTRY_A && slaveId != MODBUS_SLAVE_ID_ENTRY_B) return false;
    if (address >= MODBUS_REG_COUNT) return false;
    if (qFull()) return false;

    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) _regsEntryA[address] = value;
    else _regsEntryB[address] = value;

    PendingWrite w{};
    w.slave = slaveId;
    w.addr = address;
    w.value = value;
    qPush(w);
    return true;
}

uint16_t* ModbusRS485::getCachePtr(uint8_t slaveId) {
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_A) return _regsEntryA;
    if (slaveId == MODBUS_SLAVE_ID_ENTRY_B) return _regsEntryB;
    return nullptr;
}
