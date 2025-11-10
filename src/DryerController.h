// =================================================================
// Plik:          DryerController.h
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REVERT] Powrót do bazy v5.30 (processUserInput wewnątrz tej
//    klasy), aby naprawić błąd znikającego menu.
//  - [FIX] Implementacja nowej logiki grzania (Boost->Ramp->PID+Failsafe).
//  - [FEATURE] Dodanie obsługi ustawień wentylacji.
// =================================================================
#ifndef DRYERCONTROLLER_H
#define DRYERCONTROLLER_H

#include "config.h"
#include "DryerState.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "ActuatorManager.h"
#include "WebManager.h"
#include <PID_v1.h>

class DryerController {
public:
  DryerController();
  void begin();
  void update();

private:
  SensorManager sensorManager;
  DisplayManager displayManager;
  InputManager inputManager;
  ActuatorManager actuatorManager;
  DryerState currentState;
  FilamentProfile profiles[4];
  WebManager webManager;

  unsigned long lastSensorReadTime = 0;
  unsigned long lastHeaterUpdateTime = 0;
  int editingSpoolIndex = 0;

  double pidSetpoint, pidInput, pidOutput;
  PID pid;

  void handleWifi();
  void startWifiConfig();
  void initializeProfiles();
  void processUserInput(); // Kluczowa funkcja z logiką menu
  void loadSettings();
  void saveSettings();
  void startDryingProcess();
  void processWebCommand(String command);
};
#endif