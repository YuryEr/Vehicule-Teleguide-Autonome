#include <Wire.h>
#include "Arduino_RouterBridge.h"
#include "config.h"
#include "moteurs.h"
#include "imu.h"
#include "deplacement.h"
#include "comm_bridge.h"
#include "sonar.h"
#include "TFLunaScan.h"

// ======================== LEDs ========================

static void rpc_mode_led1(int mode) { /* TODO: hardware LED */ }
static void rpc_mode_led2(int mode) { /* TODO: hardware LED */ }

// ======================== Capteurs de distance ========================
// Dernieres mesures mises en cache. Elles sont rafraichies dans loop()
// SANS bloquer la boucle temps reel ni la machine a etats des moteurs.
//   -1  = mesure invalide / pas encore lue
//   >=0 = distance en cm (SONAR_DISTANCE_MAX => voie degagee)
static int distance_sonar_cm = -1;   // sonar HC-SR04 (frontal)
static int distance_lidar_cm = -1;   // TF-Luna, droit devant

// RPC exposees au MPU (Python) via le Bridge.
static int rpc_lire_sonar_cm(void) { return distance_sonar_cm; }
static int rpc_lire_lidar_cm(void) { return distance_lidar_cm; }

// Rafraichit une seule mesure par passage, cadencee a ~10 Hz.
// On alterne sonar / lidar pour etaler la charge et eviter les a-coups.
static void Capteurs_MettreAJour(void) {
    static unsigned long t_precedent = 0;
    static uint8_t tour = 0;

    unsigned long maintenant = millis();
    if (maintenant - t_precedent < 100) return;   // throttle 100 ms
    t_precedent = maintenant;

    if (tour == 0) {
        // Sonar : un seul tir rapide (pas de mediane => non bloquant).
        distance_sonar_cm = GetDistanceUneFoiseCm();
    } else {
        // TF-Luna : lecture I2C fraiche, servo laisse au centre.
        float d; uint16_t force; float tempC;
        if (TfLunaLireI2C(d, force, tempC) && LectureValide(d, force)) {
            distance_lidar_cm = (int)d;
        } else {
            distance_lidar_cm = -1;
        }
    }
    tour ^= 1;
}

// ======================== Setup ========================

void setup() {
    Serial.begin(9600);
    Wire1.begin();
    delay(500);

    Moteurs_Initialiser();
    Imu_Initialiser();
    Imu_Calibrer();

    // --- Capteurs de distance ---
    sonar_init();
    // Servo du lidar positionne "droit devant". Le balayage complet
    // (BalayageEnvironnement) reste disponible pour un futur mode scan,
    // mais la boucle principale se contente d'une lecture frontale.
    lidarServo.attach(SERVO_PIN);
    lidarServo.write(SERVO_CENTRE);
    courantAngleServoDeg = SERVO_CENTRE;

    CommBridge_Initialiser();

    Bridge.provide_safe("joy_x",              Deplacement_JoystickX);
    Bridge.provide_safe("joy_y",              Deplacement_JoystickY);
    Bridge.provide_safe("avancer_metres",     Deplacement_AvancerMetres);
    Bridge.provide_safe("reculer_metres",     Deplacement_ReculerMetres);
    Bridge.provide_safe("tourner_gauche_deg", Deplacement_TournerGauche);
    Bridge.provide_safe("tourner_droite_deg", Deplacement_TournerDroite);
    Bridge.provide_safe("arreter_mouvement",  Deplacement_Arreter);
    Bridge.provide_safe("mouvement_actif",    Deplacement_EstActif);
    Bridge.provide_safe("mode_led1",          rpc_mode_led1);
    Bridge.provide_safe("mode_led2",          rpc_mode_led2);
    Bridge.provide_safe("lire_sonar_cm",      rpc_lire_sonar_cm);
    Bridge.provide_safe("lire_lidar_cm",      rpc_lire_lidar_cm);

    Moteurs_Arreter();
    Serial.println("[MCU] TankETS pret — Bridge actif");
}

// ======================== Loop ========================

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();
    Capteurs_MettreAJour();
}
