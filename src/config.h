// =================================================================
// Plik:         config.h
// Wersja:       5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [TASK] Wydłużono INTERACTIVE_DISPLAY_TIMEOUT do 180000 (3 min).
// - [TASK] Zastąpiono stałe progów temp. stałymi progów procentowych.
// - [CHORE] Usunięto DEFAULT_BOOST_TEMP_THRESHOLD i _PSU_TEMP_LIMIT.
// - [CHORE] Usunięto DEFAULT_RAMP_POWER_LEVEL (wartość hardcoded).
// - [CHORE] Aktualizacja FW_VERSION.
// =================================================================
#ifndef CONFIG_H
#define CONFIG_H
#include <DallasTemperature.h>

// ================== POCZĄTEK ZMIANY v5.35 ==================
#define FW_VERSION "v5.35"
// =================== KONIEC ZMIANY v5.35 ===================

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define GLCD_CLK_PIN   33
#define GLCD_DATA_PIN  32
#define GLCD_CS_PIN    15
#define GLCD_DC_PIN    16
#define GLCD_RESET_PIN 2
#define GLCD_BACKLIGHT_PIN 17

#define HEATER_PIN_MAIN 23 // Grzałka 1 (Środek, 23W)
#define HEATER_PIN_AUX1 27 // Grzałka 2 (Bok 1, 23W)
#define HEATER_PIN_AUX2 19 // Grzałka 3 (Bok 2, 23W)

#define CHAMBER_FAN_PIN 26 // Wentylator(y) komory
#define VENTILATION_FAN_PIN 5 // NOWY Wentylator wentylacji

#define HEATER_LED_PIN 13
#define BUZZER_PIN 18
#define ONE_WIRE_BUS 4
#define ENCODER_A_PIN  12
#define ENCODER_B_PIN  14
#define ENCODER_BTN_PIN 25
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
const DeviceAddress SENSOR_ADDRESS_REAR_RIGHT = {0x28, 0xE8, 0xC9, 0xBB, 0x00, 0x00, 0x00, 0x42};
const DeviceAddress SENSOR_ADDRESS_FRONT_LEFT = {0x28, 0xD4, 0x1F, 0xC0, 0x00, 0x00, 0x00, 0x2C};
const DeviceAddress SENSOR_ADDRESS_REAR_LEFT = {0x28, 0x9A, 0x08, 0xC0, 0x00, 0x00, 0x00, 0x67};
const DeviceAddress SENSOR_ADDRESS_PSU = {0x28, 0x46, 0x23, 0x79, 0xA2, 0x00, 0x03, 0x0F};
const DeviceAddress SENSOR_ADDRESS_FRONT_RIGHT = {0x28, 0x5B, 0x18, 0xBC, 0x00, 0x00, 0x00, 0x09};
#define SENSOR_READ_INTERVAL 1000
#define LCD_STATUS_UPDATE_INTERVAL 500
#define STATUS_SCREEN_ROTATION_INTERVAL 7000
// ================== POCZĄTEK ZMIANY v5.35 ==================
#define INTERACTIVE_DISPLAY_TIMEOUT 180000 // 3 minuty (było 60000)
// =================== KONIEC ZMIANY v5.35 ===================
#define STARTUP_SCREEN_DELAY 3000
#define LONG_PRESS_DELAY 1500
#define BUTTON_DEBOUNCE_DELAY 50
#define OVERHEAT_TEMP_LIMIT 75.0
#define DS18B20_ERROR_CODE -127.0
#define MIN_VALID_TEMP -50.0
#define CHAMBER_DELTA_ON 2.0
#define CHAMBER_DELTA_OFF 1.0

#define PSU_OVERHEAT_LIMIT_DEFAULT 55.0

#define GLCD_CONTRAST_DEFAULT 50

#define PID_KP_DEFAULT 100.0 // Wartości dla sterowania ON/OFF
#define PID_KI_DEFAULT 0.1
#define PID_KD_DEFAULT 0.1

#define DEFAULT_BOOST_TIME_MIN 5

// ================== POCZĄTEK ZMIANY v5.35 ==================
// Usunięto stare progi, dodano nowe (procentowe)
#define DEFAULT_BOOST_THRESHOLD_PERCENT 80 // Próg % temp. celu dla wyjścia z Boost
#define DEFAULT_RAMP_THRESHOLD_PERCENT 95  // Próg % temp. celu dla wyjścia z Ramp
// Usunięto DEFAULT_BOOST_TEMP_THRESHOLD
// Usunięto DEFAULT_BOOST_PSU_TEMP_LIMIT
// Usunięto DEFAULT_RAMP_POWER_LEVEL (hardcoded jako Poziom 2)
// =================== KONIEC ZMIANY v5.35 ===================

#define DEFAULT_VENT_INTERVAL_MIN 15 // Domyślny interwał wentylacji (w minutach)
#define DEFAULT_VENT_DURATION_SEC 60 // Domyślny czas wentylacji (w sekundach)

#endif