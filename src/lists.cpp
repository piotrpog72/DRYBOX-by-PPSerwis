
// =================================================================
// Plik:          Lists.cpp
// Wersja:        5.28
// Data:          16.10.2025    
// Opis Zmian:
//  
// =================================================================
#include "lists.h"

const char* FILAMENT_TYPES[] = {
    "Pusty", "PLA", "PETG", "ABS", "ASA", "TPU", "Inny"
};
const int FILAMENT_TYPE_COUNT = sizeof(FILAMENT_TYPES) / sizeof(char*);

const char* FILAMENT_COLORS_SHORT[] = {
    "Brak", "Bialy", "Czarny", "Szary", "Czerw.", "Ziel.", "Nieb.", 
    "Zolty", "Pomar.", "Fiolet", "Natur.", "Srebrny", "Zloty", "MultiK."
};
const int FILAMENT_COLOR_COUNT = sizeof(FILAMENT_COLORS_SHORT) / sizeof(char*);

const char* FILAMENT_COLORS_FULL[] = {
    "Brak", "Bialy", "Czarny", "Szary", "Czerwony", "Zielony", "Niebieski", 
    "Zolty", "Pomaranczowy", "Fioletowy", "Naturalny", "Srebrny", "Zloty", "Multikolor"
};
const int FILAMENT_COLORS_FULL_COUNT = sizeof(FILAMENT_COLORS_FULL) / sizeof(char*);