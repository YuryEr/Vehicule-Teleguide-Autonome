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

// Attend que ECHO atteigne l'etat demande, dans la limite impartie.
// Retourne l'instant du changement, ou 0 si le delai expire.
static unsigned long attendreEtat(int etat, unsigned long limiteUs) {
    unsigned long t0 = micros();
    while (digitalRead(PIN_ULTRASON_ECHO) != etat) {
        if (micros() - t0 >= limiteUs) return 0;
    }
    return micros();
}

// Un HC-SR04 alimente repond toujours sur ECHO, meme sans obstacle
// (~38 ms a vide). Aucun front montant = capteur absent.
static bool detecterPresence(void) {
    declencher();
    return attendreEtat(HIGH, ULTRASON_TIMEOUT_PRESENCE_US) != 0;
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

    unsigned long debut = attendreEtat(HIGH, ULTRASON_TIMEOUT_US);
    if (debut == 0) return ULTRASON_DISTANCE_MAX;   // aucun echo = voie degagee

    unsigned long fin = attendreEtat(LOW, ULTRASON_TIMEOUT_US);
    if (fin == 0) return ULTRASON_DISTANCE_MAX;     // echo bloque : mesure ignoree

    int cm = (int)((fin - debut) / ULTRASON_US_PAR_CM);
    return (cm > ULTRASON_DISTANCE_MAX) ? ULTRASON_DISTANCE_MAX : cm;
}

void Ultrason_MettreAJour(void) {
    if (!capteurPresent) return;      // capteur absent : aucune attente sur ECHO

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
