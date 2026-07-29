#include "ultrason.h"
#include "config.h"

// Facteur de conversion : aller-retour du son ~58 us par cm.
#define ULTRASON_US_PAR_CM   58UL

static int  distanceCache  = -1;
static bool capteurPresent = false;

// Emet l'impulsion de declenchement (10 us sur TRIG).
static void declencher(void) {
    digitalWrite(PIN_ULTRASON_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRASON_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRASON_TRIG, LOW);
}

// Un HC-SR04 alimente repond toujours sur ECHO, meme sans obstacle
// (~38 ms a vide). Aucune impulsion sur un timeout large = capteur absent.
static bool detecterPresence(void) {
    declencher();
    return pulseIn(PIN_ULTRASON_ECHO, HIGH, ULTRASON_TIMEOUT_PRESENCE_MS) > 0;
}

void Ultrason_Initialiser(void) {
    pinMode(PIN_ULTRASON_TRIG, OUTPUT);
    pinMode(PIN_ULTRASON_ECHO, INPUT);
    digitalWrite(PIN_ULTRASON_TRIG, LOW);
    delay(50);                        // stabilisation avant le sondage
    capteurPresent = detecterPresence();
    distanceCache  = -1;
}

// Un tir : impulsion trigger 10 us, mesure de la duree d'echo.
static int mesurer(void) {
    declencher();

    unsigned long duree = pulseIn(PIN_ULTRASON_ECHO, HIGH, ULTRASON_TIMEOUT_MESURE_MS);

    if (duree == 0) return ULTRASON_DISTANCE_MAX;   // aucun echo = voie degagee
    int cm = (int)(duree / ULTRASON_US_PAR_CM);
    return (cm > ULTRASON_DISTANCE_MAX) ? ULTRASON_DISTANCE_MAX : cm;
}

void Ultrason_MettreAJour(void) {
    if (!capteurPresent) return;      // capteur absent : pas d'attente sur pulseIn

    static unsigned long tPrecedent = 0;
    unsigned long maintenant = millis();
    if (maintenant - tPrecedent < ULTRASON_PERIODE_MS) return;
    tPrecedent = maintenant;
    distanceCache = mesurer();
}

int Ultrason_DistanceCm(void) {
    return distanceCache;
}

bool Ultrason_EstPresent(void) { return capteurPresent; }