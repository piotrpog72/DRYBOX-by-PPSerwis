// =================================================================
// Plik:          ActuatorManager.cpp
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REFACTOR] Implementacja sterowania 3 grzałkami (Active HIGH).
//  - [FEATURE] Implementacja logiki cyklera wentylacji.
//  - [REFACTOR] Usunięto logikę wentylatorów grzałki i zasilacza.
// =================================================================
#include <Arduino.h>
#include "ActuatorManager.h"

ActuatorManager::ActuatorManager() {}

void ActuatorManager::begin() {
    pinMode(HEATER_PIN_MAIN, OUTPUT);
    pinMode(HEATER_PIN_AUX1, OUTPUT);
    pinMode(HEATER_PIN_AUX2, OUTPUT);
    pinMode(VENTILATION_FAN_PIN, OUTPUT);
    pinMode(CHAMBER_FAN_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(HEATER_LED_PIN, OUTPUT);
    
    resetAllForced(); // Ustawia wszystkie piny na LOW (OFF)
}

// Logika Active HIGH: LOW=OFF, HIGH=ON
// Poziom 0: OFF
// Poziom 1: Tylko Środek (23W) - Pin 23
// Poziom 2: Tylko Boki (46W) - Pin 27, 19
// Poziom 3: Wszystkie (69W) - Pin 23, 27, 19
void ActuatorManager::setHeaterPowerLevel(int level) {
    bool mainState = false; // Pin 23
    bool aux1State = false; // Pin 27
    bool aux2State = false; // Pin 19

    if (level == 1) {
        mainState = true;
    } else if (level == 2) {
        aux1State = true;
        aux2State = true;
    } else if (level >= 3) {
        mainState = true;
        aux1State = true;
        aux2State = true;
    }

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
    
    // Aktualizacja flag globalnych dla UI
    state.isHeaterMainOn = isHeaterMainOn_internal;
    state.isHeaterAux1On = isHeaterAux1On_internal;
    state.isHeaterAux2On = isHeaterAux2On_internal;
    state.isHeaterOn = (isHeaterMainOn_internal || isHeaterAux1On_internal || isHeaterAux2On_internal);

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

    // Sterowanie wentylatorem wentylacji (Cykl Czasowy)
    unsigned long now = millis();
    bool shouldVentilate = (state.currentMode != MODE_IDLE) && (state.dhtHum > state.targetHumidity);

    if (shouldVentilate) {
        if (isVenting) {
            // Jesteśmy w trakcie wentylacji, sprawdź czy zakończyć
            if (now - lastVentilationStartTime >= (state.ventilationDuration_sec * 1000UL)) {
                isVenting = false;
                setVentilationFan(false);
                // Nie resetujemy lastVentilationStartTime, aby interwał liczył się od startu
            }
        } else {
            // Nie wentylujemy, sprawdź czy zacząć
            // (Sprawdzamy od startu suszenia lub od ostatniego *startu* wentylacji)
            if (lastVentilationStartTime == 0 || (now - lastVentilationStartTime >= (state.ventilationInterval_min * 60000UL))) {
                isVenting = true;
                setVentilationFan(true);
                lastVentilationStartTime = now; // Resetuj timer interwału
            }
        }
    } else {
        // Suszenie wyłączone lub cel wilgotności osiągnięty
        setVentilationFan(false);
        isVenting = false;
        if (lastVentilationStartTime != 0) { // Zresetuj timer, aby był gotowy na następny cykl
             lastVentilationStartTime = 0;
        }
    }
    state.isVentilationFanOn = isVentilationFanOn_internal;
    
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
void ActuatorManager::forceVentilationFan(bool state) {
    digitalWrite(VENTILATION_FAN_PIN, state ? HIGH : LOW);
    isVentilationFanOn_internal = state;
}
void ActuatorManager::forceChamberFan(bool state) {
    digitalWrite(CHAMBER_FAN_PIN, state ? HIGH : LOW);
    isChamberFanOn_internal = state;
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
    forceHeaterMain(false);
    forceHeaterAux1(false);
    forceHeaterAux2(false);
    forceVentilationFan(false);
    forceChamberFan(false);
    forceBuzzer(false);
    forceHeaterLed(false);
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