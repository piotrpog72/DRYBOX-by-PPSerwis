// =================================================================
// Plik:          DryerState.h
// Wersja:        5.28
// Data:          16.10.2025
// Opis Zmian:
//  - [REFACTOR] Wprowadzono enum `HeatingPhase` do zarządzania
//    fazami grzania.
//  - [REFACTOR] Usunięto nieużywaną flagę `isBoostActive`.
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

// ================== POCZĄTEK ZMIANY v5.26 ==================
enum HeatingPhase { PHASE_OFF,
                    PHASE_BOOST,
                    PHASE_RAMP,
                    PHASE_PID };
// =================== KONIEC ZMIANY v5.26 ===================

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
  bool isHeaterFanOn = false;
  bool isChamberFanOn = false;
  bool isPsuFanOn = false;
  bool interactiveDisplayActive = true;
  unsigned long lastUserInputTime = 0;
  SpoolData spools[4];
  bool isWifiEnabled = true;
  bool isWifiConnected = false;
  
  bool areSoundsEnabled = true;
  float psuFanOnTemp = PSU_FAN_ON_TEMP_DEFAULT;
  float psuFanOffHysteresis = PSU_FAN_OFF_HYSTERESIS_DEFAULT;
  float psuOverheatLimit = PSU_OVERHEAT_LIMIT_DEFAULT;

  bool isInAlarmState = false;

  int glcdContrast = GLCD_CONTRAST_DEFAULT;
  bool isInTestMode = false;
  bool test_heater = false;
  bool test_heater_fan = false;
  bool test_chamber_fan = false;
  bool test_psu_fan = false;
  bool test_buzzer = false;
  bool test_heater_led = false;

  double pid_kp = PID_KP_DEFAULT;
  double pid_ki = PID_KI_DEFAULT;
  double pid_kd = PID_KD_DEFAULT;
  double pidOutput = 0;

  // ================== POCZĄTEK ZMIANY v5.26 ==================
  HeatingPhase currentPhase = PHASE_OFF; // Zastępuje isBoostActive
  // =================== KONIEC ZMIANY v5.26 ===================

  uint8_t boostMaxTime_min = DEFAULT_BOOST_TIME_MIN;
  float boostTempThreshold = DEFAULT_BOOST_TEMP_THRESHOLD;
  float boostPsuTempLimit = DEFAULT_BOOST_PSU_TEMP_LIMIT;
  uint8_t rampPowerPercent = DEFAULT_RAMP_POWER_PERCENT;
};
#endif