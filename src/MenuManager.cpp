// =================================================================
// Plik:          MenuManager.cpp
// Wersja:        5.32b
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [FIX] Usunięto odwołania do nieistniejących stanów/zmiennych PSU Fan.
//  - [TODO] Zaktualizować logikę testu podzespołów dla 3 grzałek.
// =================================================================
#include "MenuManager.h"
#include "lists.h"

MenuManager::MenuManager(DryerState& state, InputManager& input, DisplayManager& display)
: _state(state), _input(input), _display(display), _currentAction(ACTION_NONE), _actionData(0), _editingSpoolIndex(0) {}

void MenuManager::begin() {
    _state.currentMenuState = SCREEN_MAIN;
    _state.menu_selection = 0;
}

void MenuManager::handleInput() {
    bool short_pressed = _input.shortPressTriggered();
    bool long_pressed = _input.longPressTriggered();
    long current_encoder_val = _input.getEncoderValue(); // Odczytaj raz na początku

    // Wybudzenie ekranu
    if (short_pressed || long_pressed || current_encoder_val != _state.menu_selection) {
         _state.lastUserInputTime = millis();
        if (!_state.interactiveDisplayActive) {
            _state.interactiveDisplayActive = true;
            _display.wakeUpInteractive();
            return;
        }
    }

    // Przetwarzanie wejścia
    if (_state.interactiveDisplayActive) {
        if (short_pressed) {
            processShortPress();
        } else if (long_pressed) {
            processLongPress();
        } else {
            // Przetwarzaj zmianę enkodera tylko, jeśli się zmieniła wartość
            if (current_encoder_val != _state.menu_selection) {
                 processEncoderChange(current_encoder_val); // Przekaż nową wartość
            }
        }
    }
}


MenuAction MenuManager::getAction() {
    return _currentAction;
}

int MenuManager::getActionData() {
    return _actionData;
}

void MenuManager::resetAction() {
    _currentAction = ACTION_NONE;
    _actionData = 0;
}

void MenuManager::navigateToMainMenu() {
    _state.currentMenuState = MENU_MAIN_SELECT;
    _input.resetEncoder();
    _state.menu_selection = 0;
}

void MenuManager::navigateToScreenMain() {
    _state.currentMenuState = SCREEN_MAIN;
}


void MenuManager::processShortPress() {
    _currentAction = ACTION_NONE;

    switch (_state.currentMenuState) {
        case SCREEN_MAIN: if (!_state.isInAlarmState) navigateToMainMenu(); break;
        case SCREEN_SPOOL_STATUS: case SCREEN_WIFI_STATUS: navigateToMainMenu(); break;

        case MENU_MAIN_SELECT:
            if (_state.menu_selection == 0) {
                if (_state.currentMode != MODE_IDLE) { _currentAction = ACTION_STOP_DRYING; navigateToScreenMain(); }
                else if (!_state.isInAlarmState) { _state.currentMenuState = MENU_PROFILE_SELECT; _input.resetEncoder(); _state.menu_selection = 0; }
            } else if (_state.menu_selection == 1) { _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(); _state.menu_selection = 0; }
            else if (_state.menu_selection == 2) { navigateToScreenMain(); }
            break;

        case MENU_PROFILE_SELECT:
            _state.currentProfileType = (ProfileType)_state.menu_selection;
            _state.currentMenuState = MENU_MODE_SELECT;
            _input.resetEncoder(); _state.menu_selection = 0;
            break;

        case MENU_MODE_SELECT:
            switch(_state.menu_selection) {
                case 0: _state.currentMenuState = MENU_SET_TIME; _input.resetEncoder(36); break;
                case 1: _currentAction = ACTION_START_DRYING_HUMIDITY; break;
                case 2: _currentAction = ACTION_START_DRYING_CONTINUOUS; break;
            }
            break;

        case MENU_SET_TIME:
             _actionData = _state.menu_selection * 10;
            if (_actionData > 0) _currentAction = ACTION_START_DRYING_TIMED;
            break;

        case MENU_SETTINGS:
            if (_state.menu_selection == 0) { _state.currentMenuState = MENU_SET_CUSTOM_TEMP; _input.resetEncoder((int)_state.targetTemp); }
            else if (_state.menu_selection == 1) { _currentAction = ACTION_TOGGLE_SOUNDS; }
            else if (_state.menu_selection == 2) { _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(); _state.menu_selection = 0; }
            else if (_state.menu_selection == 3) { _state.currentMenuState = MENU_WIFI_SETTINGS; _input.resetEncoder(); _state.menu_selection = 0; }
            else if (_state.menu_selection == 4) { _state.currentMenuState = MENU_SPOOL_SELECT; _input.resetEncoder(); _state.menu_selection = 0; }
            else if (_state.menu_selection == 5) { navigateToMainMenu(); }
            break;

        case MENU_ADVANCED_SETTINGS:
            // ================== POCZĄTEK ZMIANY v5.32b ==================
            // Zaktualizowane indeksy po usunięciu opcji wentylatora PSU
            switch(_state.menu_selection) {
                case 0: _state.currentMenuState = MENU_SET_PSU_OVERHEAT_LIMIT; _input.resetEncoder((int)_state.psuOverheatLimit); break; // Było 2
                case 1: _state.currentMenuState = MENU_SET_CONTRAST; _input.resetEncoder(_state.glcdContrast); break; // Było 3
                case 2: _state.currentMenuState = MENU_SET_PID_KP; _input.resetEncoder((int)(_state.pid_kp * 10)); break; // Było 4
                case 3: _state.currentMenuState = MENU_SET_PID_KI; _input.resetEncoder((int)(_state.pid_ki * 10)); break; // Było 5
                case 4: _state.currentMenuState = MENU_SET_PID_KD; _input.resetEncoder((int)(_state.pid_kd * 10)); break; // Było 6
                case 5: _state.currentMenuState = MENU_SET_BOOST_TIME; _input.resetEncoder(_state.boostMaxTime_min); break; // Było 7
                case 6: _state.currentMenuState = MENU_SET_BOOST_TEMP; _input.resetEncoder((int)_state.boostTempThreshold); break; // Było 8
                case 7: _state.currentMenuState = MENU_SET_BOOST_PSU_TEMP; _input.resetEncoder((int)_state.boostPsuTempLimit); break; // Było 9
                case 8: _state.currentMenuState = MENU_SET_RAMP_POWER; _input.resetEncoder(_state.rampPowerPercent); break; // Było 10
                case 9: _currentAction = ACTION_ENTER_TEST_MODE; _state.currentMenuState = MENU_COMPONENT_TEST; _input.resetEncoder(0); break; // Było 11
                case 10: _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(2); break; // Powrót, było 12
            }
            // =================== KONIEC ZMIANY v5.32b ===================
            break;

        case MENU_COMPONENT_TEST:
            // TODO: Zaktualizować akcje dla 3 grzałek
            switch(_state.menu_selection) {
                case 0: _currentAction = ACTION_TOGGLE_TEST_HEATER; break; // Główna
                case 1: /* ACTION_TOGGLE_TEST_HEATER_AUX1 */ break;
                case 2: /* ACTION_TOGGLE_TEST_HEATER_AUX2 */ break;
                case 3: _currentAction = ACTION_TOGGLE_TEST_CHAMBER_FAN; break; // Było 2
                case 4: _currentAction = ACTION_TOGGLE_TEST_BUZZER; break; // Było 4
                case 5: _currentAction = ACTION_TOGGLE_TEST_HEATER_LED; break; // Było 5
                case 6: // Powrót
                    _currentAction = ACTION_EXIT_TEST_MODE_AND_RESET_ACTUATORS;
                    _state.currentMenuState = MENU_ADVANCED_SETTINGS;
                    _input.resetEncoder(9); // Nowy indeks Testu podzespołów
                    break;
            }
            break;

        case MENU_SET_CONTRAST: _state.glcdContrast = _state.menu_selection; _currentAction = ACTION_APPLY_CONTRAST; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(1); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 3
        case MENU_SET_PID_KP: _state.pid_kp = (double)_state.menu_selection / 10.0; _currentAction = ACTION_APPLY_PID_TUNINGS; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(2); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 4
        case MENU_SET_PID_KI: _state.pid_ki = (double)_state.menu_selection / 10.0; _currentAction = ACTION_APPLY_PID_TUNINGS; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(3); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 5
        case MENU_SET_PID_KD: _state.pid_kd = (double)_state.menu_selection / 10.0; _currentAction = ACTION_APPLY_PID_TUNINGS; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(4); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 6
        case MENU_SET_PSU_OVERHEAT_LIMIT: _state.psuOverheatLimit = (float)_state.menu_selection; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(0); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 2
        case MENU_SET_BOOST_TIME: _state.boostMaxTime_min = _state.menu_selection; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(5); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 7
        case MENU_SET_BOOST_TEMP: _state.boostTempThreshold = (float)_state.menu_selection; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(6); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 8
        case MENU_SET_BOOST_PSU_TEMP: _state.boostPsuTempLimit = (float)_state.menu_selection; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(7); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 9
        case MENU_SET_RAMP_POWER: _state.rampPowerPercent = _state.menu_selection; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(8); _currentAction = ACTION_SAVE_SETTINGS; break; // Było 10

        // ================== POCZĄTEK ZMIANY v5.32b ==================
        // Usunięto case MENU_SET_PSU_FAN_ON_TEMP
        // Usunięto case MENU_SET_PSU_FAN_HYSTERESIS
        // =================== KONIEC ZMIANY v5.32b ===================

        case MENU_WIFI_SETTINGS:
            if (_state.menu_selection == 0) { _state.isWifiEnabled = !_state.isWifiEnabled; _currentAction = ACTION_SAVE_SETTINGS; }
            else if (_state.menu_selection == 1) { _currentAction = ACTION_START_WIFI_CONFIG; _state.currentMenuState = MENU_SETTINGS; }
            else if (_state.menu_selection == 2) { _state.currentMenuState = SCREEN_WIFI_STATUS; }
            else if (_state.menu_selection == 3) { _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(3); }
            break;

        case MENU_SPOOL_SELECT:
            _editingSpoolIndex = _state.menu_selection;
            _state.currentMenuState = MENU_SPOOL_SET_TYPE;
            _input.resetEncoder(_state.spools[_editingSpoolIndex].typeIndex);
            break;

        case MENU_SPOOL_SET_TYPE:
            _state.spools[_editingSpoolIndex].typeIndex = _state.menu_selection;
            if (_state.menu_selection == 0) {
                 _state.spools[_editingSpoolIndex].colorIndex = 0;
                 _state.currentMenuState = MENU_SPOOL_SELECT;
                 _input.resetEncoder(_editingSpoolIndex);
                 _currentAction = ACTION_SAVE_SETTINGS;
            } else {
                 _state.currentMenuState = MENU_SPOOL_SET_COLOR;
                 _input.resetEncoder(_state.spools[_editingSpoolIndex].colorIndex);
            }
            break;

        case MENU_SPOOL_SET_COLOR:
            _state.spools[_editingSpoolIndex].colorIndex = _state.menu_selection;
            _state.currentMenuState = MENU_SPOOL_SELECT;
            _input.resetEncoder(_editingSpoolIndex);
            _currentAction = ACTION_SAVE_SETTINGS;
            break;

        case MENU_SET_CUSTOM_TEMP:
             _state.targetTemp = (float)_state.menu_selection;
             _state.currentMenuState = MENU_SET_CUSTOM_HUM;
             _input.resetEncoder((int)_state.targetHumidity);
             break;
        case MENU_SET_CUSTOM_HUM:
             _state.targetHumidity = (float)_state.menu_selection;
             _currentAction = ACTION_SAVE_SETTINGS;
             _state.currentMenuState = MENU_SETTINGS;
             _input.resetEncoder(); _state.menu_selection = 0;
             break;

        default: break;
    }
}

void MenuManager::processLongPress() {
    _currentAction = ACTION_NONE;

    switch (_state.currentMenuState) {
        case SCREEN_MAIN: _state.currentMenuState = SCREEN_SPOOL_STATUS; break;
        case SCREEN_SPOOL_STATUS: _state.currentMenuState = SCREEN_MAIN; break;
        case MENU_PROFILE_SELECT: navigateToMainMenu(); break;
        case MENU_MODE_SELECT: _state.currentMenuState = MENU_PROFILE_SELECT; _input.resetEncoder(_state.currentProfileType); break;
        case MENU_SET_TIME: _state.currentMenuState = MENU_MODE_SELECT; _input.resetEncoder(0); break;
        case MENU_SETTINGS: navigateToMainMenu(); break;
        case MENU_ADVANCED_SETTINGS: _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(2); break;
        case MENU_WIFI_SETTINGS: _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(3); break;
        case MENU_SPOOL_SELECT: _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(4); break;
        case MENU_SET_CUSTOM_TEMP: _state.currentMenuState = MENU_SETTINGS; _input.resetEncoder(0); break;
        case MENU_SET_CUSTOM_HUM: _state.currentMenuState = MENU_SET_CUSTOM_TEMP; _input.resetEncoder((int)_state.targetTemp); break;
        case MENU_SPOOL_SET_TYPE: _state.currentMenuState = MENU_SPOOL_SELECT; _input.resetEncoder(_editingSpoolIndex); break;
        case MENU_SPOOL_SET_COLOR: _state.currentMenuState = MENU_SPOOL_SET_TYPE; _input.resetEncoder(_state.spools[_editingSpoolIndex].typeIndex); break;
        case MENU_COMPONENT_TEST: _currentAction = ACTION_EXIT_TEST_MODE_AND_RESET_ACTUATORS; _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(9); break; // Było 11

        // ================== POCZĄTEK ZMIANY v5.32b ==================
        // Zaktualizowano powrót z menu ustawień wartości
        case MENU_SET_PSU_OVERHEAT_LIMIT: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(0); break; // Było 2
        case MENU_SET_CONTRAST: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(1); break; // Było 3
        case MENU_SET_PID_KP: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(2); break; // Było 4
        case MENU_SET_PID_KI: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(3); break; // Było 5
        case MENU_SET_PID_KD: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(4); break; // Było 6
        case MENU_SET_BOOST_TIME: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(5); break; // Było 7
        case MENU_SET_BOOST_TEMP: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(6); break; // Było 8
        case MENU_SET_BOOST_PSU_TEMP: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(7); break; // Było 9
        case MENU_SET_RAMP_POWER: _state.currentMenuState = MENU_ADVANCED_SETTINGS; _input.resetEncoder(8); break; // Było 10
        // Usunięto MENU_SET_PSU_FAN_ON_TEMP, MENU_SET_PSU_FAN_HYSTERESIS
        // =================== KONIEC ZMIANY v5.32b ===================

        case SCREEN_WIFI_STATUS: _state.currentMenuState = MENU_WIFI_SETTINGS; _input.resetEncoder(2); break;
        default: break;
    }
}

void MenuManager::processEncoderChange(long newValue) {
    long val = newValue; // Użyj przekazanej wartości

    switch (_state.currentMenuState) {
        case MENU_MAIN_SELECT: { int opt = 3; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_PROFILE_SELECT: { int opt = 4; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_MODE_SELECT: { int opt = 3; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_SETTINGS: { int opt = 6; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_ADVANCED_SETTINGS: { int opt = 11; _state.menu_selection = (val % opt + opt) % opt; break; } // Było 13
        case MENU_COMPONENT_TEST: { int opt = 7; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_WIFI_SETTINGS: { int opt = 4; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_SPOOL_SELECT: { int opt = 4; _state.menu_selection = (val % opt + opt) % opt; break; }
        case MENU_SPOOL_SET_TYPE: _state.menu_selection = (val % FILAMENT_TYPE_COUNT + FILAMENT_TYPE_COUNT) % FILAMENT_TYPE_COUNT; break;
        case MENU_SPOOL_SET_COLOR: _state.menu_selection = (val % FILAMENT_COLORS_FULL_COUNT + FILAMENT_COLORS_FULL_COUNT) % FILAMENT_COLORS_FULL_COUNT; break;
        case MENU_SET_TIME: { int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 144) new_val = 144; _state.menu_selection = new_val; break; }
        case MENU_SET_CUSTOM_TEMP: { int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 90) new_val = 90; _state.menu_selection = new_val; break; }
        case MENU_SET_CUSTOM_HUM: { int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 50) new_val = 50; _state.menu_selection = new_val; break; }
        case MENU_SET_PSU_OVERHEAT_LIMIT: { int new_val = val; if (new_val < 45) new_val = 45; if (new_val > 70) new_val = 70; _state.menu_selection = new_val; break; }
        case MENU_SET_CONTRAST: { int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 255) new_val = 255; _state.menu_selection = new_val; _currentAction = ACTION_APPLY_CONTRAST; break; }
        case MENU_SET_PID_KP: case MENU_SET_PID_KI: case MENU_SET_PID_KD: { int new_val = val; if (new_val < 0) new_val = 0; if (new_val > 1000) new_val = 1000; _state.menu_selection = new_val; break; }
        case MENU_SET_BOOST_TIME: { int new_val = val; if (new_val < 1) new_val = 1; if (new_val > 30) new_val = 30; _state.menu_selection = new_val; break; }
        case MENU_SET_BOOST_TEMP: case MENU_SET_BOOST_PSU_TEMP: { int new_val = val; if (new_val < 30) new_val = 30; if (new_val > 60) new_val = 60; _state.menu_selection = new_val; break; }
        case MENU_SET_RAMP_POWER: { int new_val = val; if (new_val < 10) new_val = 10; if (new_val > 100) new_val = 100; _state.menu_selection = new_val; break; }
        default: _state.menu_selection = val; break;
    }
}