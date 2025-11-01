// =================================================================
// Plik:          DryerController.h
// Wersja:        5.31
// Data:          18.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [REFACTOR] Usunięto logikę menu, zastępując ją
//    obiektem nowej klasy MenuManager.
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
#include "MenuManager.h" // <-- NOWY INCLUDE
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
  
  // ================== POCZĄTEK ZMIANY v5.31 ==================
  MenuManager menuManager;
  // =================== KONIEC ZMIANY v5.31 ===================

  unsigned long lastSensorReadTime = 0;
  unsigned long lastHeaterUpdateTime = 0;
  // int editingSpoolIndex = 0; // Przeniesione do MenuManager

  double pidSetpoint, pidInput, pidOutput;
  PID pid;

  void handleWifi();
  void startWifiConfig();
  void initializeProfiles();
  // void processUserInput(); // Usunięte
  void loadSettings();
  void saveSettings();
  void startDryingProcess();
  void processWebCommand(String command);
  
  // ================== POCZĄTEK ZMIANY v5.31 ==================
  void processMenuAction(MenuAction action, int data); // Nowa funkcja do obsługi akcji z menu
  // =================== KONIEC ZMIANY v5.31 ===================
};
#endif