/*
    Projekt: Suszarka do filamentu 3D
    Wersja: v5.28
    Data: 16.10.2025
    Autor: PPSerwis & Gemini  
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