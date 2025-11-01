// =================================================================
// Plik:          ActuatorManager.cpp
// Wersja:        5.32d
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [FIX] Dodano jawne ustawienie pinów na LOW w funkcji begin()
//    dla logiki Active HIGH (LOW=OFF).
// =================================================================
#include <Arduino.h>
#include "ActuatorManager.h"

ActuatorManager::ActuatorManager() {}

void ActuatorManager::begin() {
    pinMode(HEATER_PIN_MAIN, OUTPUT);
    pinMode(HEATER_PIN_AUX1, OUTPUT);
    pinMode(HEATER_PIN_AUX2, OUTPUT);
    pinMode(CHAMBER_FAN_PIN, OUTPUT);
    pinMode(VENTILATION_FAN_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(HEATER_LED_PIN, OUTPUT);

    // ================== POCZĄTEK ZMIANY v5.32d ==================
    // Jawne ustawienie stanu początkowego LOW (OFF dla Active HIGH)
    digitalWrite(HEATER_PIN_MAIN, LOW);
    digitalWrite(HEATER_PIN_AUX1, LOW);
    digitalWrite(HEATER_PIN_AUX2, LOW);
    digitalWrite(CHAMBER_FAN_PIN, LOW);
    digitalWrite(VENTILATION_FAN_PIN, LOW);
    digitalWrite(HEATER_LED_PIN, LOW);
    noTone(BUZZER_PIN); // Upewnij się, że buzzer jest cicho
    // =================== KONIEC ZMIANY v5.32d ===================

    // resetAllForced() dodatkowo potwierdzi stan LOW
    resetAllForced();
}

void ActuatorManager::setHeaterPowerLevel(int level) {
    // Logika standardowa (Active HIGH)
    bool mainState = (level >= 1);
    bool aux1State = (level >= 2);
    bool aux2State = (level >= 3);

    if (mainState != isHeaterMainOn_internal) {
        digitalWrite(HEATER_PIN_MAIN, mainState ? HIGH : LOW);
        isHeaterMainOn_internal = mainState;
    }
    if (aux1State != isHeaterAux1On_internal) {
        digitalWrite(HEATER_PIN_AUX1, aux1State ? HIGH : LOW);
        isHeaterAux1On_internal = aux1State;
    }
    if (aux2State != isHeaterAux2On_internal) {
        digitalWrite(HEATER_PIN_AUX2, aux2State ? HIGH : LOW);
        isHeaterAux2On_internal = aux2State;
    }
}

void ActuatorManager::update(DryerState& state) {
    if (state.isInTestMode) {
        return;
    }

    // Sterowanie grzałkami
    if (state.currentMode != MODE_IDLE && !state.isInAlarmState) {
        setHeaterPowerLevel((int)state.pidOutput);
    } else {
        setHeaterPowerLevel(0);
    }

    // Wentylator komory
    if (state.currentPhase == PHASE_BOOST) {
        setChamberFan(true);
    } else if (state.currentMode != MODE_IDLE) {
        float max_t = -999.0, min_t = 999.0;
        for (int i : {0, 1, 2, 4}) {
            if (state.ds18b20_temps[i] > max_t) max_t = state.ds18b20_temps[i];
            if (state.ds18b20_temps[i] < min_t) min_t = state.ds18b20_temps[i];
        }
        float d = max_t - min_t;
        if (!isChamberFanOn_internal && d > CHAMBER_DELTA_ON) {
            setChamberFan(true);
        } else if (isChamberFanOn_internal && d < CHAMBER_DELTA_OFF) {
            setChamberFan(false);
        }
    } else {
        setChamberFan(false);
    }
    state.isChamberFanOn = isChamberFanOn_internal;

    // Wentylator wentylacji (Przyszłość)
    if (state.currentMode == MODE_IDLE) {
        setVentilationFan(false);
    }
    // state.isVentilationFanOn = isVentilationFanOn_internal; // Aktualizacja stanu w DryerState, jeśli potrzebne

    // Dioda LED
    setHeaterLed(state.isHeaterOn);

    // Buzzer alarmu
    if (isAlarmActive) {
        if (millis() - lastAlarmBeepTime > 1000) {
            lastAlarmBeepTime = millis();
            isAlarmBeeping = true;
            tone(BUZZER_PIN, 2000);
        }
        if (isAlarmBeeping && millis() - lastAlarmBeepTime > 200) {
            isAlarmBeeping = false;
            noTone(BUZZER_PIN);
        }
    } else {
        noTone(BUZZER_PIN);
    }
}

void ActuatorManager::playCompletionSound() {
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 1000, 80);
        delay(160);
    }
}

void ActuatorManager::playAlarmSound(bool play) {
    if (play && !isAlarmActive) {
        isAlarmActive = true;
        lastAlarmBeepTime = millis() - 1001;
    } else if (!play) {
        isAlarmActive = false;
        isAlarmBeeping = false;
        noTone(BUZZER_PIN);
    }
}

void ActuatorManager::forceHeaterMain(bool state) {
    digitalWrite(HEATER_PIN_MAIN, state ? HIGH : LOW);
    isHeaterMainOn_internal = state;
}
void ActuatorManager::forceHeaterAux1(bool state) {
    digitalWrite(HEATER_PIN_AUX1, state ? HIGH : LOW);
    isHeaterAux1On_internal = state;
}
void ActuatorManager::forceHeaterAux2(bool state) {
    digitalWrite(HEATER_PIN_AUX2, state ? HIGH : LOW);
    isHeaterAux2On_internal = state;
}
void ActuatorManager::forceChamberFan(bool state) {
    digitalWrite(CHAMBER_FAN_PIN, state ? HIGH : LOW);
    isChamberFanOn_internal = state;
}
void ActuatorManager::forceVentilationFan(bool state) {
    digitalWrite(VENTILATION_FAN_PIN, state ? HIGH : LOW);
    isVentilationFanOn_internal = state;
}
void ActuatorManager::forceBuzzer(bool state) {
    if (state) {
        tone(BUZZER_PIN, 1000);
    } else {
        noTone(BUZZER_PIN);
    }
}
void ActuatorManager::forceHeaterLed(bool state) {
    digitalWrite(HEATER_LED_PIN, state ? HIGH : LOW);
}

void ActuatorManager::resetAllForced() {
    forceHeaterMain(false); // Ustawia LOW
    forceHeaterAux1(false); // Ustawia LOW
    forceHeaterAux2(false); // Ustawia LOW
    forceChamberFan(false); // Ustawia LOW
    forceVentilationFan(false); // Ustawia LOW
    forceBuzzer(false);
    forceHeaterLed(false); // Ustawia LOW
}

void ActuatorManager::setChamberFan(bool s) {
    if (s != isChamberFanOn_internal) {
        digitalWrite(CHAMBER_FAN_PIN, s ? HIGH : LOW);
        isChamberFanOn_internal = s;
    }
}

void ActuatorManager::setVentilationFan(bool s) {
    if (s != isVentilationFanOn_internal) {
        digitalWrite(VENTILATION_FAN_PIN, s ? HIGH : LOW);
        isVentilationFanOn_internal = s;
    }
}

void ActuatorManager::setHeaterLed(bool s) {
    digitalWrite(HEATER_LED_PIN, s ? HIGH : LOW);
}