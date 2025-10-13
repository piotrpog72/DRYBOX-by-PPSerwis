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