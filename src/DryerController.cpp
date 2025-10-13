// =================================================================
// Plik:          DryerController.cpp
// Wersja:        5.10 (Skonsolidowana, Poprawiona)
// Data:          13.10.2025
// Opis Zmian:
//  - Przywrócenie pełnej, kompletnej wersji z działającą architekturą
//    serwera WWW i pełną implementacją logiki PID/Boost/Rampa.
// =================================================================
#include "DryerController.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ArduinoOTA.h>
#include "lists.h"

DryerController::DryerController() :
  pid(&pidInput, &pidOutput, &pidSetpoint, currentState.pid_kp, currentState.pid_ki, currentState.pid_kd, DIRECT)
{
    server = new AsyncWebServer(80);
    ws = new AsyncWebSocket("/ws");
}

void DryerController::begin() {
    Serial.begin(115200); Serial.println("DryerController: Inicjalizacja " FW_VERSION "...");
    if (!LittleFS.begin(true)) { Serial.println("Błąd inicjalizacji LittleFS."); }
    initializeProfiles();
    loadSettings();
    
    sensorManager.begin(); displayManager.begin();
    inputManager.begin(); actuatorManager.begin();
    displayManager.setContrast(currentState.glcdContrast);
    displayManager.showStartupScreen(currentState); delay(STARTUP_SCREEN_DELAY);
    
    if (currentState.isWifiEnabled) {
        WiFi.mode(WIFI_STA);
        
        AsyncWiFiManager wifiManager(server, nullptr);
        if (wifiManager.autoConnect("Drybox_Setup")) {
            Serial.println("Polaczono z siecia przez WiFiManager.");
            setupWebServer();
        } else {
            Serial.println("Nie udalo sie polaczyc z WiFi.");
        }
    }

    currentState.sensorsOk = sensorManager.areSensorsFound();
    currentState.lastUserInputTime = millis();
    lastHeaterUpdateTime = millis();
    currentState.targetTemp = profiles[currentState.currentProfileType].temp;
    currentState.targetHumidity = profiles[currentState.currentProfileType].humidity;

    pid.SetSampleTime(1000);
    pid.SetOutputLimits(0, 255);
    pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
    pid.SetMode(AUTOMATIC);
}

void DryerController::startWifiConfig() {
    displayManager.showWifiConfigScreen();
    AsyncWiFiManager wifiManager(server, nullptr);
    wifiManager.startConfigPortal("Drybox_Setup");
}

void DryerController::handleWifi() {
    if (!currentState.isWifiEnabled) {
        if (currentState.isWifiConnected) {
            WiFi.disconnect(true);
            ArduinoOTA.end();
            currentState.isWifiConnected = false;
        }
        return;
    }

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

void DryerController::update() {
    handleWifi();
    unsigned long currentTime = millis();
    inputManager.update(); 
    processUserInput();
    
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

     if (currentState.currentMode != MODE_IDLE && !currentState.isInAlarmState) {
    pidInput = currentState.avgChamberTemp;
    pidSetpoint = currentState.targetTemp;

    // Logika 3-stopniowego grzania
    bool boostTimeExceeded = (millis() - currentState.dryingStartTime) > (currentState.boostMaxTime_min * 60000UL);
    bool boostTempReached = (pidInput >= currentState.boostTempThreshold);
    bool psuTooHot = (currentState.ds18b20_temps[3] >= currentState.boostPsuTempLimit);
    
    if (currentState.isBoostActive && (boostTimeExceeded || boostTempReached || psuTooHot)) {
        currentState.isBoostActive = false;
    }

    if (currentState.isBoostActive) {
        currentState.pidOutput = 255;
    } else if (pidInput < pidSetpoint) {
        currentState.pidOutput = (currentState.rampPowerPercent * 255) / 100;
    } else {
        pid.Compute();
        currentState.pidOutput = pidOutput;
    }
    
    // KLUCZOWA POPRAWKA: Ustawiamy logiczny stan grzania na podstawie tego, czy moc jest > 0
    currentState.isHeaterOn = (currentState.pidOutput > 0);

} else {
    currentState.pidOutput = 0;
    currentState.isHeaterOn = false; // Logiczny stan grzania = wyłączony
    currentState.isBoostActive = false;
}

actuatorManager.update(currentState); // ActuatorManager teraz tylko wykonuje polecenia

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

    if (ws) {
        ws->cleanupClients();
        static unsigned long lastWsTime = 0;
        if (ws->count() > 0 && millis() - lastWsTime > 2000) {
            lastWsTime = millis();
            DynamicJsonDocument doc(1024);
            doc["temp"] = currentState.avgChamberTemp;
            doc["hum"] = currentState.dhtHum;
            doc["target_temp"] = currentState.targetTemp;
            doc["target_hum"] = currentState.targetHumidity;
            
            String modeStr;
            switch (currentState.currentMode) {
                case MODE_IDLE: modeStr = "Czuwanie"; break;
                case MODE_TIMED: modeStr = "Czasowy"; break;
                case MODE_HUMIDITY_TARGET: modeStr = "Wilgotnosc"; break;
                case MODE_CONTINUOUS: modeStr = "Ciagly"; break;
            }
            doc["mode"] = modeStr;

            doc["icon_heater"] = currentState.isHeaterOn;
            doc["icon_fan_chamber"] = currentState.isChamberFanOn;
            doc["icon_fan_psu"] = currentState.isPsuFanOn;
            doc["icon_cooling"] = currentState.isHeaterFanOn && !currentState.isHeaterOn;
            
            doc["ds0"] = currentState.ds18b20_temps[0];
            doc["ds1"] = currentState.ds18b20_temps[1];
            doc["ds2"] = currentState.ds18b20_temps[2];
            doc["ds3"] = currentState.ds18b20_temps[3];
            doc["ds4"] = currentState.ds18b20_temps[4];
            
            doc["pid_power"] = currentState.pidOutput;

            JsonArray spools = doc.createNestedArray("spools");
            for (int i=0; i<4; i++) {
                JsonObject spool = spools.createNestedObject();
                spool["type"] = currentState.spools[i].typeIndex;
                spool["color"] = currentState.spools[i].colorIndex;
            }

            String json;
            serializeJson(doc, json);
            ws->textAll(json);
        }
    }
}

void DryerController::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            String command = (char*)data;
            Serial.printf("Odebrano komendę: %s\n", command.c_str());

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
                    case 0: // Czasowy
                        currentState.currentMode = MODE_TIMED;
                        break;
                    case 1: // Wilgotność
                        currentState.currentMode = MODE_HUMIDITY_TARGET;
                        break;
                    case 2: // Ciągły
                        currentState.currentMode = MODE_CONTINUOUS;
                        break;
                }
                currentState.dryingStartTime = millis();
                currentState.startingHumidity = currentState.dhtHum;
                currentState.heaterTotalOnTime_ms = 0;
                lastHeaterUpdateTime = millis();
                currentState.isBoostActive = true;
            }
        }
    }
}

void DryerController::setupWebServer() {
    ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        this->onWsEvent(server, client, type, arg, data, len);
    });
    server->addHandler(ws);

    server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server->begin();
    Serial.println("Serwer HTTP uruchomiony");
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
            case MENU_PROFILE_SELECT:
                currentState.currentMenuState = MENU_MAIN_SELECT;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
            case MENU_MODE_SELECT:
                currentState.currentMenuState = MENU_PROFILE_SELECT;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
            case MENU_SET_TIME:
            case MENU_SET_CUSTOM_TEMP:
            case MENU_SET_CUSTOM_HUM:
                 currentState.currentMenuState = MENU_SETTINGS;
                 inputManager.resetEncoder(); currentState.menu_selection = 0;
                 break;
            case MENU_SPOOL_SELECT:
                saveSettings();
                currentState.currentMenuState = MENU_SETTINGS;
                inputManager.resetEncoder(4);
                break;
            case MENU_SPOOL_SET_TYPE:
                currentState.currentMenuState = MENU_SPOOL_SELECT;
                inputManager.resetEncoder(editingSpoolIndex);
                break;
            case MENU_SPOOL_SET_COLOR:
                currentState.currentMenuState = MENU_SPOOL_SET_TYPE;
                inputManager.resetEncoder(currentState.spools[editingSpoolIndex].typeIndex);
                break;
            case MENU_SETTINGS:
                currentState.currentMenuState = MENU_MAIN_SELECT;
                inputManager.resetEncoder(); currentState.menu_selection = 0;
                break;
            case MENU_ADVANCED_SETTINGS:
                 currentState.currentMenuState = MENU_SETTINGS;
                 inputManager.resetEncoder(2);
                 break;
            case MENU_COMPONENT_TEST:
                 currentState.isInTestMode = false;
                 actuatorManager.resetAllForced();
                 currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                 inputManager.resetEncoder(11);
                 break;
            case MENU_SET_CONTRAST:
            case MENU_SET_PID_KP:
            case MENU_SET_PID_KI:
            case MENU_SET_PID_KD:
            case MENU_SET_BOOST_TIME:
            case MENU_SET_BOOST_TEMP:
            case MENU_SET_BOOST_PSU_TEMP:
            case MENU_SET_RAMP_POWER:
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(0);
                break;
            case MENU_WIFI_SETTINGS:
                 currentState.currentMenuState = MENU_SETTINGS;
                 inputManager.resetEncoder(3);
                 break;
            case SCREEN_WIFI_STATUS:
                currentState.currentMenuState = MENU_WIFI_SETTINGS;
                inputManager.resetEncoder(2);
                break;
        }
    }
    if (short_pressed) {
        switch (currentState.currentMenuState) {
            case SCREEN_MAIN:
                if (currentState.isInAlarmState) break;
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
                    case 1:
                        currentState.currentMode = MODE_HUMIDITY_TARGET;
                        break;
                    case 2:
                        currentState.currentMode = MODE_CONTINUOUS;
                        break;
                }
                if (currentState.menu_selection != 0) {
                    currentState.dryingStartTime = millis();
                    currentState.startingHumidity = currentState.dhtHum;
                    currentState.heaterTotalOnTime_ms = 0;
                    lastHeaterUpdateTime = millis();
                    currentState.isBoostActive = true;
                    currentState.currentMenuState = SCREEN_MAIN;
                }
                break;
            case MENU_SET_TIME:
                currentState.dryingDurationMinutes = currentState.menu_selection * 10;
                if (currentState.dryingDurationMinutes > 0) {
                    currentState.currentMode = MODE_TIMED;
                    currentState.dryingStartTime = millis();
                    currentState.isBoostActive = true;
                    currentState.currentMenuState = SCREEN_MAIN;
                }
                break;
            case MENU_SETTINGS:
                if (currentState.menu_selection == 0) {
                    currentState.currentMenuState = MENU_SET_CUSTOM_TEMP;
                    inputManager.resetEncoder((int)profiles[PROFILE_CUSTOM].temp);
                } else if (currentState.menu_selection == 1) {
                    currentState.areSoundsEnabled = !currentState.areSoundsEnabled;
                    saveSettings();
                } else if (currentState.menu_selection == 2) {
                    currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 3) {
                    currentState.currentMenuState = MENU_WIFI_SETTINGS;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 4) {
                    currentState.currentMenuState = MENU_SPOOL_SELECT;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                } else if (currentState.menu_selection == 5) {
                    currentState.currentMenuState = MENU_MAIN_SELECT;
                    inputManager.resetEncoder(); currentState.menu_selection = 0;
                }
                break;
            case MENU_ADVANCED_SETTINGS:
                 if (currentState.menu_selection == 0) {
                    currentState.currentMenuState = MENU_SET_PSU_FAN_ON_TEMP;
                    inputManager.resetEncoder((int)currentState.psuFanOnTemp);
                } else if (currentState.menu_selection == 1) {
                    currentState.currentMenuState = MENU_SET_PSU_FAN_HYSTERESIS;
                    inputManager.resetEncoder((int)currentState.psuFanOffHysteresis);
                } else if (currentState.menu_selection == 2) {
                    currentState.currentMenuState = MENU_SET_PSU_OVERHEAT_LIMIT;
                    inputManager.resetEncoder((int)currentState.psuOverheatLimit);
                } else if (currentState.menu_selection == 3) {
                    currentState.currentMenuState = MENU_SET_CONTRAST;
                    inputManager.resetEncoder(currentState.glcdContrast);
                } else if (currentState.menu_selection == 4) {
                    currentState.currentMenuState = MENU_SET_PID_KP;
                    inputManager.resetEncoder((int)(currentState.pid_kp * 10));
                } else if (currentState.menu_selection == 5) {
                    currentState.currentMenuState = MENU_SET_PID_KI;
                    inputManager.resetEncoder((int)(currentState.pid_ki * 10));
                } else if (currentState.menu_selection == 6) {
                    currentState.currentMenuState = MENU_SET_PID_KD;
                    inputManager.resetEncoder((int)(currentState.pid_kd * 10));
                } else if (currentState.menu_selection == 7) {
                    currentState.currentMenuState = MENU_SET_BOOST_TIME;
                    inputManager.resetEncoder(currentState.boostMaxTime_min);
                } else if (currentState.menu_selection == 8) {
                    currentState.currentMenuState = MENU_SET_BOOST_TEMP;
                    inputManager.resetEncoder((int)currentState.boostTempThreshold);
                } else if (currentState.menu_selection == 9) {
                    currentState.currentMenuState = MENU_SET_BOOST_PSU_TEMP;
                    inputManager.resetEncoder((int)currentState.boostPsuTempLimit);
                } else if (currentState.menu_selection == 10) {
                    currentState.currentMenuState = MENU_SET_RAMP_POWER;
                    inputManager.resetEncoder(currentState.rampPowerPercent);
                } else if (currentState.menu_selection == 11) {
                    currentState.isInTestMode = true;
                    currentState.currentMenuState = MENU_COMPONENT_TEST;
                    inputManager.resetEncoder(0);
                } else if (currentState.menu_selection == 12) {
                    currentState.currentMenuState = MENU_SETTINGS;
                    inputManager.resetEncoder(2);
                }
                break;
            case MENU_COMPONENT_TEST:
                switch(currentState.menu_selection) {
                    case 0: currentState.test_heater = !currentState.test_heater; actuatorManager.forceHeater(currentState.test_heater); break;
                    case 1: currentState.test_heater_fan = !currentState.test_heater_fan; actuatorManager.forceHeaterFan(currentState.test_heater_fan); break;
                    case 2: currentState.test_chamber_fan = !currentState.test_chamber_fan; actuatorManager.forceChamberFan(currentState.test_chamber_fan); break;
                    case 3: currentState.test_psu_fan = !currentState.test_psu_fan; actuatorManager.forcePsuFan(currentState.test_psu_fan); break;
                    case 4: currentState.test_buzzer = !currentState.test_buzzer; actuatorManager.forceBuzzer(currentState.test_buzzer); break;
                    case 5: currentState.test_heater_led = !currentState.test_heater_led; actuatorManager.forceHeaterLed(currentState.test_heater_led); break;
                    case 6: // Powrót
                        currentState.isInTestMode = false;
                        actuatorManager.resetAllForced();
                        currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                        inputManager.resetEncoder(11);
                        break;
                }
                break;
            case MENU_SET_CONTRAST:
                currentState.glcdContrast = currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(3);
                break;
            case MENU_SET_PID_KP:
                currentState.pid_kp = (double)currentState.menu_selection / 10.0;
                pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(4);
                break;
             case MENU_SET_PID_KI:
                currentState.pid_ki = (double)currentState.menu_selection / 10.0;
                pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(5);
                break;
             case MENU_SET_PID_KD:
                currentState.pid_kd = (double)currentState.menu_selection / 10.0;
                pid.SetTunings(currentState.pid_kp, currentState.pid_ki, currentState.pid_kd);
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(6);
                break;
            case MENU_SET_PSU_FAN_ON_TEMP:
                currentState.psuFanOnTemp = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(0);
                break;
            case MENU_SET_PSU_FAN_HYSTERESIS:
                currentState.psuFanOffHysteresis = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(1);
                break;
            case MENU_SET_PSU_OVERHEAT_LIMIT:
                currentState.psuOverheatLimit = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(2);
                break;
            case MENU_WIFI_SETTINGS:
                if (currentState.menu_selection == 0) {
                    currentState.isWifiEnabled = !currentState.isWifiEnabled;
                    saveSettings();
                    if (!currentState.isWifiEnabled) { WiFi.disconnect(true); }
                } else if (currentState.menu_selection == 1) {
                    startWifiConfig();
                    currentState.currentMenuState = MENU_SETTINGS; 
                } else if (currentState.menu_selection == 2) {
                    currentState.currentMenuState = SCREEN_WIFI_STATUS;
                } else if (currentState.menu_selection == 3) {
                    currentState.currentMenuState = MENU_SETTINGS;
                    inputManager.resetEncoder(3);
                }
                break;
            case MENU_SPOOL_SELECT:
                editingSpoolIndex = currentState.menu_selection;
                currentState.currentMenuState = MENU_SPOOL_SET_TYPE;
                inputManager.resetEncoder(currentState.spools[editingSpoolIndex].typeIndex);
                break;
            case MENU_SPOOL_SET_TYPE:
                currentState.spools[editingSpoolIndex].typeIndex = currentState.menu_selection;
                currentState.currentMenuState = MENU_SPOOL_SET_COLOR;
                inputManager.resetEncoder(currentState.spools[editingSpoolIndex].colorIndex);
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
            case MENU_SET_BOOST_TIME:
                currentState.boostMaxTime_min = currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(7);
                break;
            case MENU_SET_BOOST_TEMP:
                currentState.boostTempThreshold = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(8);
                break;
            case MENU_SET_BOOST_PSU_TEMP:
                currentState.boostPsuTempLimit = (float)currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(9);
                break;
            case MENU_SET_RAMP_POWER:
                currentState.rampPowerPercent = currentState.menu_selection;
                saveSettings();
                currentState.currentMenuState = MENU_ADVANCED_SETTINGS;
                inputManager.resetEncoder(10);
                break;
        }
    }
    
    long val = inputManager.getEncoderValue();
    if (currentState.currentMenuState == MENU_MAIN_SELECT) {
        int opt = 3; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_PROFILE_SELECT) {
        int opt = 4; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_MODE_SELECT) {
        int opt = 3; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_SETTINGS) {
        int opt = 6; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_ADVANCED_SETTINGS) {
        int opt = 13; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_COMPONENT_TEST) {
        int opt = 7; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_WIFI_SETTINGS) {
        int opt = 4; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_SPOOL_SELECT) {
        int opt = 4; currentState.menu_selection = (val % opt + opt) % opt;
    } else if (currentState.currentMenuState == MENU_SPOOL_SET_TYPE) {
        currentState.menu_selection = (val % FILAMENT_TYPE_COUNT + FILAMENT_TYPE_COUNT) % FILAMENT_TYPE_COUNT;
    } else if (currentState.currentMenuState == MENU_SPOOL_SET_COLOR) {
        currentState.menu_selection = (val % FILAMENT_COLORS_FULL_COUNT + FILAMENT_COLORS_FULL_COUNT) % FILAMENT_COLORS_FULL_COUNT;
    } else if (currentState.currentMenuState == MENU_SET_TIME) {
        int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 144) new_val = 144; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_CUSTOM_TEMP) {
        int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 90) new_val = 90; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_CUSTOM_HUM) {
        int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 50) new_val = 50; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_PSU_FAN_ON_TEMP) {
        int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 60) new_val = 60; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_PSU_FAN_HYSTERESIS) {
        int new_val = val; if (new_val < 2) new_val = 2; if (new_val > 15) new_val = 15; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_PSU_OVERHEAT_LIMIT) {
        int new_val = val; if (new_val < 45) new_val = 45; if (new_val > 70) new_val = 70; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_CONTRAST) {
        int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 255) new_val = 255; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_PID_KP || currentState.currentMenuState == MENU_SET_PID_KI || currentState.currentMenuState == MENU_SET_PID_KD) {
        int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 1000) new_val = 1000;
        currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_BOOST_TIME) {
        int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 30) new_val = 30; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_BOOST_TEMP || currentState.currentMenuState == MENU_SET_BOOST_PSU_TEMP) {
        int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 60) new_val = 60; currentState.menu_selection = new_val;
    } else if (currentState.currentMenuState == MENU_SET_RAMP_POWER) {
        int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 100) new_val = 100; currentState.menu_selection = new_val;
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
    doc["psuFanOnTemp"] = currentState.psuFanOnTemp;
    doc["psuFanOffHysteresis"] = currentState.psuFanOffHysteresis;
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
    
    currentState.psuFanOnTemp = doc["psuFanOnTemp"] | PSU_FAN_ON_TEMP_DEFAULT;
    currentState.psuFanOffHysteresis = doc["psuFanOffHysteresis"] | PSU_FAN_OFF_HYSTERESIS_DEFAULT;
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
}