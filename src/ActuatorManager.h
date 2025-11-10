// =================================================================
// Plik:          ActuatorManager.h
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REFACTOR] Przebudowa klasy do sterowania 3 grzałkami 230V.
//  - [FEATURE] Dodano sterowanie wentylatorem wentylacji.
//  - [REFACTOR] Usunięto logikę wentylatora grzałki i zasilacza.
//  - [FEATURE] Dodano zmienne do obsługi cyklera wentylacji.
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

    void forceHeaterMain(bool state);
    void forceHeaterAux1(bool state);
    void forceHeaterAux2(bool state);
    void forceVentilationFan(bool state);
    void forceChamberFan(bool state);
    void forceBuzzer(bool state);
    void forceHeaterLed(bool state);
    void resetAllForced();

private:
    void setHeaterPowerLevel(int level); 
    void setVentilationFan(bool s); 
    void setChamberFan(bool s);
    void setHeaterLed(bool s);
    
    bool isHeaterMainOn_internal = false;
    bool isHeaterAux1On_internal = false;
    bool isHeaterAux2On_internal = false;
    bool isVentilationFanOn_internal = false;
    bool isChamberFanOn_internal = false;

    // Zmienne do obsługi cyklera wentylacji
    unsigned long lastVentilationStartTime = 0;
    bool isVenting = false;

    bool isAlarmActive = false;
    unsigned long lastAlarmBeepTime = 0;
    bool isAlarmBeeping = false;
};
#endif