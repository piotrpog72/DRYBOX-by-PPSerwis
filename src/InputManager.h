// =================================================================
// Plik:          InputManager.h
// Wersja:        5.26
// Data:          16.10.2025
// Opis Zmian:
//  - Dodano nowe opcje do menu "Ustawienia Zaawansowane".
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