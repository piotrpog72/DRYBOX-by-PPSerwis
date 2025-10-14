// =================================================================
// Plik:          SensorManager.cpp
// Wersja:        5.20
// Opis Zmian:
//  
// =================================================================
#include "SensorManager.h"
#include <Arduino.h>

SensorManager::SensorManager()
    : oneWire(ONE_WIRE_BUS), ds18b20(&oneWire) {
    memcpy(sensorAddresses[0], SENSOR_ADDRESS_REAR_RIGHT, 8);
    memcpy(sensorAddresses[1], SENSOR_ADDRESS_FRONT_LEFT, 8);
    memcpy(sensorAddresses[2], SENSOR_ADDRESS_REAR_LEFT, 8);
    memcpy(sensorAddresses[3], SENSOR_ADDRESS_PSU, 8);
    memcpy(sensorAddresses[4], SENSOR_ADDRESS_FRONT_RIGHT, 8);
}

void SensorManager::begin() {
    if (!sht4x.begin()) {
        Serial.println("SensorManager: Nie znaleziono czujnika SHT41!");
    } else {
        Serial.println("SensorManager: SHT41 OK.");
        sht4x.setPrecision(SHT4X_HIGH_PRECISION);
        sht4x.setHeater(SHT4X_NO_HEATER);
    }

    ds18b20.begin();
    uint8_t deviceCount = ds18b20.getDeviceCount();
    if (deviceCount < 5) {
        Serial.print("SensorManager: Ostrzeżenie! Znaleziono tylko ");
        Serial.print(deviceCount);
        Serial.println("/5 czujników DS18B20.");
    } else {
        Serial.println("SensorManager: DS18B20 OK (znaleziono 5/5).");
    }
    sensorsOk = (deviceCount > 0);
    if (!sensorsOk) {
        Serial.println("SensorManager: Błąd krytyczny! Nie znaleziono żadnego czujnika DS18B20.");
    }
}

void SensorManager::readSensors(DryerState* s) {
    sensors_event_t humidity, temp;
    sht4x.getEvent(&humidity, &temp);
    
    bool shtOk = false;
    if (!isnan(temp.temperature)) {
        s->dhtTemp = temp.temperature;
        shtOk = true;
    }
    if (!isnan(humidity.relative_humidity)) {
        s->dhtHum = humidity.relative_humidity;
    }

    ds18b20.requestTemperatures();
    float ds18b20_sum = 0;
    int valid_ds18b20_count = 0;
    for (int i = 0; i < 5; i++) {
        float p = ds18b20.getTempC(sensorAddresses[i]);
        if (p > MIN_VALID_TEMP && p != DS18B20_ERROR_CODE) {
            s->ds18b20_temps[i] = p;
            if (i != 3) {
                ds18b20_sum += p;
                valid_ds18b20_count++;
            }
        } else {
            s->ds18b20_temps[i] = -999.0;
        }
    }

    // GŁÓWNA ZMIANA: Priorytetem jest czujnik SHT41
    if (shtOk) {
        s->avgChamberTemp = s->dhtTemp; // Użyj dokładnego odczytu z SHT41
    } else if (valid_ds18b20_count > 0) {
        s->avgChamberTemp = ds18b20_sum / valid_ds18b20_count; // Wartość zapasowa: średnia z DS18B20
    }
    // Jeśli oba zawiodą, pozostanie ostatnia znana wartość
}

bool SensorManager::areSensorsFound() {
    return sensorsOk;
}