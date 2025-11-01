// =================================================================
// Plik:          DryerController.cpp
// Wersja:        5.32
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REFACTOR] Dostosowano logikę grzania do sterowania
//    trzema grzałkami (poziomy mocy 0-3).
//  - [REFACTOR] Zmodyfikowano logikę PID do sterowania ON/OFF
//    jedną grzałką (poziom 1).
//  - Zaktualizowano test podzespołów.
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
  webManager(currentState, profiles),
  menuManager(currentState, inputManager, displayManager)
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
    menuManager.begin();

    displayManager.setContrast(currentState.glcdContrast);
    displayManager.showStartupScreen(currentState); delay(STARTUP_SCREEN_DELAY);

    if (currentState.isWifiEnabled) {
        WiFi.mode(WIFI_STA);
        WiFiManager wm;
        if (wm.autoConnect("Drybox_Setup")) {
            Serial.println("Polaczono z siecia przez WiFiManager.");
            webManager.begin();
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
    // Użyj wartości Kp, Ki, Kd z ustawień (lub domyślnych)
    // Pamiętaj o dostrojeniu ich do sterowania ON/OFF!
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
    menuManager.navigateToScreenMain();
}

void DryerController::update() {
    if(currentState.isWifiEnabled) {
        webManager.update();
        handleWifi();

        String webCmd = webManager.getCommand();
        if (webCmd.length() > 0) {
            processWebCommand(webCmd);
        }
    }

    unsigned long currentTime = millis();
    inputManager.update();
    menuManager.handleInput();

    MenuAction menuAction = menuManager.getAction();
    if (menuAction != ACTION_NONE) {
        processMenuAction(menuAction, menuManager.getActionData());
        menuManager.resetAction();
    }

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
            if (currentState.areSoundsEnabled) actuatorManager.playAlarmSound(true);
        } else if (!alarmCondition && currentState.isInAlarmState) {
            currentState.isInAlarmState = false;
            actuatorManager.playAlarmSound(false);
        }

        int desiredPowerLevel = 0; // Domyślnie wyłączone

        if (currentState.currentMode != MODE_IDLE && !currentState.isInAlarmState) {
            switch (currentState.currentPhase) {
                case PHASE_BOOST: {
                    desiredPowerLevel = 3; // Max moc (69W)

                    bool boostTimeElapsed = (millis() - currentState.dryingStartTime) > (currentState.boostMaxTime_min * 60000UL);
                    bool tempThresholdReached = currentState.avgChamberTemp >= currentState.boostTempThreshold;
                    bool psuTempLimitReached = currentState.ds18b20_temps[3] >= currentState.boostPsuTempLimit;

                    // Przejście do PID z poziomem 1 (23W)
                    if (boostTimeElapsed || tempThresholdReached || psuTempLimitReached) {
                        pid.SetMode(AUTOMATIC);
                        currentState.currentPhase = PHASE_PID;
                        Serial.println("HEATING: Boost -> PID (Level 1)");
                    }
                    break;
                }
                // Na razie brak fazy Ramp
                case PHASE_PID: {
                    pidInput = currentState.avgChamberTemp;
                    pidSetpoint = currentState.targetTemp;
                    if(pid.Compute()){ // Compute zwraca true jeśli obliczenia zostały wykonane
                       desiredPowerLevel = (int)pidOutput; // pidOutput to 0 lub 1
                    } else {
                        // Jeśli Compute() zwróci false (np. minął za krótki czas),
                        // zachowaj poprzedni stan (zapobiega migotaniu)
                        desiredPowerLevel = (int)currentState.pidOutput;
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
        currentState.pidOutput = desiredPowerLevel; // Zapisz wybrany poziom mocy

        currentState.isHeaterOn = (currentState.currentPhase != PHASE_OFF && !currentState.isInAlarmState);

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
            if (currentState.areSoundsEnabled) actuatorManager.playCompletionSound();
        }
    } // Koniec if (!currentState.isInTestMode)

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
        int firstColon = command.indexOf(':');
        int secondColon = command.indexOf(':', firstColon + 1);
        int profileIndex = command.substring(firstColon + 1, secondColon).toInt();
        int modeIndex = command.substring(secondColon + 1).toInt();

        currentState.currentProfileType = (ProfileType)profileIndex;
        currentState.targetTemp = profiles[profileIndex].temp;
        currentState.targetHumidity = profiles[profileIndex].humidity;

        switch(modeIndex) {
            case 0: currentState.currentMode = MODE_TIMED; break;
            case 1: currentState.currentMode = MODE_HUMIDITY_TARGET; break;
            case 2: currentState.currentMode = MODE_CONTINUOUS; break;
        }
        startDryingProcess();
    } else if (command == "SAVE_SETTINGS") {
        saveSettings();
        // Zastosuj nowe nastawy PID i kontrast natychmiast
        pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd); // Załaduj nowe nastawy
        displayManager.setContrast(currentState.glcdContrast);
        Serial.println("DryerController: Ustawienia zapisane z Web UI.");
    }
}

void DryerController::processMenuAction(MenuAction action, int data) {
    switch (action) {
        case ACTION_START_DRYING_TIMED:
            currentState.dryingDurationMinutes = data;
            currentState.currentMode = MODE_TIMED;
            currentState.targetTemp = profiles[currentState.currentProfileType].temp; // Upewnij się, że cel jest ustawiony
            currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;
            startDryingProcess();
            break;
        case ACTION_START_DRYING_HUMIDITY:
            currentState.currentMode = MODE_HUMIDITY_TARGET;
            currentState.targetTemp = profiles[currentState.currentProfileType].temp;
            currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;
            startDryingProcess();
            break;
        case ACTION_START_DRYING_CONTINUOUS:
            currentState.currentMode = MODE_CONTINUOUS;
            currentState.targetTemp = profiles[currentState.currentProfileType].temp;
            currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;
            startDryingProcess();
            break;
        case ACTION_STOP_DRYING:
            currentState.currentMode = MODE_IDLE;
            break;
        case ACTION_SAVE_SETTINGS:
            profiles[PROFILE_CUSTOM].temp = currentState.targetTemp; // Zapisz temp/hum z MENU_SET_CUSTOM_*
            profiles[PROFILE_CUSTOM].humidity = currentState.targetHumidity;
            saveSettings();
            break;
        case ACTION_TOGGLE_SOUNDS:
            currentState.areSoundsEnabled = !currentState.areSoundsEnabled;
            saveSettings();
            break;
        case ACTION_START_WIFI_CONFIG: startWifiConfig(); break;
        case ACTION_APPLY_CONTRAST: displayManager.setContrast(currentState.glcdContrast); break; // Zastosuj, zapis przy wyjściu z Advanced
         case ACTION_APPLY_PID_TUNINGS:
            pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd); // Zastosuj, zapis przy wyjściu z Advanced
            break;
        case ACTION_ENTER_TEST_MODE:
            currentState.isInTestMode = true;
            currentState.test_heater = false; // Reset flag testowych
            currentState.test_chamber_fan = false;
            currentState.test_buzzer = false;
            currentState.test_heater_led = false;
            actuatorManager.resetAllForced();
            break;
        case ACTION_EXIT_TEST_MODE_AND_RESET_ACTUATORS:
            currentState.isInTestMode = false;
            actuatorManager.resetAllForced();
            break;
        // UWAGA: Logika testów wymaga aktualizacji w MenuManager.cpp, aby zwracać osobne akcje dla każdej grzałki
        // Na razie zostawiamy uproszczoną wersję
        case ACTION_TOGGLE_TEST_HEATER: /* TODO w MenuManager */ break;
        case ACTION_TOGGLE_TEST_CHAMBER_FAN: currentState.test_chamber_fan = !currentState.test_chamber_fan; actuatorManager.forceChamberFan(currentState.test_chamber_fan); break;
        case ACTION_TOGGLE_TEST_BUZZER: currentState.test_buzzer = !currentState.test_buzzer; actuatorManager.forceBuzzer(currentState.test_buzzer); break;
        case ACTION_TOGGLE_TEST_HEATER_LED: currentState.test_heater_led = !currentState.test_heater_led; actuatorManager.forceHeaterLed(currentState.test_heater_led); break;
        case ACTION_NONE: default: break;
    }
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
    doc["boostTempThreshold"] = currentState.boostTempThreshold;
    doc["boostPsuTempLimit"] = currentState.boostPsuTempLimit;
    doc["rampPowerPercent"] = currentState.rampPowerPercent;

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
    currentState.boostTempThreshold = doc["boostTempThreshold"] | DEFAULT_BOOST_TEMP_THRESHOLD;
    currentState.boostPsuTempLimit = doc["boostPsuTempLimit"] | DEFAULT_BOOST_PSU_TEMP_LIMIT;
    currentState.rampPowerPercent = doc["rampPowerPercent"] | DEFAULT_RAMP_POWER_PERCENT;

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

    // Ustawienia PID są stosowane w begin() po załadowaniu
}