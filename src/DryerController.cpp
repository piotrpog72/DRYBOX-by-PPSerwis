// =================================================================
// Plik:         DryerController.cpp
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [TASK] Implementacja logiki progów procentowych w logice grzania
//          (PHASE_BOOST, PHASE_RAMP, PHASE_PID).
// - [CHORE] Usunięto logikę boostPsuTempLimit i rampPowerPercent.
// - [TASK] Aktualizacja processWebCommand do obsługi 4-parametrowego
//          polecenia START (z czasem).
// - [TASK] Aktualizacja processUserInput (menu) dla nowych opcji %.
// - [TASK] Aktualizacja load/saveSettings dla nowych zmiennych %.
// =================================================================
#include "DryerController.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include "lists.h"

DryerController::DryerController() :
  pid(&pidInput, &pidOutput, &pidSetpoint, currentState.pid_kp, currentState.pid_ki, currentState.pid_kd, DIRECT),
  webManager(currentState, profiles) // Inicjalizacja WebManagera
{}

void DryerController::begin() {
    Serial.begin(115200); Serial.println("DryerController: Inicjalizacja " FW_VERSION "...");
    if (!LittleFS.begin(true)) { Serial.println("Błąd inicjalizacji LittleFS."); }
    initializeProfiles();
    loadSettings();
    
    sensorManager.begin();
    displayManager.begin();
    inputManager.begin();
    actuatorManager.begin();
    
    displayManager.setContrast(currentState.glcdContrast);
    displayManager.showStartupScreen(currentState); delay(STARTUP_SCREEN_DELAY);
    
    if (currentState.isWifiEnabled) {
        WiFi.mode(WIFI_STA);
        WiFiManager wm;
        if (wm.autoConnect("Drybox_Setup")) {
            Serial.println("Polaczono z siecia przez WiFiManager.");
            webManager.begin(); // Uruchomienie WebManagera
        } else {
            Serial.println("Nie udalo sie polaczyc z WiFi.");
        }
    }

    currentState.sensorsOk = sensorManager.areSensorsFound();
    currentState.lastUserInputTime = millis();
    lastHeaterUpdateTime = millis();
    currentState.targetTemp = profiles[currentState.currentProfileType].temp;
    currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;

    pid.SetOutputLimits(0, 1); 
    pid.SetSampleTime(1000);
    pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
    pid.SetMode(MANUAL);
}

void DryerController::startWifiConfig() {
    displayManager.showWifiConfigScreen();
    WiFiManager wm;
    wm.startConfigPortal("Drybox_Setup");
}

void DryerController::handleWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!currentState.isWifiConnected) {
            currentState.isWifiConnected = true;
            Serial.println("Połączono z WiFi!");
            Serial.print("Adres IP: ");
            Serial.println(WiFi.localIP());
            ArduinoOTA.setHostname("Drybox");
            ArduinoOTA.begin();
        }
        ArduinoOTA.handle();
    } else {
        if (currentState.isWifiConnected) {
            currentState.isWifiConnected = false;
            ArduinoOTA.end();
            Serial.println("Utracono połączenie WiFi.");
        }
    }
}

void DryerController::startDryingProcess() {
    currentState.dryingStartTime = millis();
    currentState.startingHumidity = currentState.dhtHum;
    currentState.heaterTotalOnTime_ms = 0;
    lastHeaterUpdateTime = millis();
    currentState.currentPhase = PHASE_BOOST;
    currentState.currentMenuState = SCREEN_MAIN;
}

void DryerController::update() {
    if(currentState.isWifiEnabled) {
        webManager.update(); // Aktualizacja WebManagera
        handleWifi();

        String command = webManager.getCommand();
        if (command.length() > 0) {
            processWebCommand(command);
        }
    }
    
    unsigned long currentTime = millis();
    inputManager.update();  
    processUserInput(); // Używamy lokalnej funkcji processUserInput
    
    if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
        lastSensorReadTime = currentTime;
        sensorManager.readSensors(&currentState);
    }
    
    if (!currentState.isInTestMode) {
        currentState.previousMode = currentState.currentMode;
        
        bool alarmCondition = (currentState.avgChamberTemp > OVERHEAT_TEMP_LIMIT) || (currentState.ds18b20_temps[3] > currentState.psuOverheatLimit);
        if (alarmCondition && !currentState.isInAlarmState) {
            currentState.isInAlarmState = true;
            currentState.currentMode = MODE_IDLE;
            if (currentState.areSoundsEnabled) {
                actuatorManager.playAlarmSound(true);
            }
        } else if (!alarmCondition && currentState.isInAlarmState) {
            currentState.isInAlarmState = false;
            actuatorManager.playAlarmSound(false);
        }

        // ================== POCZĄTEK ZMIANY v5.35 ==================
        int desiredPowerLevel = 0; // Domyślnie wyłączone

        if (currentState.currentMode != MODE_IDLE && !currentState.isInAlarmState) {
            
            // Obliczanie progów procentowych
            const float boostStopTemp = (currentState.targetTemp * currentState.boostThresholdPercent) / 100.0;
            const float rampStopTemp = (currentState.targetTemp * currentState.rampThresholdPercent) / 100.0;
            // Próg Failsafe dla PID ustawiamy np. 1 stopień poniżej progu RAMP
            const float pidFailsafeThreshold = rampStopTemp - 1.0; 

            switch (currentState.currentPhase) {
                
                case PHASE_BOOST: {
                    desiredPowerLevel = 3; // Max moc (69W)

                    bool boostTimeElapsed = (millis() - currentState.dryingStartTime) > (currentState.boostMaxTime_min * 60000UL);
                    bool tempThresholdReached = currentState.avgChamberTemp >= boostStopTemp;
                    // Usunięto psuTempLimitReached

                    if (boostTimeElapsed || tempThresholdReached) {
                         currentState.currentPhase = PHASE_RAMP;
                         Serial.println("HEATING: Boost -> Ramp");
                    }
                    break;
                }

                case PHASE_RAMP: {
                    desiredPowerLevel = 2; // Poziom 2 (46W) - Hardcoded

                    if (currentState.avgChamberTemp >= rampStopTemp) {
                        pid.SetMode(AUTOMATIC); 
                        currentState.currentPhase = PHASE_PID;
                        Serial.println("HEATING: Ramp -> PID (Level 1 + Failsafe)");
                    }
                    break;
                }

                case PHASE_PID: {
                    pidInput = currentState.avgChamberTemp;
                    pidSetpoint = currentState.targetTemp;
                    
                    if (currentState.avgChamberTemp < pidFailsafeThreshold) {
                        desiredPowerLevel = 2; // Wymuś 46W (Poziom 2) - Hardcoded
                        Serial.println("HEATING: PID Failsafe -> Ramp Level");
                    } else {
                        if(pid.Compute()){
                           desiredPowerLevel = (int)pidOutput; // pidOutput to 0 lub 1
                        } else {
                           desiredPowerLevel = (int)currentState.pidOutput; // Zachowaj poprzedni stan
                        }
                    }
                    break;
                }

                case PHASE_OFF:
                default:
                    desiredPowerLevel = 0;
                    break;
            }
        } else { // Tryb IDLE lub ALARM
            desiredPowerLevel = 0;
            if (currentState.currentPhase != PHASE_OFF) {
                currentState.currentPhase = PHASE_OFF;
                pid.SetMode(MANUAL);
            }
        }
        currentState.pidOutput = desiredPowerLevel; // Zapisz wybrany poziom mocy (0, 1, 2 lub 3)
        // =================== KONIEC ZMIANY v5.35 ===================
        
        actuatorManager.update(currentState);

        if(currentState.isHeaterOn) {
            currentState.heaterTotalOnTime_ms += (currentTime - lastHeaterUpdateTime);
        }
        lastHeaterUpdateTime = currentTime;
        
        if (currentState.currentMode == MODE_HUMIDITY_TARGET && currentState.dhtHum <= currentState.targetHumidity && currentState.dryingStartTime > 0) {
            currentState.currentMode = MODE_IDLE;
        }
        if (currentState.currentMode == MODE_TIMED && currentState.dryingStartTime > 0) {
            unsigned long elapsedTime_ms = millis() - currentState.dryingStartTime;
            if (elapsedTime_ms >= (currentState.dryingDurationMinutes * 60000UL)) {
                currentState.currentMode = MODE_IDLE;
            }
        }
        
        if (currentState.currentMode == MODE_IDLE && currentState.previousMode != MODE_IDLE && !currentState.isInAlarmState) {
            if (currentState.areSoundsEnabled) {
                actuatorManager.playCompletionSound();
            }
        }
    } 

    displayManager.update(currentState, profiles[currentState.currentProfileType].name);

    if (currentState.interactiveDisplayActive && (millis() - currentState.lastUserInputTime > INTERACTIVE_DISPLAY_TIMEOUT)) {
        currentState.interactiveDisplayActive = false;
        displayManager.sleepInteractive();
    }
}

void DryerController::processWebCommand(String command) {
    if (command == "STOP") {
        currentState.currentMode = MODE_IDLE;
    } else if (command.startsWith("START:")) {
        // ================== POCZĄTEK ZMIANY v5.35 ==================
        // Format: START:PROFIL:TRYB[:CZAS]
        int firstColon = command.indexOf(':');
        int secondColon = command.indexOf(':', firstColon + 1);
        int thirdColon = command.indexOf(':', secondColon + 1); // Szukaj trzeciego separatora

        int profileIndex = command.substring(firstColon + 1, secondColon).toInt();
        int modeIndex = 0;
        
        if (thirdColon != -1) { // Znaleziono 4 parametry (np. START:0:0:360)
            modeIndex = command.substring(secondColon + 1, thirdColon).toInt();
        } else { // Znaleziono 3 parametry (np. START:0:1)
            modeIndex = command.substring(secondColon + 1).toInt();
        }

        currentState.currentProfileType = (ProfileType)profileIndex;
        currentState.targetTemp = profiles[profileIndex].temp;
        currentState.targetHumidity = profiles[profileIndex].humidity;
        
        switch(modeIndex) {
            case 0: 
                currentState.currentMode = MODE_TIMED;
                if (thirdColon != -1) { // Jeśli podano czas
                    int timeMins = command.substring(thirdColon + 1).toInt();
                    if (timeMins > 0) {
                        currentState.dryingDurationMinutes = timeMins;
                    } else {
                        currentState.dryingDurationMinutes = 360; // Domyślny, jeśli błąd
                    }
                } else {
                    currentState.dryingDurationMinutes = 360; // Domyślny, jeśli nie podano
                }
                break;
            case 1: 
                currentState.currentMode = MODE_HUMIDITY_TARGET;
                break;
            case 2: 
                currentState.currentMode = MODE_CONTINUOUS;
                break;
        }
        // =================== KONIEC ZMIANY v5.35 ===================
        startDryingProcess();
    } else if (command == "SAVE_SETTINGS") {
        saveSettings();
        pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
        displayManager.setContrast(currentState.glcdContrast);
        Serial.println("DryerController: Ustawienia zapisane z Web UI.");
    }
}

void DryerController::processUserInput() {
    bool short_pressed = inputManager.shortPressTriggered();
    bool long_pressed = inputManager.longPressTriggered();
    
    if (short_pressed || long_pressed) {
        currentState.lastUserInputTime = millis();
        if (!currentState.interactiveDisplayActive) {
            currentState.interactiveDisplayActive = true;
            displayManager.wakeUpInteractive();
            return;
        }
    }

    if (long_pressed) {
        switch (currentState.currentMenuState) {
            case SCREEN_MAIN: currentState.currentMenuState = SCREEN_SPOOL_STATUS; break;
            case SCREEN_SPOOL_STATUS: currentState.currentMenuState = SCREEN_MAIN; break;
            case MENU_PROFILE_SELECT: currentState.currentMenuState = MENU_MAIN_SELECT; inputManager.resetEncoder(); currentState.menu_selection = 0; break;
            case MENU_MODE_SELECT: currentState.currentMenuState = MENU_PROFILE_SELECT; inputManager.resetEncoder(currentState.currentProfileType); break;
            case MENU_SET_TIME: currentState.currentMenuState = MENU_MODE_SELECT; inputManager.resetEncoder(0); break;
            case MENU_SETTINGS: currentState.currentMenuState = MENU_MAIN_SELECT; inputManager.resetEncoder(); currentState.menu_selection = 0; break;
            case MENU_SET_CUSTOM_TEMP: currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(0); break;
            case MENU_SET_CUSTOM_HUM: currentState.currentMenuState = MENU_SET_CUSTOM_TEMP; inputManager.resetEncoder((int)profiles[PROFILE_CUSTOM].temp); break;
            case MENU_SPOOL_SELECT: saveSettings(); currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(4); break;
            case MENU_SPOOL_SET_TYPE: currentState.currentMenuState = MENU_SPOOL_SELECT; inputManager.resetEncoder(editingSpoolIndex); break;
            case MENU_SPOOL_SET_COLOR: currentState.currentMenuState = MENU_SPOOL_SET_TYPE; inputManager.resetEncoder(currentState.spools[editingSpoolIndex].typeIndex); break;
            case MENU_ADVANCED_SETTINGS: currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(2); break;
            case MENU_WIFI_SETTINGS: currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(3); break;
            case SCREEN_WIFI_STATUS: currentState.currentMenuState = MENU_WIFI_SETTINGS; inputManager.resetEncoder(2); break;
            
            case MENU_COMPONENT_TEST:
                 currentState.isInTestMode = false;
                 actuatorManager.resetAllForced();
                 currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                 // ================== POCZĄTEK ZMIANY v5.35 ==================
                 inputManager.resetEncoder(10); // Zaktualizowany indeks (było 11)
                 // =================== KONIEC ZMIANY v5.35 ===================
                 break;
            
            // Powrót z menu ustawień wartości
            case MENU_SET_PSU_OVERHEAT_LIMIT: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(0); break;
            case MENU_SET_CONTRAST: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(1); break;
            case MENU_SET_PID_KP: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(2); break;
            case MENU_SET_PID_KI: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(3); break;
            case MENU_SET_PID_KD: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(4); break;
            case MENU_SET_BOOST_TIME: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(5); break;
            // ================== POCZĄTEK ZMIANY v5.35 ==================
            case MENU_SET_BOOST_THRESHOLD_PERCENT: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(6); break;
            case MENU_SET_RAMP_THRESHOLD_PERCENT: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(7); break;
            case MENU_SET_VENT_INTERVAL: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(8); break;
            case MENU_SET_VENT_DURATION: currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(9); break;
            // =================== KONIEC ZMIANY v5.35 ===================
        }
    }

    if (short_pressed) {
        switch (currentState.currentMenuState) {
            case SCREEN_MAIN: if (currentState.isInAlarmState) break;
            case SCREEN_SPOOL_STATUS: case SCREEN_WIFI_STATUS:
                currentState.currentMenuState = MENU_MAIN_SELECT;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
            
            case MENU_MAIN_SELECT:
                if (currentState.menu_selection == 0) {
                    if (currentState.currentMode != MODE_IDLE) {
                        currentState.currentMode = MODE_IDLE;
                        currentState.currentMenuState = SCREEN_MAIN;
                    } else if (!currentState.isInAlarmState) {
                        currentState.currentMenuState = MENU_PROFILE_SELECT;
                        inputManager.resetEncoder(); currentState.menu_selection = 0;
                    }
                } else if (currentState.menu_selection == 1) {
                    currentState.currentMenuState = MENU_SETTINGS;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 2) {
                    currentState.currentMenuState = SCREEN_MAIN;
                }
                break;
            
            case MENU_PROFILE_SELECT:
                currentState.currentProfileType = (ProfileType)currentState.menu_selection;
                currentState.targetTemp = profiles[currentState.currentProfileType].temp;
                currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;
                currentState.currentMenuState = MENU_MODE_SELECT;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
            
            case MENU_MODE_SELECT:
                switch(currentState.menu_selection) {
                    case 0: currentState.currentMenuState = MENU_SET_TIME; inputManager.resetEncoder(36); break;
                    case 1: currentState.currentMode = MODE_HUMIDITY_TARGET; startDryingProcess(); break;
                    case 2: currentState.currentMode = MODE_CONTINUOUS; startDryingProcess(); break;
                }
                break;
            
            case MENU_SET_TIME:
                currentState.dryingDurationMinutes = currentState.menu_selection * 10;
                if (currentState.dryingDurationMinutes > 0) {
                    currentState.currentMode = MODE_TIMED;
                    startDryingProcess();
                }
                break;
            
            case MENU_SETTINGS:
                if (currentState.menu_selection == 0) { // Edytuj Profil Własny
                    currentState.currentMenuState = MENU_SET_CUSTOM_TEMP;
                    inputManager.resetEncoder((int)profiles[PROFILE_CUSTOM].temp);
                } else if (currentState.menu_selection == 1) { // Dźwięki
                    currentState.areSoundsEnabled = !currentState.areSoundsEnabled;
                    saveSettings();
                } else if (currentState.menu_selection == 2) { // Zaawansowane
                    currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 3) { // WiFi
                    currentState.currentMenuState = MENU_WIFI_SETTINGS;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 4) { // Etykiety
                    currentState.currentMenuState = MENU_SPOOL_SELECT;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 5) { // Powrót
                    currentState.currentMenuState = MENU_MAIN_SELECT;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                }
                break;
            
            case MENU_ADVANCED_SETTINGS:
                // ================== POCZĄTEK ZMIANY v5.35 ==================
                switch(currentState.menu_selection) {
                    case 0: currentState.currentMenuState = MENU_SET_PSU_OVERHEAT_LIMIT; inputManager.resetEncoder((int)currentState.psuOverheatLimit); break;
                    case 1: currentState.currentMenuState = MENU_SET_CONTRAST; inputManager.resetEncoder(currentState.glcdContrast); break;
                    case 2: currentState.currentMenuState = MENU_SET_PID_KP; inputManager.resetEncoder((int)(currentState.pid_kp * 10)); break;
                    case 3: currentState.currentMenuState = MENU_SET_PID_KI; inputManager.resetEncoder((int)(currentState.pid_ki * 10)); break;
                    case 4: currentState.currentMenuState = MENU_SET_PID_KD; inputManager.resetEncoder((int)(currentState.pid_kd * 10)); break;
                    case 5: currentState.currentMenuState = MENU_SET_BOOST_TIME; inputManager.resetEncoder(currentState.boostMaxTime_min); break;
                    case 6: currentState.currentMenuState = MENU_SET_BOOST_THRESHOLD_PERCENT; inputManager.resetEncoder(currentState.boostThresholdPercent); break;
                    case 7: currentState.currentMenuState = MENU_SET_RAMP_THRESHOLD_PERCENT; inputManager.resetEncoder(currentState.rampThresholdPercent); break;
                    case 8: currentState.currentMenuState = MENU_SET_VENT_INTERVAL; inputManager.resetEncoder(currentState.ventilationInterval_min); break;
                    case 9: currentState.currentMenuState = MENU_SET_VENT_DURATION; inputManager.resetEncoder(currentState.ventilationDuration_sec); break;
                    case 10: // Test Podzespołów (nowy indeks 10)
                        currentState.isInTestMode = true;
                        currentState.test_heater_main = false; currentState.test_heater_aux1 = false; currentState.test_heater_aux2 = false;
                        currentState.test_chamber_fan = false; currentState.test_vent_fan = false;
                        currentState.test_buzzer = false; currentState.test_heater_led = false;
                        actuatorManager.resetAllForced();
                        currentState.currentMenuState = MENU_COMPONENT_TEST;
                        inputManager.resetEncoder(0);
                        break;
                    case 11: currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(2); break; // Powrót (nowy indeks 11)
                }
                // =================== KONIEC ZMIANY v5.35 ===================
                break;
            
            case MENU_COMPONENT_TEST:
                switch(currentState.menu_selection) {
                    case 0: currentState.test_heater_main = !currentState.test_heater_main; actuatorManager.forceHeaterMain(currentState.test_heater_main); break;
                    case 1: currentState.test_heater_aux1 = !currentState.test_heater_aux1; actuatorManager.forceHeaterAux1(currentState.test_heater_aux1); break;
                    case 2: currentState.test_heater_aux2 = !currentState.test_heater_aux2; actuatorManager.forceHeaterAux2(currentState.test_heater_aux2); break;
                    case 3: currentState.test_chamber_fan = !currentState.test_chamber_fan; actuatorManager.forceChamberFan(currentState.test_chamber_fan); break;
                    case 4: currentState.test_vent_fan = !currentState.test_vent_fan; actuatorManager.forceVentilationFan(currentState.test_vent_fan); break;
                    case 5: currentState.test_buzzer = !currentState.test_buzzer; actuatorManager.forceBuzzer(currentState.test_buzzer); break;
                    case 6: currentState.test_heater_led = !currentState.test_heater_led; actuatorManager.forceHeaterLed(currentState.test_heater_led); break;
                    case 7: // Powrót
                        currentState.isInTestMode = false;
                        actuatorManager.resetAllForced();
                        currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                        // ================== POCZĄTEK ZMIANY v5.35 ==================
                        inputManager.resetEncoder(10); // Zaktualizowany indeks (było 11)
                        // =================== KONIEC ZMIANY v5.35 ===================
                        break;
                }
                break;
            
            // Zapisywanie wartości i powrót
            case MENU_SET_CONTRAST: currentState.glcdContrast = currentState.menu_selection; displayManager.setContrast(currentState.glcdContrast); saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(1); break;
            case MENU_SET_PID_KP: currentState.pid_kp = (double)currentState.menu_selection / 10.0; pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd); saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(2); break;
            case MENU_SET_PID_KI: currentState.pid_ki = (double)currentState.menu_selection / 10.0; pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd); saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(3); break;
            case MENU_SET_PID_KD: currentState.pid_kd = (double)currentState.menu_selection / 10.0; pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd); saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(4); break;
            case MENU_SET_PSU_OVERHEAT_LIMIT: currentState.psuOverheatLimit = (float)currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(0); break;
            case MENU_SET_BOOST_TIME: currentState.boostMaxTime_min = currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(5); break;
            // ================== POCZĄTEK ZMIANY v5.35 ==================
            case MENU_SET_BOOST_THRESHOLD_PERCENT: currentState.boostThresholdPercent = currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(6); break;
            case MENU_SET_RAMP_THRESHOLD_PERCENT: currentState.rampThresholdPercent = currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(7); break;
            case MENU_SET_VENT_INTERVAL: currentState.ventilationInterval_min = currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(8); break;
            case MENU_SET_VENT_DURATION: currentState.ventilationDuration_sec = currentState.menu_selection; saveSettings(); currentState.currentMenuState = MENU_ADVANCED_SETTINGS; inputManager.resetEncoder(9); break;
            // =================== KONIEC ZMIANY v5.35 ===================

            case MENU_WIFI_SETTINGS:
                if (currentState.menu_selection == 0) { currentState.isWifiEnabled = !currentState.isWifiEnabled; saveSettings(); if (!currentState.isWifiEnabled) { WiFi.disconnect(true); } }
                else if (currentState.menu_selection == 1) { startWifiConfig(); currentState.currentMenuState = MENU_SETTINGS; }
                else if (currentState.menu_selection == 2) { currentState.currentMenuState = SCREEN_WIFI_STATUS; }
                else if (currentState.menu_selection == 3) { currentState.currentMenuState = MENU_SETTINGS; inputManager.resetEncoder(3); }
                break;
            
            case MENU_SPOOL_SELECT:
                editingSpoolIndex = currentState.menu_selection;
                currentState.currentMenuState = MENU_SPOOL_SET_TYPE;
                inputManager.resetEncoder(currentState.spools[editingSpoolIndex].typeIndex);
                break;
            case MENU_SPOOL_SET_TYPE:
                currentState.spools[editingSpoolIndex].typeIndex = currentState.menu_selection;
                if (currentState.menu_selection == 0) { currentState.spools[editingSpoolIndex].colorIndex = 0; currentState.currentMenuState = MENU_SPOOL_SELECT; inputManager.resetEncoder(editingSpoolIndex); }
                else { currentState.currentMenuState = MENU_SPOOL_SET_COLOR; inputManager.resetEncoder(currentState.spools[editingSpoolIndex].colorIndex); }
                break;
            case MENU_SPOOL_SET_COLOR:
                currentState.spools[editingSpoolIndex].colorIndex = currentState.menu_selection;
                currentState.currentMenuState = MENU_SPOOL_SELECT;
                inputManager.resetEncoder(editingSpoolIndex);
                break;
            
            case MENU_SET_CUSTOM_TEMP:
                profiles[PROFILE_CUSTOM].temp = (float)currentState.menu_selection;
                currentState.currentMenuState = MENU_SET_CUSTOM_HUM;
                inputManager.resetEncoder((int)profiles[PROFILE_CUSTOM].humidity);
                break;
            case MENU_SET_CUSTOM_HUM:
                profiles[PROFILE_CUSTOM].humidity = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_SETTINGS;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
        }
    }
    
    long val = inputManager.getEncoderValue();
    if (currentState.currentMenuState == MENU_MAIN_SELECT) { int opt = 3; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_PROFILE_SELECT) { int opt = 4; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_MODE_SELECT) { int opt = 3; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_SETTINGS) { int opt = 6; currentState.menu_selection = (val % opt + opt) % opt; }
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    else if (currentState.currentMenuState == MENU_ADVANCED_SETTINGS) { int opt = 12; currentState.menu_selection = (val % opt + opt) % opt; } // 12 opcji
    // =================== KONIEC ZMIANY v5.35 ===================
    else if (currentState.currentMenuState == MENU_COMPONENT_TEST) { int opt = 8; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_WIFI_SETTINGS) { int opt = 4; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_SPOOL_SELECT) { int opt = 4; currentState.menu_selection = (val % opt + opt) % opt; }
    else if (currentState.currentMenuState == MENU_SPOOL_SET_TYPE) { currentState.menu_selection = (val % FILAMENT_TYPE_COUNT + FILAMENT_TYPE_COUNT) % FILAMENT_TYPE_COUNT; }
    else if (currentState.currentMenuState == MENU_SPOOL_SET_COLOR) { currentState.menu_selection = (val % FILAMENT_COLORS_FULL_COUNT + FILAMENT_COLORS_FULL_COUNT) % FILAMENT_COLORS_FULL_COUNT; }
    else if (currentState.currentMenuState == MENU_SET_TIME) { int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 144) new_val = 144; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_CUSTOM_TEMP) { int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 90) new_val = 90; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_CUSTOM_HUM) { int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 50) new_val = 50; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_PSU_OVERHEAT_LIMIT) { int new_val = val; if (new_val < 45) new_val = 45; if (new_val > 70) new_val = 70; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_CONTRAST) { int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 255) new_val = 255; currentState.menu_selection = new_val; displayManager.setContrast(new_val); }
    else if (currentState.currentMenuState == MENU_SET_PID_KP || currentState.currentMenuState == MENU_SET_PID_KI || currentState.currentMenuState == MENU_SET_PID_KD) { int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 1000) new_val = 1000; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_BOOST_TIME) { int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 30) new_val = 30; currentState.menu_selection = new_val; }
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    else if (currentState.currentMenuState == MENU_SET_BOOST_THRESHOLD_PERCENT) { int new_val = val; if (new_val < 50) new_val = 50; if (new_val > 95) new_val = 95; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_RAMP_THRESHOLD_PERCENT) { int new_val = val; if (new_val < 50) new_val = 50; if (new_val > 99) new_val = 99; currentState.menu_selection = new_val; }
    // =================== KONIEC ZMIANY v5.35 ===================
    else if (currentState.currentMenuState == MENU_SET_VENT_INTERVAL) { int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 60) new_val = 60; currentState.menu_selection = new_val; }
    else if (currentState.currentMenuState == MENU_SET_VENT_DURATION) { int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 300) new_val = 300; currentState.menu_selection = new_val; }
}

void DryerController::initializeProfiles() {
    profiles[PROFILE_PLA]={"PLA",45.0,15.0}; profiles[PROFILE_PETG]={"PETG",55.0,15.0};
    profiles[PROFILE_ABS]={"ABS",60.0,15.0}; profiles[PROFILE_CUSTOM]={"Wlasny",50.0,20.0};
}

void DryerController::saveSettings() {
    DynamicJsonDocument doc(2048);
    doc["wifiEnabled"] = currentState.isWifiEnabled;
    doc["customTemp"] = profiles[PROFILE_CUSTOM].temp;
    doc["customHum"] = profiles[PROFILE_CUSTOM].humidity;
    doc["soundsEnabled"] = currentState.areSoundsEnabled;
    doc["psuOverheatLimit"] = currentState.psuOverheatLimit;
    doc["glcdContrast"] = currentState.glcdContrast;
    
    doc["pid_kp"] = currentState.pid_kp;
    doc["pid_ki"] = currentState.pid_ki;
    doc["pid_kd"] = currentState.pid_kd;

    doc["boostMaxTime_min"] = currentState.boostMaxTime_min;
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    doc["boostThresholdPercent"] = currentState.boostThresholdPercent;
    doc["rampThresholdPercent"] = currentState.rampThresholdPercent;
    // =================== KONIEC ZMIANY v5.35 ===================

    doc["ventilationInterval_min"] = currentState.ventilationInterval_min;
    doc["ventilationDuration_sec"] = currentState.ventilationDuration_sec;

    JsonArray spoolsArray = doc.createNestedArray("spools");
    for (int i = 0; i < 4; i++) {
        JsonObject spool = spoolsArray.createNestedObject();
        spool["type"] = currentState.spools[i].typeIndex;
        spool["color"] = currentState.spools[i].colorIndex;
    }
    File file = LittleFS.open("/settings.json", "w");
    if(serializeJson(doc, file) == 0) { Serial.println(F("Blad zapisu do pliku.")); }
    file.close();
}

void DryerController::loadSettings() {
    File file = LittleFS.open("/settings.json", "r");
    if (!file) { return; }
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    if (error) { file.close(); return; }

    currentState.isWifiEnabled = doc["wifiEnabled"] | true;
    profiles[PROFILE_CUSTOM].temp = doc["customTemp"] | 50.0;
    profiles[PROFILE_CUSTOM].humidity = doc["customHum"] | 20.0;
    currentState.areSoundsEnabled = doc["soundsEnabled"] | true;
    currentState.psuOverheatLimit = doc["psuOverheatLimit"] | PSU_OVERHEAT_LIMIT_DEFAULT;
    currentState.glcdContrast = doc["glcdContrast"] | GLCD_CONTRAST_DEFAULT;
    
    currentState.pid_kp = doc["pid_kp"] | PID_KP_DEFAULT;
    currentState.pid_ki = doc["pid_ki"] | PID_KI_DEFAULT;
    currentState.pid_kd = doc["pid_kd"] | PID_KD_DEFAULT;

    currentState.boostMaxTime_min = doc["boostMaxTime_min"] | DEFAULT_BOOST_TIME_MIN;
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    currentState.boostThresholdPercent = doc["boostThresholdPercent"] | DEFAULT_BOOST_THRESHOLD_PERCENT;
    currentState.rampThresholdPercent = doc["rampThresholdPercent"] | DEFAULT_RAMP_THRESHOLD_PERCENT;
    // =================== KONIEC ZMIANY v5.35 ===================

    currentState.ventilationInterval_min = doc["ventilationInterval_min"] | DEFAULT_VENT_INTERVAL_MIN;
    currentState.ventilationDuration_sec = doc["ventilationDuration_sec"] | DEFAULT_VENT_DURATION_SEC;

    JsonArray spoolsArray = doc["spools"];
    if (!spoolsArray.isNull()) {
        int i = 0;
        for(JsonVariant v : spoolsArray) {
            if (i < 4) {
                currentState.spools[i].typeIndex = v["type"] | 0;
                currentState.spools[i].colorIndex = v["color"] | 0;
                i++;
            }
        }
    }
    file.close();
}
// =================== KONIEC PLIKU v5.35 ===================