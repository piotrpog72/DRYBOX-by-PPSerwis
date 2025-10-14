// =================================================================
// Plik:          DisplayManager.h
// Wersja:        5.20
// Opis Zmian:
//  - Dodano nowe opcje do menu "Ustawienia Zaawansowane".
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
    void setContrast(uint8_t value); // Nowa funkcja

private:
    LiquidCrystal_I2C statusLcd;
    int currentStatusScreen = 0;
    unsigned long lastStatusScreenChange = 0;
    unsigned long lastStatusUpdate = 0;
    void updateStatusDisplay(const DryerState& state);
    void drawStatusScreen_Main(const DryerState& state);
    void drawStatusScreen_Chamber(const DryerState& state);
    void drawStatusScreen_PSU(const DryerState& state);

    U8G2_ST7565_EA_DOGM128_F_4W_SW_SPI interactiveGcd;
    bool is_sleeping = false;
    void updateInteractiveDisplay(const DryerState& state, const String& profileName);

    void drawGenericMenu(const DryerState& state, const char* options[], int optionCount, const char* title);
    
    void drawSetTimeMenu(const DryerState& state);
    
    void drawSetCustomValueMenu(const DryerState& state, const char* title);
    void drawSpoolStatusScreen(const DryerState& state);
    void drawWifiStatusScreen(const DryerState& state);
};

#endif