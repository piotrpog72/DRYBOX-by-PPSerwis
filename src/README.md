Tytuł: Drybox v5 - Zaawansowana Suszarka do Filamentu 3D (Filament Dryer)
Wersja stabilna 5.35 final

Opis: Wielofunkcyjna, inteligentna suszarka do filamentu do druku 3D oparta na mikrokontrolerze ESP32, 
z podwójnym wyświetlaczem, zaawansowanym sterowaniem grzaniem i pełnym interfejsem webowym.

KLUCZOWE FUNKCJE (FEATURES)
Precyzyjne Sensory: Monitorowanie temperatury i wilgotności w czasie rzeczywistym (SHT41, 5x DS18B20).
Zaawansowane Sterowanie Grzaniem: Trójfazowa logika (Boost, Ramp, PID) dla szybkiego i stabilnego nagrzewania z wykorzystaniem trzech stopni grzania 23W, 46W, 69W.
Wiele Trybów Suszenia: Czasowy, do osiągnięcia celu wilgotności oraz tryb ciągły.
Profile Filamentów: Wbudowane profile dla PLA, PETG, ABS oraz w pełni konfigurowalny profil własny.
Podwójny Wyświetlacz: Ekran LCD 16x2 do stałego podglądu statusu oraz graficzny GLCD 128x64 do obsługi menu.
Zdalne Sterowanie Web: Pełny interfejs webowy do monitorowania i sterowania suszarką z dowolnego urządzenia w sieci, w tym strona z zaawansowanymi ustawieniami.
Łączność Wi-Fi: Prosta konfiguracja dzięki WiFiManager i aktualizacje oprogramowania przez sieć (OTA - Over-the-Air).
System Alarmowy: Zabezpieczenie przed przegrzaniem komory i zasilacza.

KOMPONENTY SPRZĘTOWE (HARDWARE)
Mikrokontroler: ESP32 DevKitC
Wyświetlacze: LCD 16x2 z konwerterem I2C, GLCD 128x64 (ST7565)
Czujniki: 1x SHT41 (I2C), 5x DS18B20 (OneWire)
Interfejs: Enkoder obrotowy z przyciskiem
Elementy wykonawcze: Grzałki z drutu oporowego 23W x3, tranzystor MOSFET, 4x wentylatory 12V, buzzer, pasek LED
Zasilanie: Zasilacz 12V (np. 5A), opcjonalnie osobny zasilacz 5V dla logiki, przekaźnik SSR 230V 2A czteropozycyjny do sterowania grzałkami

INSTALACJA I URUCHOMIENIE (INSTALLATION)
Krótki przewodnik krok po kroku, jak wgrać oprogramowanie i uruchomić urządzenie.
    Wymagania: Visual Studio Code z dodatkiem PlatformIO.
    Klonowanie Repozytorium: Instrukcja, jak pobrać kod (np. git clone ...).
    Biblioteki: PlatformIO automatycznie pobierze wszystkie wymagane biblioteki z pliku platformio.ini.
    Kompilacja i Wgranie:
        Podłącz ESP32 do komputera.
        Użyj opcji "Upload and Monitor" w PlatformIO, aby wgrać oprogramowanie.
        Użyj opcji "Upload Filesystem Image", aby wgrać pliki interfejsu webowego (index.html, style.css, settings.html) z folderu data.
    Pierwsze Uruchomienie:
        Po uruchomieniu urządzenie stworzy punkt dostępowy Wi-Fi o nazwie "Drybox_Setup".
        Połącz się z tą siecią i przejdź pod adres 192.168.4.1, aby skonfigurować połączenie z Twoją domową siecią Wi-Fi.

UŻYTKOWANIE (USAGE)
Krótka instrukcja obsługi.
    Menu Urządzenia: Nawigacja odbywa się za pomocą enkodera obrotowego. Obrót zmienia opcje, krótkie naciśnięcie zatwierdza wybór, a długie naciśnięcie cofa do poprzedniego menu.
    Interfejs Webowy: Dostępny pod adresem http://drybox.local (jeśli sieć obsługuje mDNS) lub pod adresem IP wyświetlanym na ekranie.
    Ustawienia: Pełna konfiguracja parametrów jest dostępna w menu urządzenia oraz na podstronie /settings w interfejsie webowym.

LICENCJA (LICENSE)
Projekt jest udostępniany na licencji MIT. Pełna treść licencji znajduje się w pliku LICENSE.
Copyright (c) 2025 Piotr Pogorzałek PPSerwis AIRSOFT & more  