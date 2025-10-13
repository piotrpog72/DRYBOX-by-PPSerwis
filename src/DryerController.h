// =================================================================
// Plik:          DryerController.h
// Wersja:        5.10
// Opis Zmian:
//  - Wersja skonsolidowana, bez zmian w deklaracjach.
// =================================================================
#ifndef DRYERCONTROLLER_H
#define DRYERCONTROLLER_H

#include <ESPAsyncWebServer.h>

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

  AsyncWebServer *server;
  AsyncWebSocket *ws;
  
  uint8_t lastSentHeaterPower = 0;

  void handleWifi();
  void startWifiConfig();
  void initializeProfiles();
  void processUserInput();
  void loadSettings();
  void saveSettings();
  void setupWebServer();
  void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  void setHeaterPowerWithCheck(uint8_t power);
};
#endif