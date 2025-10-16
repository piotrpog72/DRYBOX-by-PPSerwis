// =================================================================
// Plik:          MenuState.h
// Wersja:        5.26
// Data:          16.10.2025
// Opis Zmian:
//  - Dodano nowe stany menu dla opcji sterowania grzaniem.
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
  MENU_SET_PSU_FAN_ON_TEMP,
  MENU_SET_PSU_FAN_HYSTERESIS,
  MENU_SET_PSU_OVERHEAT_LIMIT,

  MENU_SET_CONTRAST,
  MENU_COMPONENT_TEST,

  MENU_SET_PID_KP,
  MENU_SET_PID_KI,
  MENU_SET_PID_KD,
  
  MENU_SET_BOOST_TIME,
  MENU_SET_BOOST_TEMP,
  MENU_SET_BOOST_PSU_TEMP,
  MENU_SET_RAMP_POWER
};
#endif