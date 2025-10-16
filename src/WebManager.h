// =================================================================
// Plik:          WebManager.h
// Wersja:        5.28
// Data:          16.10.2025
// Opis:
//  - Dodano endpointy i handlery dla strony ustawień.
// =================================================================
#ifndef WEBMANAGER_H
#define WEBMANAGER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "DryerState.h"
#include "config.h"

class WebManager {
public:
    WebManager(DryerState& state, FilamentProfile* profiles);
    void begin();
    void update();
    String getCommand();

private:
    WebServer _server;
    DryerState& _state;
    FilamentProfile* _profiles;
    String _lastCommand;

    void handleRoot();
    void handleStyle();
    void handleData();
    void handleCommand();
    void handleNotFound();
    void handleFileRequest(const String& path, const String& contentType);

    // ================== POCZĄTEK ZMIANY v5.28 ==================
    void handleSettingsPage();
    void handleGetSettings();
    void handleSaveSettings();
    // =================== KONIEC ZMIANY v5.28 ===================
};

#endif