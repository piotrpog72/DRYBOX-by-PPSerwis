// =================================================================
// Plik:          DryerController.h
// Wersja:        5.28
// Data:          16.10.2025
// Opis Zmian:
//  - [REFACTOR] Usunięto logikę webserwera, zastępując ją
//    obiektem nowej klasy WebManager.
// =================================================================
#ifndef DRYERCONTROLLER_H
#define DRYERCONTROLLER_H

#include "config.h"
#include "DryerState.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "ActuatorManager.h"
#include "WebManager.h" // <-- NOWY INCLUDE
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
  
  // ================== POCZĄTEK ZMIANY v5.27 ==================
  WebManager webManager;
  // =================== KONIEC ZMIANY v5.27 ===================

  unsigned long lastSensorReadTime = 0;
  unsigned long lastHeaterUpdateTime = 0;
  int editingSpoolIndex = 0;

  double pidSetpoint, pidInput, pidOutput;
  PID pid;

  void handleWifi();
  void startWifiConfig();
  void initializeProfiles();
  void processUserInput();
  void loadSettings();
  void saveSettings();
  void startDryingProcess();
  
  // ================== POCZĄTEK ZMIANY v5.27 ==================
  void processWebCommand(String command);
  // =================== KONIEC ZMIANY v5.27 ===================
};
#endif