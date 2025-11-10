// =================================================================
// Plik:          WebManager.h
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [CHORE] Aktualizacja nagłówka wersji.
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