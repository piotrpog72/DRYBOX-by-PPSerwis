// =================================================================
// Plik:          WebManager.h
// Wersja:        5.29
// Data:          17.10.2025
// Opis:
//  - Zaktualizowano interfejs klasy o obsługę etykiet rolek.
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

    void handleSettingsPage();
    void handleGetSettings();
    void handleSaveSettings();
};

#endif