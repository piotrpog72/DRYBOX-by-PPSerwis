// =================================================================
// Plik:          WebManager.cpp
// Wersja:        5.28
// Data:          16.10.2025
// Opis:
//  - Implementacja logiki dla strony ustawień.
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
    
    // ================== POCZĄTEK ZMIANY v5.28 ==================
    _server.on("/settings", HTTP_GET, [this](){ this->handleSettingsPage(); });
    _server.on("/get_settings", HTTP_GET, [this](){ this->handleGetSettings(); });
    _server.on("/save_settings", HTTP_POST, [this](){ this->handleSaveSettings(); });
    // =================== KONIEC ZMIANY v5.28 ===================

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

void WebManager::handleSettingsPage() {
    handleFileRequest("/settings.html", "text/html");
}

void WebManager::handleGetSettings() {
    DynamicJsonDocument doc(1024);
    doc["customTemp"] = _profiles[PROFILE_CUSTOM].temp;
    doc["customHum"] = _profiles[PROFILE_CUSTOM].humidity;
    doc["soundsEnabled"] = _state.areSoundsEnabled;
    doc["glcdContrast"] = _state.glcdContrast;
    doc["boostMaxTime_min"] = _state.boostMaxTime_min;
    doc["boostTempThreshold"] = _state.boostTempThreshold;
    doc["boostPsuTempLimit"] = _state.boostPsuTempLimit;
    doc["rampPowerPercent"] = _state.rampPowerPercent;
    doc["pid_kp"] = _state.pid_kp;
    doc["pid_ki"] = _state.pid_ki;
    doc["pid_kd"] = _state.pid_kd;
    doc["psuFanOnTemp"] = _state.psuFanOnTemp;
    doc["psuFanOffHysteresis"] = _state.psuFanOffHysteresis;
    doc["psuOverheatLimit"] = _state.psuOverheatLimit;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebManager::handleSaveSettings() {
    // Profil Własny
    if (_server.hasArg("customTemp")) _profiles[PROFILE_CUSTOM].temp = _server.arg("customTemp").toFloat();
    if (_server.hasArg("customHum")) _profiles[PROFILE_CUSTOM].humidity = _server.arg("customHum").toFloat();
    
    // Ustawienia ogólne
    if (_server.hasArg("soundsEnabled")) _state.areSoundsEnabled = (_server.arg("soundsEnabled") == "true");
    if (_server.hasArg("glcdContrast")) _state.glcdContrast = _server.arg("glcdContrast").toInt();

    // Sterowanie Grzaniem
    if (_server.hasArg("boostMaxTime_min")) _state.boostMaxTime_min = _server.arg("boostMaxTime_min").toInt();
    if (_server.hasArg("boostTempThreshold")) _state.boostTempThreshold = _server.arg("boostTempThreshold").toFloat();
    if (_server.hasArg("boostPsuTempLimit")) _state.boostPsuTempLimit = _server.arg("boostPsuTempLimit").toFloat();
    if (_server.hasArg("rampPowerPercent")) _state.rampPowerPercent = _server.arg("rampPowerPercent").toInt();

    // Nastawy PID
    if (_server.hasArg("pid_kp")) _state.pid_kp = _server.arg("pid_kp").toFloat();
    if (_server.hasArg("pid_ki")) _state.pid_ki = _server.arg("pid_ki").toFloat();
    if (_server.hasArg("pid_kd")) _state.pid_kd = _server.arg("pid_kd").toFloat();

    // Zasilacz
    if (_server.hasArg("psuFanOnTemp")) _state.psuFanOnTemp = _server.arg("psuFanOnTemp").toFloat();
    if (_server.hasArg("psuFanOffHysteresis")) _state.psuFanOffHysteresis = _server.arg("psuFanOffHysteresis").toFloat();
    if (_server.hasArg("psuOverheatLimit")) _state.psuOverheatLimit = _server.arg("psuOverheatLimit").toFloat();

    _lastCommand = "SAVE_SETTINGS";
    _server.send(200, "text/plain", "OK");
}

void WebManager::handleData() {
    DynamicJsonDocument doc(1024);
    doc["fw_version"] = FW_VERSION;
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
            case PHASE_BOOST: phaseStr = "Boost"; break;
            case PHASE_RAMP: phaseStr = "Ramp"; break;
            case PHASE_PID: phaseStr = "PID"; break;
            default: break;
        }
    }
    doc["heating_phase"] = phaseStr;
    
    doc["icon_heater"] = _state.isHeaterOn;
    doc["icon_cooling"] = (_state.isHeaterFanOn && _state.currentMode == MODE_IDLE);
    doc["icon_fan_chamber"] = _state.isChamberFanOn;
    doc["icon_fan_psu"] = _state.isPsuFanOn;
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