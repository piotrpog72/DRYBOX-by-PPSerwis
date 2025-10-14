// =================================================================
// Plik:          ActuatorManager.h
// Wersja:        5.20
// Data:          15.10.2025
// Opis Zmian:
//  - Dodano zmienne do obsługi "wolnego" PWM.
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

    void forceHeater(bool state);
    void forceHeaterFan(bool state);
    void forceChamberFan(bool state);
    void forcePsuFan(bool state);
    void forceBuzzer(bool state);
    void forceHeaterLed(bool state);
    void resetAllForced();

private:
    void setHeater(bool state);
    void setHeaterFan(bool s);
    void setChamberFan(bool s);
    void setPsuFan(bool s);
    void setHeaterLed(bool s);
    
    bool isHeaterOn_internal = false;
    bool isHeaterFanOn_internal = false;
    bool isChamberFanOn_internal = false;
    bool isPsuFanOn_internal = false;
    unsigned long heaterOffTime = 0;

    bool isAlarmActive = false;
    unsigned long lastAlarmBeepTime = 0;
    bool isAlarmBeeping = false;
    
    unsigned long windowStartTime;
    const unsigned long windowSize = 5000; // Okno 5 sekund
};
#endif