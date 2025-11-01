// =================================================================
// Plik:          ActuatorManager.h
// Wersja:        5.32
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REFACTOR] Dostosowano do sterowania trzema grzałkami 230V.
//  - [REFACTOR] Usunięto logikę wentylatora grzałki.
// =================================================================
#ifndef ACTUATORMANAGER_H
#define ACTUATORMANAGER_H
#include "config.h"
#include "DryerState.h"

class ActuatorManager {
public:
    ActuatorManager();
    void begin();
    void update(DryerState& state);

    void playCompletionSound();
    void playAlarmSound(bool play);

    // ================== POCZĄTEK ZMIANY v5.32 ==================
    // Funkcje force dla indywidualnych grzałek (do testów)
    void forceHeaterMain(bool state);
    void forceHeaterAux1(bool state);
    void forceHeaterAux2(bool state);
    // Usunięto forceHeaterFan
    // =================== KONIEC ZMIANY v5.32 ===================
    void forceChamberFan(bool state);
    void forceVentilationFan(bool state); // Przyszła wentylacja
    void forceBuzzer(bool state);
    void forceHeaterLed(bool state);
    void resetAllForced();

private:
    // ================== POCZĄTEK ZMIANY v5.32 ==================
    // Główna funkcja sterująca mocą grzewczą
    void setHeaterPowerLevel(int level); // 0=OFF, 1=23W, 2=46W, 3=69W
    // Usunięto setHeater(bool state)
    // Usunięto setHeaterFan(bool s)
    // =================== KONIEC ZMIANY v5.32 ===================
    void setChamberFan(bool s);
    void setVentilationFan(bool s); // Przyszła wentylacja
    void setHeaterLed(bool s);
    
    // ================== POCZĄTEK ZMIANY v5.32 ==================
    bool isHeaterMainOn_internal = false;
    bool isHeaterAux1On_internal = false;
    bool isHeaterAux2On_internal = false;
    // Usunięto isHeaterFanOn_internal
    // Usunięto heaterOffTime
    // =================== KONIEC ZMIANY v5.32 ===================
    bool isChamberFanOn_internal = false;
    bool isVentilationFanOn_internal = false; // Przyszła wentylacja

    bool isAlarmActive = false;
    unsigned long lastAlarmBeepTime = 0;
    bool isAlarmBeeping = false;

    // Usunięto zmienne do "wolnego PWM"
};
#endif