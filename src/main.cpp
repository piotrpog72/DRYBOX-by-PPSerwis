// =================================================================
// Plik:          main.cpp
// Wersja:        5.35 final
// Data:          11.11.2025
// Autor:         PPSerwis AIRSOFT & more
// Copyright (c) 2025 PPSerwis AIRSOFT & more
// Licencja:      MIT License (zobacz plik LICENSE w repozytorium)
// Opis Zmian:
//  - [CHORE] Dodano informacje o prawach autorskich i licencji.
// =================================================================
#include <Arduino.h>
#include "DryerController.h"

DryerController dryerController;

void setup() {
    dryerController.begin();
}

void loop() {
    dryerController.update();
}