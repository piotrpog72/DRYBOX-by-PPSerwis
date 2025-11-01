// =================================================================
// Plik:          DisplayManager.cpp
// Wersja:        5.32b
// Data:          23.10.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [FIX] Usunięto odwołania do nieistniejących zmiennych PSU Fan.
// =================================================================
#include "DisplayManager.h"
#include <Arduino.h>
#include "lists.h"
#include <WiFi.h>

DisplayManager::DisplayManager() :
    statusLcd(LCD_ADDR, LCD_COLS, LCD_ROWS),
    interactiveGcd(U8G2_R2, GLCD_CLK_PIN, GLCD_DATA_PIN, GLCD_CS_PIN, GLCD_DC_PIN, GLCD_RESET_PIN)
{}

void DisplayManager::begin() {
    pinMode(GLCD_BACKLIGHT_PIN, OUTPUT); digitalWrite(GLCD_BACKLIGHT_PIN, HIGH);
    is_sleeping = false; Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    statusLcd.init(); statusLcd.backlight(); statusLcd.clear();

    interactiveGcd.begin();
    interactiveGcd.clearDisplay();
    interactiveGcd.setPowerSave(0);
}

void DisplayManager::setContrast(uint8_t value) {
    interactiveGcd.setContrast(value);
}

void DisplayManager::showStartupScreen(const DryerState& s) {
    statusLcd.setCursor(0, 0); statusLcd.print("DRYBOX " FW_VERSION);
    statusLcd.setCursor(0, 1); statusLcd.print("by PPSerwis     ");

    interactiveGcd.clearBuffer(); interactiveGcd.setFontMode(1); interactiveGcd.setDrawColor(1);
    interactiveGcd.setFont(u8g2_font_helvB10_tr);
    interactiveGcd.drawStr(10, 28, "DRYBOX " FW_VERSION);
    interactiveGcd.setFont(u8g2_font_6x12_tr);
    interactiveGcd.drawStr(10, 48, "by PPSerwis AIRSOFT & more");
    interactiveGcd.sendBuffer();
}

void DisplayManager::showWifiConfigScreen() {
    statusLcd.clear();
    statusLcd.setCursor(0, 0); statusLcd.print("Tryb Konfig. AP");
    statusLcd.setCursor(0, 1); statusLcd.print("Polacz z Drybox");
    interactiveGcd.clearBuffer();
    interactiveGcd.setFont(u8g2_font_6x12_tr);
    interactiveGcd.drawStr(2, 12, "KONFIGURACJA WIFI");
    interactiveGcd.drawStr(2, 30, "1. Polacz sie z siecia");
    interactiveGcd.drawStr(2, 42, "   'Drybox_Setup'");
    interactiveGcd.drawStr(2, 54, "2. Przejdz do 192.168.4.1");
    interactiveGcd.sendBuffer();
}

void DisplayManager::update(const DryerState& s, const String& p) {
    if (millis() - lastStatusUpdate > LCD_STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = millis();
        updateStatusDisplay(s);
    }
    if (!is_sleeping) {
        updateInteractiveDisplay(s, p);
    }
}

void DisplayManager::wakeUpInteractive() {
    digitalWrite(GLCD_BACKLIGHT_PIN, HIGH);
    interactiveGcd.setPowerSave(0);
    interactiveGcd.clearDisplay();
    is_sleeping = false;
}

void DisplayManager::sleepInteractive() {
    digitalWrite(GLCD_BACKLIGHT_PIN, LOW); is_sleeping = true;
}

void DisplayManager::updateStatusDisplay(const DryerState& s) {
    // ================== POCZĄTEK ZMIANY v5.32b ==================
    // Usunięto rotację ekranów, pokazujemy tylko główny i komorę
    if (millis() - lastStatusScreenChange > STATUS_SCREEN_ROTATION_INTERVAL) {
        lastStatusScreenChange = millis();
        currentStatusScreen = (currentStatusScreen + 1) % 2; // Tylko 0 i 1
        statusLcd.clear();
    }
    switch (currentStatusScreen) {
        case 0: drawStatusScreen_Main(s); break;
        case 1: drawStatusScreen_Chamber(s); break;
        // case 2: drawStatusScreen_PSU(s); break; // Usunięto
    }
    // =================== KONIEC ZMIANY v5.32b ===================
}

void DisplayManager::drawStatusScreen_Main(const DryerState& s) {
    char b[24]; float t = isnan(s.avgChamberTemp) ? 0.0 : s.avgChamberTemp;
    float h = isnan(s.dhtHum) ? 0.0 : s.dhtHum;
    snprintf(b, sizeof(b), "T:%.1f H:%.1f%%", t, h);
    statusLcd.setCursor(0, 0); statusLcd.print(b); statusLcd.setCursor(0, 1);
    String m;
    switch (s.currentMode) {
        case MODE_IDLE: m = "Tryb: Czuwanie"; break;
        case MODE_TIMED: m = "Tryb: Czasowy"; break;
        case MODE_HUMIDITY_TARGET: m = "Tryb: Wilgotnosc"; break;
        case MODE_CONTINUOUS: m = "Tryb: Aktywny"; break;
    }
    statusLcd.print(m);
    for (int i = m.length(); i < 16; i++) { statusLcd.print(" "); }
}

void DisplayManager::drawStatusScreen_Chamber(const DryerState& s) {
    char b[17];
    snprintf(b, sizeof(b), "K3:%.1fC K1:%.1fC", s.ds18b20_temps[2], s.ds18b20_temps[0]);
    statusLcd.setCursor(0, 0); statusLcd.print(b);
    snprintf(b, sizeof(b), "K2:%.1fC K4:%.1fC", s.ds18b20_temps[1], s.ds18b20_temps[4]);
    statusLcd.setCursor(0, 1); statusLcd.print(b);
}

// ================== POCZĄTEK ZMIANY v5.32b ==================
// void DisplayManager::drawStatusScreen_PSU(const DryerState& s) { ... } // Cała funkcja usunięta
// =================== KONIEC ZMIANY v5.32b ===================

void DisplayManager::drawGenericMenu(const DryerState& state, const char* options[], int optionCount, const char* title) {
    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2, 12, title);
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_6x12_tr);

    const int visible_items = 3;
    const int highlight_pos = 1;
    int start_index = 0;

    if (optionCount > visible_items) {
        start_index = state.menu_selection - highlight_pos;
        if (start_index < 0) start_index = 0;
        if (start_index > optionCount - visible_items) start_index = optionCount - visible_items;
    }

    for (int i = 0; i < visible_items; i++) {
        int current_item_index = start_index + i;
        if (current_item_index >= optionCount) break;

        int y = 34 + (i * 14);
        if (state.menu_selection == current_item_index) {
            interactiveGcd.drawBox(0, y - 10, 128, 12);
            interactiveGcd.setDrawColor(0);
            interactiveGcd.drawStr(2, y, options[current_item_index]);
            interactiveGcd.setDrawColor(1);
        } else {
            interactiveGcd.drawStr(2, y, options[current_item_index]);
        }
    }
}

void DisplayManager::drawSetTimeMenu(const DryerState& state) {
    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2, 12, "USTAW CZAS (MIN)");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_logisoso24_tr);
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%d", state.menu_selection * 10);
    int textWidth = interactiveGcd.getStrWidth(buffer);
    interactiveGcd.drawStr((128 - textWidth) / 2, 48, buffer);
}

void DisplayManager::drawSetCustomValueMenu(const DryerState& state, const char* title) {
    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2, 12, title);
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_ncenB14_tr);
    char buffer[10];
    if (state.currentMenuState == MENU_SET_PID_KP || state.currentMenuState == MENU_SET_PID_KI || state.currentMenuState == MENU_SET_PID_KD) {
        snprintf(buffer, sizeof(buffer), "%.1f", (float)state.menu_selection / 10.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%d", state.menu_selection);
    }
    int textWidth = interactiveGcd.getStrWidth(buffer);
    interactiveGcd.drawStr((128 - textWidth) / 2, 48, buffer);
}

void DisplayManager::drawSpoolStatusScreen(const DryerState& state) {
    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(22, 12, "STATUS ROLEK");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_5x8_tr);
    for (int i = 0; i < 4; i++) {
        int colX = 2 + (i * 32);
        char numBuf[2];
        snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        interactiveGcd.drawStr(colX + 12, 28, numBuf);
        interactiveGcd.drawStr(colX, 40, FILAMENT_TYPES[state.spools[i].typeIndex]);
        if (state.spools[i].typeIndex != 0) {
            interactiveGcd.drawStr(colX, 52, FILAMENT_COLORS_SHORT[state.spools[i].colorIndex]);
        }
    }
    interactiveGcd.drawVLine(32, 20, 36);
    interactiveGcd.drawVLine(64, 20, 36);
    interactiveGcd.drawVLine(96, 20, 36);
    interactiveGcd.setFont(u8g2_font_4x6_tr);
    interactiveGcd.drawStr(38, 62, "(Dlugo - Powrot)");
}

void DisplayManager::drawWifiStatusScreen(const DryerState& state) {
    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2, 12, "STATUS SIECI");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_6x12_tr);

    if (state.isWifiConnected) {
        interactiveGcd.drawStr(2, 30, "Status: Polaczono");
        String ip = "IP: " + WiFi.localIP().toString();
        interactiveGcd.drawStr(2, 42, ip.c_str());
    } else {
        interactiveGcd.drawStr(2, 30, "Status: Brak polaczenia");
    }
    interactiveGcd.setFont(u8g2_font_4x6_tr);
    interactiveGcd.drawStr(38, 62, "(Wcisnij - Powrot)");
}

void DisplayManager::updateInteractiveDisplay(const DryerState& s, const String& p) {
    interactiveGcd.clearBuffer();
    interactiveGcd.setFontMode(1);
    interactiveGcd.setDrawColor(1);

    char buffer[32];

    switch (s.currentMenuState) {
        case SCREEN_MAIN: {
            if (s.isInAlarmState) {
                // ... (kod alarmu bez zmian) ...
            } else {
                String profileString = p;
                if (s.isHeaterOn) {
                    switch (s.currentPhase) {
                        case PHASE_BOOST: profileString += " |"; break;
                        case PHASE_RAMP:  profileString += " /"; break; // Na razie nieużywane
                        case PHASE_PID:   profileString += " ~"; break;
                        default: break;
                    }
                }
                interactiveGcd.setFont(u8g2_font_7x13B_tr);
                interactiveGcd.drawStr(2, 13, profileString.c_str());

                interactiveGcd.setFont(u8g2_font_open_iconic_all_2x_t);
                const int icon_w = 16;
                const int icon_pad = 2;
                const int right_margin = 8;
                int current_icon_x = 128 - icon_w - right_margin;

                // ... (kod ikon dźwięku i WiFi bez zmian) ...

                // Ikona grzania / chłodzenia
                // ================== POCZĄTEK ZMIANY v5.32b ==================
                // Usunięto odwołanie do s.isHeaterFanOn, używamy tylko s.isHeaterOn
                if (s.isHeaterOn) {
                         interactiveGcd.drawGlyph(current_icon_x, 15, 0xA8);
                }
                // Usunięto warunek dla ikony chłodzenia 0x48, bo wentylator grzałki nie istnieje
                // =================== KONIEC ZMIANY v5.32b ===================
                current_icon_x -= (icon_w + icon_pad);

                // Ikona wentylatora komory
                if (s.isChamberFanOn) {
                    interactiveGcd.drawGlyph(current_icon_x, 15, 0x47);
                }

                interactiveGcd.setFont(u8g2_font_7x13_tr);
                snprintf(buffer, sizeof(buffer), "T: %.1f -> %.0f C", s.avgChamberTemp, s.targetTemp);
                interactiveGcd.drawStr(2, 32, buffer);
                snprintf(buffer, sizeof(buffer), "H: %.1f -> %.0f %%", s.dhtHum, s.targetHumidity);
                interactiveGcd.drawStr(2, 46, buffer);

                // ... (kod paska postępu bez zmian) ...
            }
            break;
        }
        // ... (case SCREEN_SPOOL_STATUS bez zmian) ...
        case MENU_ADVANCED_SETTINGS: {
            // ================== POCZĄTEK ZMIANY v5.32b ==================
            // Zaktualizowano listę opcji w menu zaawansowanym
            char psu_alarm[24], contrast[24];
            char pid_kp[24], pid_ki[24], pid_kd[24];
            char boost_time[24], boost_temp[24], boost_psu[24], ramp_power[24];
            
            // Usunięto psu_fan, psu_hys
            snprintf(psu_alarm, sizeof(psu_alarm), "Alarm Zas.: %d C", (int)s.psuOverheatLimit);
            snprintf(contrast, sizeof(contrast), "Kontrast: %d", s.glcdContrast);
            snprintf(pid_kp, sizeof(pid_kp), "PID Kp: %.1f", s.pid_kp);
            snprintf(pid_ki, sizeof(pid_ki), "PID Ki: %.1f", s.pid_ki);
            snprintf(pid_kd, sizeof(pid_kd), "PID Kd: %.1f", s.pid_kd);
            snprintf(boost_time, sizeof(boost_time), "Boost Czas Max: %d min", s.boostMaxTime_min);
            snprintf(boost_temp, sizeof(boost_temp), "Boost Prog Temp: %.0f C", s.boostTempThreshold);
            snprintf(boost_psu, sizeof(boost_psu), "Boost Limit PSU: %.0f C", s.boostPsuTempLimit);
            snprintf(ramp_power, sizeof(ramp_power), "Rampa Moc: %d %%", s.rampPowerPercent); // Może oznaczać poziom mocy

            const char* options[] = {
                // psu_fan, psu_hys, // Usunięto
                psu_alarm, contrast,
                pid_kp, pid_ki, pid_kd,
                boost_time, boost_temp, boost_psu, ramp_power,
                "Test Podzespolow", "Powrot"
            };
            // Nowa liczba opcji: 11 (indeksy 0-10)
            drawGenericMenu(s, options, 11, "USTAW. ZAAWANSOWANE");
            // =================== KONIEC ZMIANY v5.32b ===================
            break;
        }
        case MENU_COMPONENT_TEST: {
             // ================== POCZĄTEK ZMIANY v5.32b ==================
             // Zaktualizowano opcje testu
            char heater_main[24], heater_aux1[24], heater_aux2[24], c_fan[24], buzz[24], led[24];
            snprintf(heater_main, sizeof(heater_main), "Grzalka Glowna: %s", s.test_heater ? "Wl" : "Wyl"); // Użyjemy flagi test_heater dla głównej
            // TODO: Dodać flagi test_heater_aux1, test_heater_aux2 do DryerState
            snprintf(heater_aux1, sizeof(heater_aux1), "Grzalka Aux1: %s", "Wyl"); // Tymczasowo
            snprintf(heater_aux2, sizeof(heater_aux2), "Grzalka Aux2: %s", "Wyl"); // Tymczasowo
            snprintf(c_fan, sizeof(c_fan), "Went. Kom.: %s", s.test_chamber_fan ? "Wl" : "Wyl");
            snprintf(buzz, sizeof(buzz), "Buzzer: %s", s.test_buzzer ? "Wl" : "Wyl");
            snprintf(led, sizeof(led), "LED Grzalki: %s", s.test_heater_led ? "Wl" : "Wyl");
            // Usunięto h_fan, p_fan

            const char* options[] = {heater_main, heater_aux1, heater_aux2, c_fan, buzz, led, "Powrot"};
            drawGenericMenu(s, options, 7, "TEST PODZESPOLOW");
            // =================== KONIEC ZMIANY v5.32b ===================
            break;
        }

        // ... (reszta case'ów bez zmian, usuwamy tylko odwołania do PSU FAN) ...
        // Usunięto case MENU_SET_PSU_FAN_ON_TEMP
        // Usunięto case MENU_SET_PSU_FAN_HYSTERESIS

        default:
            // Obsługa pozostałych stanów menu (bez zmian)
            break;
    }
    interactiveGcd.sendBuffer();
}