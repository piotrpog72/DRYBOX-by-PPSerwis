// =================================================================
// Plik:          SensorManager.h
// Wersja:        5.30
// Data:          17.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [CHORE] Dodano informacje o prawach autorskich i licencji.
// =================================================================
#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Adafruit_SHT4x.h> // Zamiast DHT_U.h

#include "config.h"
#include "DryerState.h"
#include <OneWire.h> // Potrzebne dla DallasTemperature

class SensorManager {
public:
    SensorManager();
    void begin();
    void readSensors(DryerState* state);
    bool areSensorsFound();

private:
    Adafruit_SHT4x sht4x; // Obiekt dla nowego czujnika SHT41
    OneWire oneWire;
    DallasTemperature ds18b20;
    bool sensorsOk = false;
    DeviceAddress sensorAddresses[5];
};

#endif