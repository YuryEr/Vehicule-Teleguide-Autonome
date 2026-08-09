#include "servo_lidar.h"
#include "config.h"

static int angleCourant = SERVO_ANGLE_CENTRE;
static int angleCible   = SERVO_ANGLE_CENTRE;

static bool          cibleAtteinte = false;
static unsigned long tArriveeCible = 0;

void ServoLidar_Initialiser(void) {
    pinMode(PIN_SERVO, OUTPUT);
    digitalWrite(PIN_SERVO, LOW);
    angleCourant  = SERVO_ANGLE_CENTRE;
    angleCible    = SERVO_ANGLE_CENTRE;
    // Laisse le premier passage ouvrir la fenetre de maintien, le temps
    // d'amener physiquement le support au centre.
    cibleAtteinte = false;
    tArriveeCible = 0;
}

void ServoLidar_DefinirAngle(int angle) {
    angleCible = constrain(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
}

// Convertit un angle (0-180 deg) en largeur d'impulsion (us).
static int angleEnMicros(int angle) {
    return map(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX,
               SERVO_PULSE_MIN_US, SERVO_PULSE_MAX_US);
}

// PWM logiciel : avance vers la cible + rafraichit l'impulsion a 50 Hz.
// L'impulsion est mesuree avec micros() (attente active) : le scheduler
// Zephyr ne garantit pas la precision de delayMicroseconds(), ce qui
// provoque un tremblement du servo.
void ServoLidar_MettreAJour(void) {
    static unsigned long tRafraichi = 0;
    static unsigned long tDegre     = 0;

    unsigned long maintenant = millis();

    // Mouvement progressif vers la cible
    if (angleCourant != angleCible) {
        if (maintenant - tDegre >= SERVO_MS_PAR_DEGRE) {
            tDegre = maintenant;
            angleCourant += (angleCible > angleCourant) ? 1 : -1;
        }
        cibleAtteinte = false;
    } else if (!cibleAtteinte) {
        cibleAtteinte = true;
        tArriveeCible = maintenant;
    }

    // Le train d'impulsions s'arrete une fois la position tenue. A l'arret,
    // chaque impulsion regeneree porte le jitter de l'attente active et
    // redemande donc une position legerement differente : le servo vibre sur
    // place sans se deplacer. Sans signal, le reducteur maintient l'angle,
    // la charge du support etant faible.
    if (cibleAtteinte && (maintenant - tArriveeCible) >= SERVO_MAINTIEN_MS) {
        return;
    }

    // Impulsion toutes les 20 ms (50 Hz)
    if (maintenant - tRafraichi >= 20) {
        tRafraichi = maintenant;
        unsigned long largeur = (unsigned long)angleEnMicros(angleCourant);
        digitalWrite(PIN_SERVO, HIGH);
        unsigned long t0 = micros();          // apres le front : exclut la latence GPIO
        while (micros() - t0 < largeur) { }   // attente active, timing precis
        digitalWrite(PIN_SERVO, LOW);
    }
}

int ServoLidar_AngleActuel(void) { return angleCourant; }
bool ServoLidar_EstEnMouvement(void) { return angleCourant != angleCible; }
