// =================================================================
// Plik:          config.h
// Wersja:        5.28
// Opis Zmian:
//  - Podniesienie numeru wersji FW_VERSION.
//  - Dodano domyślne stałe dla nowej logiki grzania (Boost/Rampa).
//  - naprawiony serwer WWWW
//  - poprawki w wyświetlaniu ikony grzania
//  - poprawki w logice wyświetlania ekranu interaktywnego
//  - poprawki w logice suszenia w trybie wilgotności
//  - poprawki w logice obsługi przycisków enkodera
//  - poprawki w logice odczytu czujników temperatury   
//  - wyodrębnienie całej logiki webserwera do nowej klasy WebManager 
//  - Dodano obsługę zapisu ustawień z Web UI.       
// =================================================================
#ifndef CONFIG_H
#define CONFIG_H
#include <DallasTemperature.h>

#define FW_VERSION "v5.28" // Wersja z zaawansowanym sterowaniem grzaniem
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define GLCD_CLK_PIN   33
#define GLCD_DATA_PIN  32
#define GLCD_CS_PIN    15
#define GLCD_DC_PIN    16
#define GLCD_RESET_PIN 2
#define GLCD_BACKLIGHT_PIN 17
#define HEATER_PIN 23
#define HEATER_FAN_PIN 19
#define CHAMBER_FAN_PIN 26
#define POWER_SUPPLY_FAN_PIN 27
#define HEATER_LED_PIN 13
#define BUZZER_PIN 18
#define DHT_PIN 5
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
#define INTERACTIVE_DISPLAY_TIMEOUT 60000
#define STARTUP_SCREEN_DELAY 3000
#define HEATER_FAN_COOLDOWN_TIME 30000
#define LONG_PRESS_DELAY 1500
#define BUTTON_DEBOUNCE_DELAY 50
#define OVERHEAT_TEMP_LIMIT 75.0
#define DS18B20_ERROR_CODE -127.0
#define MIN_VALID_TEMP -50.0
#define CHAMBER_DELTA_ON 2.0
#define CHAMBER_DELTA_OFF 1.0

#define PSU_FAN_ON_TEMP_DEFAULT 40.0
#define PSU_FAN_OFF_HYSTERESIS_DEFAULT 5.0
#define PSU_OVERHEAT_LIMIT_DEFAULT 55.0

#define GLCD_CONTRAST_DEFAULT 50

#define PID_KP_DEFAULT 4.0
#define PID_KI_DEFAULT 0.2
#define PID_KD_DEFAULT 1.0

// Domyślne wartości dla zaawansowanego sterowania grzaniem
#define DEFAULT_BOOST_TIME_MIN 5
#define DEFAULT_BOOST_TEMP_THRESHOLD 35.0
#define DEFAULT_BOOST_PSU_TEMP_LIMIT 50.0
#define DEFAULT_RAMP_POWER_PERCENT 70

#endif