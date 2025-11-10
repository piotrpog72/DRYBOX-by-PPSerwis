// =================================================================
// Plik:         DisplayManager.cpp
// Werscja:      5.35 final
// Data:         11.11.2025
// Autor:        PPSerwis AIRSOFT & more (modyfikacja: Gemini)
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:     MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
// - [FIX] FINALNA AKCEPTOWALNA KOREKTA: Wprowadzono globalne przesunięcie 
//         całej grafiki o +2 piksele w prawo w celu zneutralizowania 
//         wizualnego wpływu nieusuwalnych artefaktów z lewego marginesu GLCD.
// - [CHORE] Usunięto nieudane maskowanie (-4) i logiczne resety rotacji.
// =================================================================
#include "DisplayManager.h"
#include <Arduino.h>
#include "lists.h"
#include <WiFi.h>

DisplayManager::DisplayManager() : 
    statusLcd(LCD_ADDR, LCD_COLS, LCD_ROWS),
    // Używamy U8G2_R2 w konstruktorze
    interactiveGcd(U8G2_R2, GLCD_CLK_PIN, GLCD_DATA_PIN, GLCD_CS_PIN, GLCD_DC_PIN, GLCD_RESET_PIN)
{}

void DisplayManager::begin() {
    pinMode(GLCD_BACKLIGHT_PIN, OUTPUT); digitalWrite(GLCD_BACKLIGHT_PIN, HIGH); 
    is_sleeping = false; Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    statusLcd.init(); statusLcd.backlight(); statusLcd.clear();
    
    // Twardy reset sprzętowy
    pinMode(GLCD_RESET_PIN, OUTPUT);
    digitalWrite(GLCD_RESET_PIN, LOW);
    delay(10); 
    digitalWrite(GLCD_RESET_PIN, HIGH);
    delay(10); 
    
    interactiveGcd.begin();

    // Wymuszone czyszczenie GDDRAM (Brutal Force Clear)
    interactiveGcd.clearDisplay(); 
    interactiveGcd.clearBuffer();
    do {
        // Pętla Brutal Force Clear
    } while ( interactiveGcd.nextPage() );
    interactiveGcd.firstPage(); 

    interactiveGcd.setPowerSave(0);
}

void DisplayManager::setContrast(uint8_t value) {
    interactiveGcd.setContrast(value);
}

void DisplayManager::showStartupScreen(const DryerState& s) {
    statusLcd.setCursor(0, 0); statusLcd.print("DRYBOX " FW_VERSION);
    statusLcd.setCursor(0, 1); statusLcd.print("by PPSerwis     ");
    
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.clearBuffer(); interactiveGcd.setFontMode(1); interactiveGcd.setDrawColor(1);
    interactiveGcd.setFont(u8g2_font_helvB10_tr);
    interactiveGcd.drawStr(10 + ARTIFACT_MASK_OFFSET, 28, "DRYBOX " FW_VERSION);
    interactiveGcd.setFont(u8g2_font_6x12_tr);
    interactiveGcd.drawStr(10 + ARTIFACT_MASK_OFFSET, 48, "by PPSerwis AIRSOFT & more");
    interactiveGcd.sendBuffer();
}

void DisplayManager::showWifiConfigScreen() {
    statusLcd.clear();
    statusLcd.setCursor(0, 0);
    statusLcd.print("Tryb Konfig. AP");
    statusLcd.setCursor(0, 1);
    statusLcd.print("Polacz z Drybox");
    
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.clearBuffer();
    interactiveGcd.setFont(u8g2_font_6x12_tr);
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 12, "KONFIGURACJA WIFI");
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 30, "1. Polacz sie z siecia");
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 42, "   'Drybox_Setup'");
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 54, "2. Przejdz do 192.168.4.1");
    interactiveGcd.sendBuffer();
}

void DisplayManager::update(const DryerState& s, const String& p) {
    if (millis() - lastStatusUpdate > LCD_STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = millis();
        updateStatusDisplay(s);
    }
    updateInteractiveDisplay(s, p);
}

void DisplayManager::wakeUpInteractive() {
    digitalWrite(GLCD_BACKLIGHT_PIN, HIGH);
    interactiveGcd.setPowerSave(0);

    // Czyszczenie przy wybudzeniu
    interactiveGcd.clearDisplay(); 
    interactiveGcd.clearBuffer();
    do {
        // Pętla Brutal Force Clear
    } while ( interactiveGcd.nextPage() );
    interactiveGcd.firstPage(); 

    is_sleeping = false;
}

void DisplayManager::sleepInteractive() {
    digitalWrite(GLCD_BACKLIGHT_PIN, LOW); 
}

void DisplayManager::updateStatusDisplay(const DryerState& s) {
    if (millis() - lastStatusScreenChange > STATUS_SCREEN_ROTATION_INTERVAL) {
        lastStatusScreenChange = millis();
        currentStatusScreen = (currentStatusScreen + 1) % 3; 
        statusLcd.clear();
    }
    switch (currentStatusScreen) {
        case 0: drawStatusScreen_Main(s); break;
        case 1: drawStatusScreen_Chamber(s); break;
        case 2: drawStatusScreen_PSU(s); break;
    }
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

void DisplayManager::drawStatusScreen_PSU(const DryerState& s) {
    char b[17];
    snprintf(b, sizeof(b), "Zasilacz: %.1fC", s.ds18b20_temps[3]);
    statusLcd.setCursor(0, 0); statusLcd.print(b);
    statusLcd.setCursor(0, 1);
    statusLcd.print("Alarm: ");
    statusLcd.print(s.psuOverheatLimit);
    statusLcd.print("C");
    for (int i = strlen(b); i < 16; i++) { statusLcd.print(" "); }
}


void DisplayManager::drawGenericMenu(const DryerState& state, const char* options[], int optionCount, const char* title) {
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 12, title);
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
        const int menu_offset = 2 + ARTIFACT_MASK_OFFSET; // Domyślny offset dla tekstu w menu
        if (state.menu_selection == current_item_index) {
            interactiveGcd.drawBox(0 + ARTIFACT_MASK_OFFSET, y - 10, 128 - ARTIFACT_MASK_OFFSET, 12); // Tło aktywnego wyboru
            interactiveGcd.setDrawColor(0); 
            interactiveGcd.drawStr(menu_offset, y, options[current_item_index]);
            interactiveGcd.setDrawColor(1); 
        } else {
            interactiveGcd.drawStr(menu_offset, y, options[current_item_index]);
        }
    }
}

void DisplayManager::drawSetTimeMenu(const DryerState& state) {
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 12, "USTAW CZAS (MIN)");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_logisoso24_tr);
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%d", state.menu_selection * 10);
    int textWidth = interactiveGcd.getStrWidth(buffer);
    interactiveGcd.drawStr((128 - textWidth) / 2 + ARTIFACT_MASK_OFFSET, 48, buffer);
}

void DisplayManager::drawSetCustomValueMenu(const DryerState& state, const char* title) {
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 12, title);
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_ncenB14_tr);
    char buffer[10];
    if (state.currentMenuState == MENU_SET_PID_KP || state.currentMenuState == MENU_SET_PID_KI || state.currentMenuState == MENU_SET_PID_KD) {
        snprintf(buffer, sizeof(buffer), "%.1f", (float)state.menu_selection / 10.0);
    } else if (state.currentMenuState == MENU_SET_BOOST_THRESHOLD_PERCENT || state.currentMenuState == MENU_SET_RAMP_THRESHOLD_PERCENT) {
         snprintf(buffer, sizeof(buffer), "%d%%", state.menu_selection);
    } else {
        snprintf(buffer, sizeof(buffer), "%d", state.menu_selection);
    }
    int textWidth = interactiveGcd.getStrWidth(buffer);
    interactiveGcd.drawStr((128 - textWidth) / 2 + ARTIFACT_MASK_OFFSET, 48, buffer);
}

void DisplayManager::drawSpoolStatusScreen(const DryerState& state) {
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(22 + ARTIFACT_MASK_OFFSET, 12, "STATUS ROLEK");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_5x8_tr);
    for (int i = 0; i < 4; i++) {
        int colX = 2 + (i * 32) + ARTIFACT_MASK_OFFSET;
        char numBuf[2];
        snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        interactiveGcd.drawStr(colX + 12, 28, numBuf);
        interactiveGcd.drawStr(colX, 40, FILAMENT_TYPES[state.spools[i].typeIndex]);
        if (state.spools[i].typeIndex != 0) {
            interactiveGcd.drawStr(colX, 52, FILAMENT_COLORS_SHORT[state.spools[i].colorIndex]);
        }
    }
    // Rysowanie linii pionowych i tekstu "Dlugo - Powrot" wymaga precyzyjnego przesunięcia
    interactiveGcd.drawVLine(32 + ARTIFACT_MASK_OFFSET, 20, 36);
    interactiveGcd.drawVLine(64 + ARTIFACT_MASK_OFFSET, 20, 36);
    interactiveGcd.drawVLine(96 + ARTIFACT_MASK_OFFSET, 20, 36);
    interactiveGcd.setFont(u8g2_font_4x6_tr);
    interactiveGcd.drawStr(38 + ARTIFACT_MASK_OFFSET, 62, "(Dlugo - Powrot)");
}

void DisplayManager::drawWifiStatusScreen(const DryerState& state) {
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    interactiveGcd.setFont(u8g2_font_7x13B_tr);
    interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 12, "STATUS SIECI");
    interactiveGcd.drawHLine(0, 16, 128);
    interactiveGcd.setFont(u8g2_font_6x12_tr);

    if (state.isWifiConnected) {
        interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 30, "Status: Polaczono");
        String ip = "IP: " + WiFi.localIP().toString();
        interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 42, ip.c_str());
    } else {
        interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 30, "Status: Brak polaczenia");
    }
    interactiveGcd.setFont(u8g2_font_4x6_tr);
    interactiveGcd.drawStr(38 + ARTIFACT_MASK_OFFSET, 62, "(Wcisnij - Powrot)");
}

void DisplayManager::updateInteractiveDisplay(const DryerState& s, const String& p) {
    interactiveGcd.clearBuffer();
    interactiveGcd.setFontMode(1);
    interactiveGcd.setDrawColor(1);
    
    char buffer[32];
    const int ARTIFACT_MASK_OFFSET = 2; // NOWY OFFSET +2

    switch (s.currentMenuState) {
        case SCREEN_MAIN: {
            if (s.isInAlarmState) {
                interactiveGcd.setFont(u8g2_font_ncenB12_tr);
                int textWidth = interactiveGcd.getStrWidth("ALARM!");
                interactiveGcd.drawStr((128 - textWidth) / 2 + ARTIFACT_MASK_OFFSET, 30, "ALARM!");
                
                interactiveGcd.setFont(u8g2_font_6x12_tr);
                textWidth = interactiveGcd.getStrWidth("PRZEGRZANIE");
                interactiveGcd.drawStr((128 - textWidth) / 2 + ARTIFACT_MASK_OFFSET, 50, "PRZEGRZANIE");
            } else {
                String profileString = p;
                if (s.isHeaterOn) {
                    switch (s.currentPhase) {
                        case PHASE_BOOST: profileString += " |"; break;
                        case PHASE_RAMP:  profileString += " /"; break;
                        case PHASE_PID:   profileString += " ~"; break;
                        default: break;
                    }
                }
                interactiveGcd.setFont(u8g2_font_7x13B_tr);
                interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 13, profileString.c_str()); 
            
                interactiveGcd.setFont(u8g2_font_open_iconic_all_2x_t);
                const int icon_w = 16;
                const int icon_pad = 2;
                const int right_margin = 8;
                int current_icon_x = 128 - icon_w - right_margin + ARTIFACT_MASK_OFFSET; 

                if (s.areSoundsEnabled) {
                    interactiveGcd.drawGlyph(current_icon_x, 15, 0x115); 
                } else {
                    interactiveGcd.drawGlyph(current_icon_x, 15, 0x11B); 
                }
                current_icon_x -= (icon_w + icon_pad);

                if (s.isWifiEnabled) {
                    if (s.isWifiConnected) {
                        interactiveGcd.drawGlyph(current_icon_x, 15, 0xF7); 
                    } else {
                        interactiveGcd.drawGlyph(current_icon_x, 15, 0x57); 
                    }
                }
                current_icon_x -= (icon_w + icon_pad);

                if (s.isHeaterOn) {
                    interactiveGcd.drawGlyph(current_icon_x, 15, 0xA8); 
                }
                current_icon_x -= (icon_w + icon_pad);
                
                if (s.isChamberFanOn) {
                    interactiveGcd.drawGlyph(current_icon_x, 15, 0x47); 
                }
                
                interactiveGcd.setFont(u8g2_font_7x13_tr);
                snprintf(buffer, sizeof(buffer), "T: %.1f -> %.0f C", s.avgChamberTemp, s.targetTemp);
                interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 32, buffer); 
                snprintf(buffer, sizeof(buffer), "H: %.1f -> %.0f %%", s.dhtHum, s.targetHumidity);
                interactiveGcd.drawStr(2 + ARTIFACT_MASK_OFFSET, 46, buffer); 

                if (s.isVentilationFanOn) {
                    interactiveGcd.setFont(u8g2_font_open_iconic_all_2x_t);
                    interactiveGcd.drawGlyph(2 + ARTIFACT_MASK_OFFSET, 62, 0x140); 
                }

                interactiveGcd.setFont(u8g2_font_6x12_tr);
                char b[32];
                int progress = 0;
                
                if (s.currentMode == MODE_IDLE) {
                    snprintf(b, sizeof(b), "Czuwanie");
                } else {
                    unsigned long elapsedSecs = (millis() - s.dryingStartTime) / 1000;
                    if (s.currentMode == MODE_TIMED) {
                        unsigned long totalSecs = s.dryingDurationMinutes * 60;
                        if (totalSecs == 0) totalSecs = 1;
                        unsigned long remainingSecs = (totalSecs > elapsedSecs) ? totalSecs - elapsedSecs : 0;
                        unsigned long remainingMinutes_roundedUp = (remainingSecs + 59) / 60;
                        int rem_H = remainingMinutes_roundedUp / 60;
                        int rem_M = remainingMinutes_roundedUp % 60;
                        snprintf(b, sizeof(b), "Czas: %dh%02dm", rem_H, rem_M);
                        if (totalSecs > 0) progress = (elapsedSecs * 100) / totalSecs;
                    } else {
                        int H = elapsedSecs / 3600;
                        int M = (elapsedSecs % 3600) / 60;
                        snprintf(b, sizeof(b), "Czas: %dh%02dm", H, M);
                        if (s.currentMode == MODE_HUMIDITY_TARGET) {
                            float total_hum_to_remove = s.startingHumidity - s.targetHumidity;
                            if (total_hum_to_remove > 0.1) {
                                float hum_removed = s.startingHumidity - s.dhtHum;
                                progress = (hum_removed * 100) / total_hum_to_remove;
                            } else { progress = 100; }
                        } else if (s.currentMode == MODE_CONTINUOUS) {
                            unsigned long total_elapsed_ms = millis() - s.dryingStartTime;
                            if (total_elapsed_ms > 0) {
                                progress = (s.heaterTotalOnTime_ms * 100) / total_elapsed_ms;
                            }
                        }
                    }
                }
                interactiveGcd.drawStr(0 + ARTIFACT_MASK_OFFSET, 62, b); 
                
                if (s.currentMode != MODE_IDLE) {
                    if (progress < 0) progress = 0; if (progress > 100) progress = 100;
                    snprintf(b, sizeof(b), "%d%%", progress);
                    uint8_t w = interactiveGcd.getStrWidth("100%");
                    interactiveGcd.drawStr(127 - w + ARTIFACT_MASK_OFFSET, 62, b); 
                    uint8_t X = 127 - w - 4 - 30 + ARTIFACT_MASK_OFFSET; 
                    interactiveGcd.drawFrame(X, 55, 30, 8);
                    interactiveGcd.drawBox(X + 1, 56, (28 * progress) / 100, 6);
                }
            }
            break;
        }
        case SCREEN_SPOOL_STATUS:   drawSpoolStatusScreen(s); break;
        
        case MENU_MAIN_SELECT: {
            const char* firstOption = (s.currentMode == MODE_IDLE) ? "Start Suszenia" : "Zatrzymaj Suszenie";
            const char* options[] = {firstOption, "Ustawienia", "Powrot do ekranu"};
            drawGenericMenu(s, options, 3, "MENU GLOWNE");
            break;
        }
        case MENU_PROFILE_SELECT: {
            const char* profiles[] = {"PLA", "PETG", "ABS", "Wlasny"};
            drawGenericMenu(s, profiles, 4, "WYBIERZ PROFIL");
            break;
        }
        case MENU_MODE_SELECT: {
            const char* modes[] = {"Czasowy", "Wilgotnosc", "Aktywny"};
            drawGenericMenu(s, modes, 3, "WYBIERZ TRYB");
            break;
        }
        case MENU_SETTINGS: {
            snprintf(buffer, sizeof(buffer), "Dzwieki: %s", s.areSoundsEnabled ? "Wl" : "Wyl");
            const char* options[] = {"Edytuj Profil Wlasny", buffer, "Ustaw. Zaawansowane", "Ustawienia WiFi", "Etykiety Rolek", "Powrot"};
            drawGenericMenu(s, options, 6, "USTAWIENIA");
            break;
        }
        case MENU_ADVANCED_SETTINGS: {
            char psu_alarm[24], contrast[24];
            char pid_kp[24], pid_ki[24], pid_kd[24];
            char boost_time[24], boost_perc[24], ramp_perc[24];
            char vent_int[24], vent_dur[24];
            
            snprintf(psu_alarm, sizeof(psu_alarm), "Alarm Zas.: %d C", (int)s.psuOverheatLimit);
            snprintf(contrast, sizeof(contrast), "Kontrast: %d", s.glcdContrast);
            snprintf(pid_kp, sizeof(pid_kp), "PID Kp: %.1f", s.pid_kp);
            snprintf(pid_ki, sizeof(pid_ki), "PID Ki: %.1f", s.pid_ki);
            snprintf(pid_kd, sizeof(pid_kd), "PID Kd: %.1f", s.pid_kd);
            snprintf(boost_time, sizeof(boost_time), "Boost Czas Max: %d min", s.boostMaxTime_min);
            snprintf(boost_perc, sizeof(boost_perc), "Boost Prog: %d%%", s.boostThresholdPercent);
            snprintf(ramp_perc, sizeof(ramp_perc), "Rampa Prog: %d%%", s.rampThresholdPercent);
            snprintf(vent_int, sizeof(vent_int), "Went. Interwal: %d min", s.ventilationInterval_min);
            snprintf(vent_dur, sizeof(vent_dur), "Went. Czas: %d sek", s.ventilationDuration_sec);

            const char* options[] = {
                psu_alarm, contrast, 
                pid_kp, pid_ki, pid_kd,
                boost_time, boost_perc, ramp_perc,
                vent_int, vent_dur,
                "Test Podzespolow", "Powrot"
            };
            drawGenericMenu(s, options, 12, "USTAW. ZAAWANSOWANE");
            break;
        }
        case MENU_COMPONENT_TEST: {
            char h_main[24], h_aux1[24], h_aux2[24], c_fan[24], v_fan[24], buzz[24], led[24];
            snprintf(h_main, sizeof(h_main), "Grzalka 1 (23): %s", s.test_heater_main ? "Wl" : "Wyl");
            snprintf(h_aux1, sizeof(h_aux1), "Grzalka 2 (27): %s", s.test_heater_aux1 ? "Wl" : "Wyl");
            snprintf(h_aux2, sizeof(h_aux2), "Grzalka 3 (19): %s", s.test_heater_aux2 ? "Wl" : "Wyl");
            snprintf(c_fan, sizeof(c_fan), "Went. Kom. (26): %s", s.test_chamber_fan ? "Wl" : "Wyl");
            snprintf(v_fan, sizeof(v_fan), "Went. Went. (5): %s", s.test_vent_fan ? "Wl" : "Wyl");
            snprintf(buzz, sizeof(buzz), "Buzzer (18): %s", s.test_buzzer ? "Wl" : "Wyl");
            snprintf(led, sizeof(led), "LED Grzalki (13): %s", s.test_heater_led ? "Wl" : "Wyl");

            const char* options[] = {h_main, h_aux1, h_aux2, c_fan, v_fan, buzz, led, "Powrot"};
            drawGenericMenu(s, options, 8, "TEST PODZESPOLOW");
            break;
        }
        case MENU_WIFI_SETTINGS: {
            char wifi_toggle[20];
            snprintf(wifi_toggle, sizeof(wifi_toggle), "WiFi: %s", s.isWifiEnabled ? "Wlaczone" : "Wylaczone");
            const char* options[] = {wifi_toggle, "Konfiguruj Siec", "Status Sieci", "Powrot"};
            drawGenericMenu(s, options, 4, "USTAWIENIA WIFI");
            break;
        }
        case MENU_SPOOL_SELECT: {
             char spool1[10], spool2[10], spool3[10], spool4[10];
             snprintf(spool1, sizeof(spool1), "Rolka 1");
             snprintf(spool2, sizeof(spool2), "Rolka 2");
             snprintf(spool3, sizeof(spool3), "Rolka 3");
             snprintf(spool4, sizeof(spool4), "Rolka 4");
             const char* options[] = {spool1, spool2, spool3, spool4};
             drawGenericMenu(s, options, 4, "ETYKIETY: WYBIERZ ROLKE");
             break;
        }

        case SCREEN_WIFI_STATUS:    drawWifiStatusScreen(s); break;
        case MENU_SET_TIME:         drawSetTimeMenu(s); break;
        case MENU_SET_CUSTOM_TEMP:  drawSetCustomValueMenu(s, "USTAW TEMP. (C)"); break;
        case MENU_SET_CUSTOM_HUM:   drawSetCustomValueMenu(s, "USTAW WILG. (%)"); break;
        case MENU_SPOOL_SET_TYPE:   drawGenericMenu(s, FILAMENT_TYPES, FILAMENT_TYPE_COUNT, "WYBIERZ TYP"); break;
        case MENU_SPOOL_SET_COLOR:  drawGenericMenu(s, FILAMENT_COLORS_FULL, FILAMENT_COLORS_FULL_COUNT, "WYBIERZ KOLOR"); break;
        
        case MENU_SET_PSU_OVERHEAT_LIMIT: drawSetCustomValueMenu(s, "Alarm Temp. Zas. (C)"); break;
        case MENU_SET_CONTRAST: drawSetCustomValueMenu(s, "KONTRAST LCD"); break;
        case MENU_SET_PID_KP: drawSetCustomValueMenu(s, "USTAW PID Kp"); break;
        case MENU_SET_PID_KI: drawSetCustomValueMenu(s, "USTAW PID Ki"); break;
        case MENU_SET_PID_KD: drawSetCustomValueMenu(s, "USTAW PID Kd"); break;
        case MENU_SET_BOOST_TIME: drawSetCustomValueMenu(s, "BOOST CZAS (MIN)"); break;
        case MENU_SET_BOOST_THRESHOLD_PERCENT: drawSetCustomValueMenu(s, "BOOST PROG (%)"); break;
        case MENU_SET_RAMP_THRESHOLD_PERCENT: drawSetCustomValueMenu(s, "RAMPA PROG (%)"); break;
        case MENU_SET_VENT_INTERVAL: drawSetCustomValueMenu(s, "WENT. INTERWAL (MIN)"); break;
        case MENU_SET_VENT_DURATION: drawSetCustomValueMenu(s, "WENT. CZAS (SEK)"); break;
    }
    interactiveGcd.sendBuffer();
}