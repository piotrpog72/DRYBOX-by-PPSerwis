// =================================================================
// Plik:          ActuatorManager.cpp
// Wersja:        5.28
// Data:          16.10.2025
// Opis Zmian:
//  - [REFACTOR] Zaktualizowano logikę wentylatora komory, aby
//    korzystała z nowego enuma `HeatingPhase`.
// =================================================================
#include <Arduino.h>
#include "ActuatorManager.h"

ActuatorManager::ActuatorManager() {}

void ActuatorManager::begin() {
    pinMode(HEATER_PIN, OUTPUT);
    pinMode(HEATER_FAN_PIN, OUTPUT);
    pinMode(CHAMBER_FAN_PIN, OUTPUT);
    pinMode(POWER_SUPPLY_FAN_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(HEATER_LED_PIN, OUTPUT);
    resetAllForced();
    windowStartTime = millis();
}

void ActuatorManager::setHeater(bool state) {
    if (state != isHeaterOn_internal) {
        digitalWrite(HEATER_PIN, state);
        isHeaterOn_internal = state;
        if (!isHeaterOn_internal) { 
            heaterOffTime = millis();
            setHeaterFan(true); 
        } else { 
             setHeaterFan(true); 
        }
    }
}

void ActuatorManager::update(DryerState& state) {
    if (state.isInTestMode) {
        return;
    }

    // Sterowanie grzałką (LOGIKA CZASOWO-PROPORCJONALNA)
    if (state.currentMode != MODE_IDLE && !state.isInAlarmState) {
        unsigned long now = millis();
        if (now - windowStartTime > windowSize) {
            windowStartTime = now;
        }
        
        long onTime = (state.pidOutput * windowSize) / 255;
        
        if ((now - windowStartTime) < onTime) {
            setHeater(true);
        } else {
            setHeater(false);
        }
    } else {
        setHeater(false);
    }

    // Sterowanie wentylatorem komory
    // ================== POCZĄTEK ZMIANY v5.26 ==================
    if (state.currentPhase == PHASE_BOOST) {
        setChamberFan(true); // Wymuszona praca w fazie Boost
    } else if (state.currentMode != MODE_IDLE) {
    // =================== KONIEC ZMIANY v5.26 ===================
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


    // Reszta logiki
    float p_on = state.psuFanOnTemp;
    float p_off = p_on - state.psuFanOffHysteresis;
    if (state.ds18b20_temps[3] > p_on) {
        setPsuFan(true);
    } else if (state.ds18b20_temps[3] < p_off) {
        setPsuFan(false);
    }
    state.isPsuFanOn = isPsuFanOn_internal;

    if (!isHeaterOn_internal && heaterOffTime > 0) {
        if (millis() - heaterOffTime >= HEATER_FAN_COOLDOWN_TIME) {
            setHeaterFan(false);
            heaterOffTime = 0;
        }
    }
    state.isHeaterFanOn = isHeaterFanOn_internal;
    
    setHeaterLed(state.isHeaterOn); 

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

void ActuatorManager::forceHeater(bool state) { 
    setHeater(state);
}
void ActuatorManager::forceHeaterFan(bool state) { digitalWrite(HEATER_FAN_PIN, state); }
void ActuatorManager::forceChamberFan(bool state) { digitalWrite(CHAMBER_FAN_PIN, state); }
void ActuatorManager::forcePsuFan(bool state) { digitalWrite(POWER_SUPPLY_FAN_PIN, state); }
void ActuatorManager::forceBuzzer(bool state) {
    if (state) {
        tone(BUZZER_PIN, 1000);
    } else {
        noTone(BUZZER_PIN);
    }
}
void ActuatorManager::forceHeaterLed(bool state) { digitalWrite(HEATER_LED_PIN, state); }

void ActuatorManager::resetAllForced() {
    forceHeater(false);
    forceHeaterFan(false);
    forceChamberFan(false);
    forcePsuFan(false);
    forceBuzzer(false);
    forceHeaterLed(false);
}

void ActuatorManager::setHeaterFan(bool s) {
    if (s != isHeaterFanOn_internal) {
        digitalWrite(HEATER_FAN_PIN, s);
        isHeaterFanOn_internal = s;
    }
}

void ActuatorManager::setChamberFan(bool s) {
    if (s != isChamberFanOn_internal) {
        digitalWrite(CHAMBER_FAN_PIN, s);
        isChamberFanOn_internal = s;
    }
}

void ActuatorManager::setPsuFan(bool s) {
    if (s != isPsuFanOn_internal) {
        digitalWrite(POWER_SUPPLY_FAN_PIN, s);
        isPsuFanOn_internal = s;
    }
}

void ActuatorManager::setHeaterLed(bool s) {
    digitalWrite(HEATER_LED_PIN, s);
}