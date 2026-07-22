#include "ultrason.h"
#include "config.h"

// Facteur de conversion : aller-retour du son ~58 us par cm.
#define ULTRASON_US_PAR_CM   58UL

static int distanceCache = -1;

void Ultrason_Initialiser(void) {
    pinMode(PIN_ULTRASON_TRIG, OUTPUT);
    pinMode(PIN_ULTRASON_ECHO, INPUT);
    digitalWrite(PIN_ULTRASON_TRIG, LOW);
}

// Un tir : impulsion trigger 10 us, mesure de la duree d'echo.
static int mesurer(void) {
    digitalWrite(PIN_ULTRASON_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASON_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASON_TRIG, LOW);

    unsigned long timeout = (unsigned long)ULTRASON_DISTANCE_MAX * ULTRASON_US_PAR_CM;
    unsigned long duree = pulseIn(PIN_ULTRASON_ECHO, HIGH, timeout);

    if (duree == 0) return ULTRASON_DISTANCE_MAX;   // aucun echo = voie degagee
    int cm = (int)(duree / ULTRASON_US_PAR_CM);
    return (cm > ULTRASON_DISTANCE_MAX) ? ULTRASON_DISTANCE_MAX : cm;
}

void Ultrason_MettreAJour(void) {
    static unsigned long tPrecedent = 0;
    unsigned long maintenant = millis();
    if (maintenant - tPrecedent < ULTRASON_PERIODE_MS) return;
    tPrecedent = maintenant;
    distanceCache = mesurer();
}

int Ultrason_DistanceCm(void) {
    return distanceCache;
}