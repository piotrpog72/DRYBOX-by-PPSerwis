// =================================================================
// Plik:         DryerState.h
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [TASK] Zastąpiono zmienne progów temp. zmiennymi progów procentowych
//          (boostThresholdPercent, rampThresholdPercent).
// - [CHORE] Usunięto boostTempThreshold, boostPsuTempLimit.
// - [CHORE] Usunięto rampPowerPercent (teraz hardcoded jako Poziom 2).
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
  
  // Flagi stanu dla UI
  bool isHeaterOn = false; // Ogólna flaga
  bool isHeaterMainOn = false;
  bool isHeaterAux1On = false;
  bool isHeaterAux2On = false;
  bool isChamberFanOn = false;
  bool isVentilationFanOn = false;

  bool interactiveDisplayActive = true;
  unsigned long lastUserInputTime = 0;
  SpoolData spools[4];
  bool isWifiEnabled = true;
  bool isWifiConnected = false;
  
  bool areSoundsEnabled = true;
  float psuOverheatLimit = PSU_OVERHEAT_LIMIT_DEFAULT;

  bool isInAlarmState = false;

  int glcdContrast = GLCD_CONTRAST_DEFAULT;
  bool isInTestMode = false;
  
  // Nowe flagi testowe
  bool test_heater_main = false;
  bool test_heater_aux1 = false;
  bool test_heater_aux2 = false;
  bool test_chamber_fan = false;
  bool test_vent_fan = false;
  bool test_buzzer = false;
  bool test_heater_led = false;

  double pid_kp = PID_KP_DEFAULT;
  double pid_ki = PID_KI_DEFAULT;
  double pid_kd = PID_KD_DEFAULT;
  double pidOutput = 0; // Przechowuje teraz poziom mocy (0-3)

  HeatingPhase currentPhase = PHASE_OFF;

  uint8_t boostMaxTime_min = DEFAULT_BOOST_TIME_MIN;
  // ================== POCZĄTEK ZMIANY v5.35 ==================
  uint8_t boostThresholdPercent = DEFAULT_BOOST_THRESHOLD_PERCENT;
  uint8_t rampThresholdPercent = DEFAULT_RAMP_THRESHOLD_PERCENT;
  // Usunięto boostTempThreshold
  // Usunięto boostPsuTempLimit
  // Usunięto rampPowerPercent
  // =================== KONIEC ZMIANY v5.35 ===================

  uint16_t ventilationInterval_min = DEFAULT_VENT_INTERVAL_MIN;
  uint16_t ventilationDuration_sec = DEFAULT_VENT_DURATION_SEC;
};
#endif