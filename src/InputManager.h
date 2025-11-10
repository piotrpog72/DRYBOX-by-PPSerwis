// =================================================================
// Plik:          InputManager.h
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [CHORE] Dodano informacje o prawach autorskich i licencji.
// =================================================================
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H
#include <Arduino.h> 
#include <ESP32Encoder.h>
#include "config.h"
class InputManager {
public:
  InputManager();
  void begin();
  void update();
  bool shortPressTriggered();
  bool longPressTriggered();
  long getEncoderValue();
  void resetEncoder();
  void resetEncoder(long value);
private:
  ESP32Encoder encoder;
  bool short_press_event = false;
  bool long_press_event = false;
  enum ButtonState { RELEASED,
                     PRESSED };
  ButtonState currentButtonState = RELEASED;
  unsigned long buttonPressStartTime = 0;
  unsigned long lastButtonCheckTime = 0;
  void checkButton();
};
#endif