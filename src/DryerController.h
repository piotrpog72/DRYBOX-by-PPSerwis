// =================================================================
// Plik:          DryerController.h
// Wersja:        5.26
// Data:          16.10.2025
// Opis Zmian:
//  - [FIX] Dodano brakującą deklarację funkcji startDryingProcess.
// =================================================================
#ifndef DRYERCONTROLLER_H
#define DRYERCONTROLLER_H

#include <WebServer.h>
#include "config.h"
#include "DryerState.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "InputManager.h"
#include "ActuatorManager.h"
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
  unsigned long lastSensorReadTime = 0;
  unsigned long lastHeaterUpdateTime = 0;
  int editingSpoolIndex = 0;

  double pidSetpoint, pidInput, pidOutput;
  PID pid;

  WebServer server;

  void handleWifi();
  void startWifiConfig();
  void initializeProfiles();
  void processUserInput();
  void loadSettings();
  void saveSettings();
  void setupWebServer();
  void handleDataRequest();
  void handleCommandRequest();
  void handleFileRequest(String path, String contentType);
  void handleNotFound();
  
  // ================== POCZĄTEK ZMIANY v5.26 ==================
  void startDryingProcess();
  // =================== KONIEC ZMIANY v5.26 ===================
};
#endif