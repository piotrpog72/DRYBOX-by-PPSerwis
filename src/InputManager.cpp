// =================================================================
// Plik:          InputManager.cpp
// Wersja:        5.26
// Data:          16.10.2025    
// Opis Zmian:
// 
// =================================================================
#include "InputManager.h"
#include <Arduino.h>

InputManager::InputManager() {}

void InputManager::begin() {
    pinMode(ENCODER_BTN_PIN, INPUT_PULLUP);
    encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    encoder.setCount(0);
}

void InputManager::update() {
    checkButton();
}

bool InputManager::shortPressTriggered() {
    if (short_press_event) {
        short_press_event = false;
        return true;
    }
    return false;
}

bool InputManager::longPressTriggered() {
    if (long_press_event) {
        long_press_event = false;
        return true;
    }
    return false;
}

long InputManager::getEncoderValue() {
    // Zwraca odwróconą wartość, aby obrót w prawo był dodatni
    return (encoder.getCount() / 4) * -1;
}

void InputManager::resetEncoder() {
    encoder.setCount(0);
}

void InputManager::resetEncoder(long value) {
    // Ustawia odwróconą wartość, aby zachować spójność
    encoder.setCount(value * 4 * -1);
}

void InputManager::checkButton() {
    if (millis() - lastButtonCheckTime < BUTTON_DEBOUNCE_DELAY) return;
    lastButtonCheckTime = millis();
    bool p = (digitalRead(ENCODER_BTN_PIN) == LOW);
    if (p && currentButtonState == RELEASED) {
        buttonPressStartTime = millis();
        currentButtonState = PRESSED;
    } else if (!p && currentButtonState == PRESSED) {
        unsigned long u = millis() - buttonPressStartTime;
        if (u >= LONG_PRESS_DELAY) {
            long_press_event = true;
            Serial.println("Button:Long");
        } else {
            short_press_event = true;
            Serial.println("Button:Short");
        }
        currentButtonState = RELEASED;
    }
}