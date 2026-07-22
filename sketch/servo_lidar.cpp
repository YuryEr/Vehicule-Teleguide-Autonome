#include "servo_lidar.h"
#include "config.h"
#include <Servo.h>

static Servo servo;
static int angleCourant = SERVO_ANGLE_CENTRE;
static int angleCible   = SERVO_ANGLE_CENTRE;

void ServoLidar_Initialiser(void) {
    servo.attach(PIN_SERVO);
    angleCourant = SERVO_ANGLE_CENTRE;
    angleCible   = SERVO_ANGLE_CENTRE;
    servo.write(angleCourant);
}

void ServoLidar_DefinirAngle(int angle) {
    angleCible = constrain(angle, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX);
}

// Avance d'un degre par tranche de SERVO_MS_PAR_DEGRE : mouvement
// fluide sans jamais bloquer la boucle principale.
void ServoLidar_MettreAJour(void) {
    static unsigned long tPrecedent = 0;

    if (angleCourant == angleCible) return;

    unsigned long maintenant = millis();
    if (maintenant - tPrecedent < SERVO_MS_PAR_DEGRE) return;
    tPrecedent = maintenant;

    angleCourant += (angleCible > angleCourant) ? 1 : -1;
    servo.write(angleCourant);
}

int ServoLidar_AngleActuel(void) {
    return angleCourant;
}

bool ServoLidar_EstEnMouvement(void) {
    return (angleCourant != angleCible);
}