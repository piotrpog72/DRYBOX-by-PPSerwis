// =================================================================
// Plik:          DryerState.h
// Wersja:        5.32c
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [FIX] Przywrócono/potwierdzono obecność zmiennej isVentilationFanOn.
//  - Usunięto nieużywane zmienne psuFan*.
// =================================================================
#ifndef DRYERSTATE_H
#define DRYERSTATE_H
#include "config.h"
#include <Arduino.h>
#include "MenuState.h"

enum DryingMode { MODE_IDLE,
                  MODE_TIMED,
                  MODE_HUMIDITY_TARGET,
                  MODE_CONTINUOUS };
enum ProfileType { PROFILE_PLA,
                   PROFILE_PETG,
                   PROFILE_ABS,
                   PROFILE_CUSTOM };

enum HeatingPhase { PHASE_OFF,
                    PHASE_BOOST,
                    PHASE_RAMP,
                    PHASE_PID };

struct FilamentProfile {
  String name;
  float temp;
  float humidity;
};
struct SpoolData {
  int typeIndex;
  int colorIndex;
};

struct DryerState {
  float dhtTemp = 25.0;
  float dhtHum = 50.0;
  float ds18b20_temps[5] = { 25.0 };
  bool sensorsOk = false;
  float avgChamberTemp = 25.0;
  DryingMode currentMode = MODE_IDLE;
  DryingMode previousMode = MODE_IDLE;
  ProfileType currentProfileType = PROFILE_PLA;
  MenuState currentMenuState = SCREEN_MAIN;
  int menu_selection = 0;
  float targetTemp = 45.0;
  float targetHumidity = 15.0;
  unsigned long dryingStartTime = 0;
  unsigned long dryingDurationMinutes = 360;
  float startingHumidity = 0.0;
  unsigned long heaterTotalOnTime_ms = 0;
  bool isHeaterOn = false;
  bool isHeaterFanOn = false; // Pozostaje, bo ActuatorManager go używa wewnętrznie
  bool isChamberFanOn = false;
  // bool isPsuFanOn = false; // Usunięte
  // ================== POCZĄTEK ZMIANY v5.32c ==================
  bool isVentilationFanOn = false; // Upewnij się, że ta linia istnieje!
  // =================== KONIEC ZMIANY v5.32c ===================
  bool interactiveDisplayActive = true;
  unsigned long lastUserInputTime = 0;
  SpoolData spools[4];
  bool isWifiEnabled = true;
  bool isWifiConnected = false;

  bool areSoundsEnabled = true;
  // float psuFanOnTemp = PSU_FAN_ON_TEMP_DEFAULT; // Usunięte
  // float psuFanOffHysteresis = PSU_FAN_OFF_HYSTERESIS_DEFAULT; // Usunięte
  float psuOverheatLimit = PSU_OVERHEAT_LIMIT_DEFAULT;

  bool isInAlarmState = false;

  int glcdContrast = GLCD_CONTRAST_DEFAULT;
  bool isInTestMode = false;
  // Flagi dla testu podzespołów
  bool test_heater = false; // Obecnie używane dla test_heater_main
  bool test_chamber_fan = false;
  bool test_buzzer = false;
  bool test_heater_led = false;
  // TODO: Dodać flagi dla test_heater_aux1, test_heater_aux2, test_vent_fan

  double pid_kp = PID_KP_DEFAULT;
  double pid_ki = PID_KI_DEFAULT;
  double pid_kd = PID_KD_DEFAULT;
  double pidOutput = 0; // Przechowuje teraz poziom mocy (0-3) lub wynik PID (0/1)

  HeatingPhase currentPhase = PHASE_OFF;

  uint8_t boostMaxTime_min = DEFAULT_BOOST_TIME_MIN;
  float boostTempThreshold = DEFAULT_BOOST_TEMP_THRESHOLD;
  float boostPsuTempLimit = DEFAULT_BOOST_PSU_TEMP_LIMIT;
  uint8_t rampPowerPercent = DEFAULT_RAMP_POWER_PERCENT; // Może być użyte do poziomu mocy w Ramp
};
#endif