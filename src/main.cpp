/*
    Projekt: Suszarka do filamentu 3D
    Wersja: v5.20 
    Architektura Page Buffer
    stabilne działające WiFi i OTA
    zmiana sposobu wyświetlania i układu plików
    zmiana typu czujnika na SHT41
    buzzer
    rozwinięte menu ustawień zasilacza
    poprrawka menu głównego  - funkcja powrót do ekranu głównego
    menu testów komponentów
    regulacja kontrastu ekranu
    dodany softPID
    WWW
*/
#include <Arduino.h>
#include "DryerController.h"

DryerController dryerController;

void setup() {
    dryerController.begin();
}

void loop() {
    dryerController.update();
}