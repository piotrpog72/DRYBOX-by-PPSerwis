// =================================================================
// Plik:         DisplayManager.h
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [CHORE] Przywrócenie nazwy klasy U8G2 do podstawowej formy (R0),
//           aby rotację kontrolować tylko w konstruktorze CPP.
// =================================================================
#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>  
#include <LiquidCrystal_I2C.h>
#include <U8g2lib.h>
#include "config.h"
#include "DryerState.h"
#include "MenuState.h"

class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update(const DryerState& state, const String& profileName);
    void showStartupScreen(const DryerState& state);
    void showWifiConfigScreen();
    void wakeUpInteractive();
    void sleepInteractive();
    void setContrast(uint8_t value);
    
private:
    LiquidCrystal_I2C statusLcd;
    int currentStatusScreen = 0;
    unsigned long lastStatusScreenChange = 0;
    unsigned long lastStatusUpdate = 0;
    
    void updateStatusDisplay(const DryerState& state);
    void drawStatusScreen_Main(const DryerState& state);
    void drawStatusScreen_Chamber(const DryerState& state);
    void drawStatusScreen_PSU(const DryerState& state); 

    // ================== POCZĄTEK ZMIANY v5.39 (KLASA PODSTAWOWA) ==================
    // Używamy podstawowej nazwy klasy U8G2
    U8G2_ST7565_EA_DOGM128_F_4W_SW_SPI interactiveGcd; 
    // =================== KONIEC ZMIANY v5.39 (KLASA PODSTAWOWA) ===================
    
    bool is_sleeping = false;
    
    void updateInteractiveDisplay(const DryerState& state, const String& profileName);
    void drawGenericMenu(const DryerState& state, const char* options[], int optionCount, const char* title);
    void drawSetTimeMenu(const DryerState& state);
    void drawSetCustomValueMenu(const DryerState& state, const char* title);
    void drawSpoolStatusScreen(const DryerState& state);
    void drawWifiStatusScreen(const DryerState& state);
};

#endif