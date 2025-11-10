// =================================================================
// Plik:         MenuState.h
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [TASK] Zaktualizowano stany menu dla progów procentowych.
// - [CHORE] Usunięto stare stany MENU_SET_BOOST_TEMP, _PSU_TEMP, _RAMP_POWER.
// =================================================================
#ifndef MENUSTATE_H
#define MENUSTATE_H

enum MenuState {
  SCREEN_MAIN,
  SCREEN_SPOOL_STATUS,
  MENU_MAIN_SELECT,
  MENU_PROFILE_SELECT,
  MENU_MODE_SELECT,
  MENU_SET_TIME,
  MENU_SETTINGS,
  MENU_SET_CUSTOM_TEMP,
  MENU_SET_CUSTOM_HUM,
  MENU_SPOOL_SELECT,
  MENU_SPOOL_SET_TYPE,
  MENU_SPOOL_SET_COLOR,
  MENU_WIFI_SETTINGS,
  SCREEN_WIFI_STATUS,
  
  MENU_ADVANCED_SETTINGS,
  MENU_SET_PSU_OVERHEAT_LIMIT,
  MENU_SET_CONTRAST,
  MENU_COMPONENT_TEST,

  MENU_SET_PID_KP,
  MENU_SET_PID_KI,
  MENU_SET_PID_KD,
  
  MENU_SET_BOOST_TIME,
  // ================== POCZĄTEK ZMIANY v5.35 ==================
  MENU_SET_BOOST_THRESHOLD_PERCENT, // Zamiast MENU_SET_BOOST_TEMP
  MENU_SET_RAMP_THRESHOLD_PERCENT,  // Nowy
  // Usunięto MENU_SET_BOOST_PSU_TEMP
  // Usunięto MENU_SET_RAMP_POWER
  // =================== KONIEC ZMIANY v5.35 ===================

  MENU_SET_VENT_INTERVAL,
  MENU_SET_VENT_DURATION
};
#endif