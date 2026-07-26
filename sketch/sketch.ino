#include <Wire.h>
#include "Arduino_RouterBridge.h"
#include "config.h"
#include "bus_i2c.h"
#include "moteurs.h"
#include "imu.h"
#include "deplacement.h"
#include "leds.h"
#include "ecran.h"
#include "ultrason.h"
#include "lidar.h"
#include "comm_bridge.h"
#include "servo_lidar.h"


// ======================== LEDs ========================

static void rpc_mode_led1(int mode) { Leds_DefinirMode(1, mode); }
static void rpc_mode_led2(int mode) { Leds_DefinirMode(2, mode); }

// ======================== Capteurs de distance ========================

static int rpc_lire_ultrason_cm(void) { return Ultrason_DistanceCm(); }
static int rpc_lire_lidar_cm(void)    { return Lidar_DistanceCm(); }

// ======================== ServoMoteur ========================

static void rpc_servo_angle(int angle) { ServoLidar_DefinirAngle(angle); }

// ======================== Setup ========================

static unsigned long dureeBridgeMs = 0;
static unsigned long dureeSetupMs  = 0;

// Chronometrage des initialisations, affiche par tracerDemarrage().
#define NB_ETAPES_INIT  8

static const char *nomEtape[NB_ETAPES_INIT] = {
    "Moteurs", "Imu", "ImuCalibrer", "Leds",
    "Ecran", "Ultrason", "Lidar", "ServoLidar"
};
static unsigned long dureeEtape[NB_ETAPES_INIT] = { 0 };

void setup() {

    Serial.begin(9600);
    Wire1.begin();
    delay(500);

    unsigned long tDebut = millis();

    // Bridge en premier : le controle survit a un capteur defaillant
    CommBridge_Initialiser();
    Bridge.provide_safe("joy_x",              Deplacement_JoystickX);
    Bridge.provide_safe("joy_y",              Deplacement_JoystickY);
    Bridge.provide_safe("roues",              Deplacement_Roues);
    Bridge.provide_safe("avancer_metres",     Deplacement_AvancerMetres);
    Bridge.provide_safe("reculer_metres",     Deplacement_ReculerMetres);
    Bridge.provide_safe("tourner_gauche_deg", Deplacement_TournerGauche);
    Bridge.provide_safe("tourner_droite_deg", Deplacement_TournerDroite);
    Bridge.provide_safe("arreter_mouvement",  Deplacement_Arreter);
    Bridge.provide_safe("mouvement_actif",    Deplacement_EstActif);
    Bridge.provide_safe("mode_led1",          rpc_mode_led1);
    Bridge.provide_safe("mode_led2",          rpc_mode_led2);
    Bridge.provide_safe("lire_ultrason_cm",   rpc_lire_ultrason_cm);
    Bridge.provide_safe("lire_lidar_cm",      rpc_lire_lidar_cm);
    Bridge.provide_safe("servo_angle",        rpc_servo_angle);

    dureeBridgeMs = millis() - tDebut;

    // Sonde le bus une fois : les modules dont le peripherique est absent
    // resteront inertes au lieu de bloquer la boucle sur des timeouts I2C.
    BusI2C_Scanner();

    unsigned long tEtape = millis();
    Moteurs_Initialiser();     dureeEtape[0] = millis() - tEtape; tEtape = millis();
    Imu_Initialiser();         dureeEtape[1] = millis() - tEtape; tEtape = millis();
    Imu_Calibrer();            dureeEtape[2] = millis() - tEtape; tEtape = millis();
    Leds_Initialiser();        dureeEtape[3] = millis() - tEtape; tEtape = millis();
    Ecran_Initialiser();       dureeEtape[4] = millis() - tEtape; tEtape = millis();
    Ultrason_Initialiser();    dureeEtape[5] = millis() - tEtape; tEtape = millis();
    Lidar_Initialiser();       dureeEtape[6] = millis() - tEtape; tEtape = millis();
    ServoLidar_Initialiser();  dureeEtape[7] = millis() - tEtape;

    Moteurs_Arreter();
    dureeSetupMs = millis() - tDebut;
}

// ======================== Loop ========================

// Bilan de demarrage, emis depuis loop() : le moniteur serie s'attache
// apres l'execution de setup().
static void tracerDemarrage(void) {
    static bool tracee = false;
    if (tracee || millis() < 3000) return;
    tracee = true;

    BusI2C_Tracer();
    for (uint8_t i = 0; i < NB_ETAPES_INIT; i++) {
        Serial.print("[init] ");
        Serial.print(nomEtape[i]);
        Serial.print(" : ");
        Serial.print(dureeEtape[i]);
        Serial.println(" ms");
    }
    Serial.print("[MCU] Bridge etabli apres ");
    Serial.print(dureeBridgeMs);
    Serial.println(" ms");
    Serial.print("[MCU] Setup complet apres ");
    Serial.print(dureeSetupMs);
    Serial.println(" ms");
    Serial.println("[MCU] TankETS pret — Bridge actif");
}

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();
    Leds_MettreAJour();
    Ultrason_MettreAJour();
    Lidar_MettreAJour();
    ServoLidar_MettreAJour();
    tracerDemarrage();

    // ===== TEST SERVO TEMPORAIRE (a retirer apres validation) =====
    static unsigned long tServo = 0;
    static bool versMax = true;
    if (millis() - tServo > 2000) {
        tServo = millis();
        ServoLidar_DefinirAngle(versMax ? 180 : 0);
        versMax = !versMax;
    }
}