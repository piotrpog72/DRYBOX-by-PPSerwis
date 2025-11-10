// =================================================================
// Plik:         WebManager.cpp
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [TASK] Aktualizacja handleGetSettings i handleSaveSettings dla
//          progów procentowych (zgodnie z settings.html v5.35).
// - [CHORE] Usunięto obsługę boostTempThreshold, boostPsuTempLimit
//           i rampPowerPercent.
// - [CHORE] Aktualizacja FW_VERSION w handleData.
// =================================================================
#include "WebManager.h"
#include <LittleFS.h>

WebManager::WebManager(DryerState& state, FilamentProfile* profiles)
: _server(80), _state(state), _profiles(profiles) {
}

void WebManager::begin() {
    _server.on("/", HTTP_GET, [this](){ this->handleRoot(); });
    _server.on("/style.css", HTTP_GET, [this](){ this->handleStyle(); });
    _server.on("/data", HTTP_GET, [this](){ this->handleData(); });
    _server.on("/command", HTTP_GET, [this](){ this->handleCommand(); });
    
    _server.on("/settings", HTTP_GET, [this](){ this->handleSettingsPage(); });
    _server.on("/get_settings", HTTP_GET, [this](){ this->handleGetSettings(); });
    _server.on("/save_settings", HTTP_POST, [this](){ this->handleSaveSettings(); });

    _server.onNotFound([this](){ this->handleNotFound(); });
    _server.begin();
    Serial.println("WebManager: Serwer HTTP uruchomiony.");
}

void WebManager::update() {
    _server.handleClient();
}

String WebManager::getCommand() {
    if (_lastCommand.length() > 0) {
        String cmd = _lastCommand;
        _lastCommand = "";
        return cmd;
    }
    return "";
}

void WebManager::handleRoot() {
    handleFileRequest("/index.html", "text/html");
}

void WebManager::handleStyle() {
    handleFileRequest("/style.css", "text/css");
}

void WebManager::handleSettingsPage() {
    handleFileRequest("/settings.html", "text/html");
}

void WebManager::handleNotFound() {
    _server.send(404, "text/plain", "Not Found");
}

void WebManager::handleFileRequest(const String& path, const String& contentType) {
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        _server.streamFile(file, contentType);
        file.close();
    } else {
        handleNotFound();
    }
}

void WebManager::handleGetSettings() {
    DynamicJsonDocument doc(1024);
    doc["customTemp"] = _profiles[PROFILE_CUSTOM].temp;
    doc["customHum"] = _profiles[PROFILE_CUSTOM].humidity;
    doc["soundsEnabled"] = _state.areSoundsEnabled;
    doc["glcdContrast"] = _state.glcdContrast;
    doc["boostMaxTime_min"] = _state.boostMaxTime_min;
    
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    doc["boostThresholdPercent"] = _state.boostThresholdPercent;
    doc["rampThresholdPercent"] = _state.rampThresholdPercent;
    // Usunięto boostTempThreshold, boostPsuTempLimit, rampPowerPercent
    // =================== KONIEC ZMIANY v5.35 ===================
    
    doc["pid_kp"] = _state.pid_kp;
    doc["pid_ki"] = _state.pid_ki;
    doc["pid_kd"] = _state.pid_kd;
    doc["psuOverheatLimit"] = _state.psuOverheatLimit;

    doc["ventilationInterval_min"] = _state.ventilationInterval_min;
    doc["ventilationDuration_sec"] = _state.ventilationDuration_sec;

    JsonArray spools = doc.createNestedArray("spools");
    for (int i=0; i<4; i++) {
        JsonObject spool = spools.createNestedObject();
        spool["type"] = _state.spools[i].typeIndex;
        spool["color"] = _state.spools[i].colorIndex;
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebManager::handleSaveSettings() {
    if (_server.hasArg("customTemp")) _profiles[PROFILE_CUSTOM].temp = _server.arg("customTemp").toFloat();
    if (_server.hasArg("customHum")) _profiles[PROFILE_CUSTOM].humidity = _server.arg("customHum").toFloat();
    if (_server.hasArg("soundsEnabled")) _state.areSoundsEnabled = (_server.arg("soundsEnabled") == "true");
    if (_server.hasArg("glcdContrast")) _state.glcdContrast = _server.arg("glcdContrast").toInt();
    if (_server.hasArg("boostMaxTime_min")) _state.boostMaxTime_min = _server.arg("boostMaxTime_min").toInt();

    // ================== POCZĄTEK ZMIANY v5.35 ==================
    if (_server.hasArg("boostThresholdPercent")) _state.boostThresholdPercent = _server.arg("boostThresholdPercent").toInt();
    if (_server.hasArg("rampThresholdPercent")) _state.rampThresholdPercent = _server.arg("rampThresholdPercent").toInt();
    // Usunięto boostTempThreshold, boostPsuTempLimit, rampPowerPercent
    // =================== KONIEC ZMIANY v5.35 ===================

    if (_server.hasArg("pid_kp")) _state.pid_kp = _server.arg("pid_kp").toFloat();
    if (_server.hasArg("pid_ki")) _state.pid_ki = _server.arg("pid_ki").toFloat();
    if (_server.hasArg("pid_kd")) _state.pid_kd = _server.arg("pid_kd").toFloat();
    if (_server.hasArg("psuOverheatLimit")) _state.psuOverheatLimit = _server.arg("psuOverheatLimit").toFloat();

    if (_server.hasArg("ventilationInterval_min")) _state.ventilationInterval_min = _server.arg("ventilationInterval_min").toInt();
    if (_server.hasArg("ventilationDuration_sec")) _state.ventilationDuration_sec = _server.arg("ventilationDuration_sec").toInt();

    for (int i = 0; i < 4; i++) {
        String type_arg = "spool_type_" + String(i);
        String color_arg = "spool_color_" + String(i);
        if (_server.hasArg(type_arg)) _state.spools[i].typeIndex = _server.arg(type_arg).toInt();
        if (_server.hasArg(color_arg)) _state.spools[i].colorIndex = _server.arg(color_arg).toInt();
    }

    _lastCommand = "SAVE_SETTINGS";
    _server.send(200, "text/plain", "OK");
}

void WebManager::handleData() {
    DynamicJsonDocument doc(1024);
    // ================== POCZĄTEK ZMIANY v5.35 ==================
    doc["fw_version"] = FW_VERSION; // Używamy teraz zaktualizowanej wersji
    // =================== KONIEC ZMIANY v5.35 ===================
    doc["temp"] = _state.avgChamberTemp;
    doc["hum"] = _state.dhtHum;
    doc["target_temp"] = _state.targetTemp;
    doc["target_hum"] = _state.targetHumidity;
    String modeStr;
    switch (_state.currentMode) {
        case MODE_IDLE: modeStr = "Czuwanie"; break;
        case MODE_TIMED: modeStr = "Czasowy"; break;
        case MODE_HUMIDITY_TARGET: modeStr = "Wilgotnosc"; break;
        case MODE_CONTINUOUS: modeStr = "Ciagly"; break;
    }
    doc["mode"] = modeStr;

    String phaseStr = "";
    if (_state.isHeaterOn) {
        switch (_state.currentPhase) {
            case PHASE_BOOST: phaseStr = "Boost (69W)"; break;
            case PHASE_RAMP:  phaseStr = "Ramp (46W)"; break;
            case PHASE_PID:   phaseStr = "PID (23W)"; break;
            default: break;
        }
    }
    doc["heating_phase"] = phaseStr;
    
    doc["icon_heater_main"] = _state.isHeaterMainOn;
    doc["icon_heater_aux1"] = _state.isHeaterAux1On;
    doc["icon_heater_aux2"] = _state.isHeaterAux2On;
    doc["icon_fan_chamber"] = _state.isChamberFanOn;
    doc["icon_fan_vent"] = _state.isVentilationFanOn; 
    
    doc["ds0"] = _state.ds18b20_temps[0];
    doc["ds1"] = _state.ds18b20_temps[1];
    doc["ds2"] = _state.ds18b20_temps[2];
    doc["ds3"] = _state.ds18b20_temps[3];
    doc["ds4"] = _state.ds18b20_temps[4];
    doc["pid_power"] = _state.pidOutput;
    
    JsonArray spools = doc.createNestedArray("spools");
    for (int i=0; i<4; i++) {
        JsonObject spool = spools.createNestedObject();
        spool["type"] = _state.spools[i].typeIndex;
        spool["color"] = _state.spools[i].colorIndex;
    }
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebManager::handleCommand() {
    if (_server.hasArg("cmd")) {
        _lastCommand = _server.arg("cmd");
        Serial.printf("WebManager: Odebrano komendę: %s\n", _lastCommand.c_str());
        _server.send(200, "text/plain", "OK");
    } else {
        _server.send(400, "text/plain", "Bad Request");
    }
}