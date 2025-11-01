// =================================================================
// Plik:          MenuManager.h
// Wersja:        5.32c
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [FIX] Poprawiono deklarację funkcji processEncoderChange.
// =================================================================
#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include "DryerState.h"
#include "InputManager.h"
#include "DisplayManager.h"

// Definicja możliwych akcji, które MenuManager może zlecić DryerControllerowi
enum MenuAction {
    ACTION_NONE,
    ACTION_START_DRYING_TIMED,
    ACTION_START_DRYING_HUMIDITY,
    ACTION_START_DRYING_CONTINUOUS,
    ACTION_STOP_DRYING,
    ACTION_SAVE_SETTINGS,
    ACTION_TOGGLE_SOUNDS,
    ACTION_START_WIFI_CONFIG,
    ACTION_APPLY_CONTRAST,
    ACTION_APPLY_PID_TUNINGS,
    ACTION_ENTER_TEST_MODE,
    ACTION_EXIT_TEST_MODE_AND_RESET_ACTUATORS,
    ACTION_TOGGLE_TEST_HEATER, // TODO: Zmienić na HEATER_MAIN?
    ACTION_TOGGLE_TEST_CHAMBER_FAN,
    ACTION_TOGGLE_TEST_BUZZER,
    ACTION_TOGGLE_TEST_HEATER_LED
    // TODO: Dodać akcje dla HEATER_AUX1, HEATER_AUX2, VENT_FAN
};

class MenuManager {
public:
    MenuManager(DryerState& state, InputManager& input, DisplayManager& display);
    void begin();
    void handleInput();
    MenuAction getAction();
    int getActionData();
    void resetAction();

    void navigateToMainMenu();
    void navigateToScreenMain();

private:
    DryerState& _state;
    InputManager& _input;
    DisplayManager& _display;

    MenuAction _currentAction;
    int _actionData;
    int _editingSpoolIndex;

    void processShortPress();
    void processLongPress();
    // ================== POCZĄTEK ZMIANY v5.32c ==================
    void processEncoderChange(long newValue); // Zmieniono deklarację
    // =================== KONIEC ZMIANY v5.32c ===================
};

#endif // MENUMANAGER_H