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
#include "obstacle.h"
#include "test_capteurs.h"


// ======================== LEDs ========================

static void rpc_mode_led1(int mode) { Leds_DefinirMode(1, mode); }
static void rpc_mode_led2(int mode) { Leds_DefinirMode(2, mode); }

// ======================== Capteurs de distance ========================

static int rpc_lire_ultrason_cm(void) { return Ultrason_DistanceCm(); }
static int rpc_lire_lidar_cm(void)    { return Lidar_DistanceCm(); }

// ======================== ServoMoteur ========================

static void rpc_servo_angle(int angle) { ServoLidar_DefinirAngle(angle); }

// ======================== Detection d'obstacles ========================

static int rpc_obstacle_frontal_cm(void) { return Obstacle_DistanceFrontaleCm(); }
static int rpc_obstacle_detecte(void)    { return Obstacle_EstDetecte() ? 1 : 0; }
static int rpc_lancer_sondage(void)      { Obstacle_LancerSondage(); return 1; }

// 0 : sondage en cours, resultat pas encore disponible. Le Bridge ne
// transmet pas de valeur negative, d'ou l'encodage 1 = gauche, 2 = droite.
static int rpc_cote_degage(void) {
    if (Obstacle_SondageEnCours()) return 0;
    return (Obstacle_CoteLePlusDegage() == SECTEUR_GAUCHE) ? 1 : 2;
}

// ======================== Setup ========================

void setup() {

    Serial.begin(9600);
    Wire1.begin();
    delay(500);

    // Bridge en premier : le controle survit a un capteur defaillant
    CommBridge_Initialiser();
    Bridge.provide_safe("joy_x",               Deplacement_JoystickX);
    Bridge.provide_safe("joy_y",               Deplacement_JoystickY);
    Bridge.provide_safe("roues",               Deplacement_Roues);
    Bridge.provide_safe("avancer_metres",      Deplacement_AvancerMetres);
    Bridge.provide_safe("reculer_metres",      Deplacement_ReculerMetres);
    Bridge.provide_safe("tourner_gauche_deg",  Deplacement_TournerGauche);
    Bridge.provide_safe("tourner_droite_deg",  Deplacement_TournerDroite);
    Bridge.provide_safe("arreter_mouvement",   Deplacement_Arreter);
    Bridge.provide_safe("mouvement_actif",     Deplacement_EstActif);
    Bridge.provide_safe("mode_led1",           rpc_mode_led1);
    Bridge.provide_safe("mode_led2",           rpc_mode_led2);
    Bridge.provide_safe("lire_ultrason_cm",    rpc_lire_ultrason_cm);
    Bridge.provide_safe("lire_lidar_cm",       rpc_lire_lidar_cm);
    Bridge.provide_safe("servo_angle",         rpc_servo_angle);
    Bridge.provide_safe("obstacle_frontal_cm", rpc_obstacle_frontal_cm);
    Bridge.provide_safe("obstacle_detecte",    rpc_obstacle_detecte);
    Bridge.provide_safe("lancer_sondage",      rpc_lancer_sondage);
    Bridge.provide_safe("cote_degage",         rpc_cote_degage);

    // Sonde le bus une fois : les modules dont le peripherique est absent
    // resteront inertes au lieu de bloquer la boucle sur des timeouts I2C.
    BusI2C_Scanner();

    Moteurs_Initialiser();
    Imu_Initialiser();
    Imu_Calibrer();
    Leds_Initialiser();
    Ecran_Initialiser();
    Ultrason_Initialiser();
    Lidar_Initialiser();
    ServoLidar_Initialiser();
    Obstacle_Initialiser();

    Moteurs_Arreter();
}

// ======================== Loop ========================

// Bilan de demarrage, emis depuis loop() : le moniteur serie s'attache
// apres l'execution de setup().
static void tracerDemarrage(void) {
    static bool tracee = false;
    if (tracee || millis() < 3000) return;
    tracee = true;

    BusI2C_Tracer();
    Serial.println("[MCU] TankETS pret — Bridge actif");
}

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();
    Leds_MettreAJour();
    Ultrason_MettreAJour();
    Lidar_MettreAJour();
    ServoLidar_MettreAJour();
    Obstacle_MettreAJour();
    tracerDemarrage();
    TestCapteurs_MettreAJour();
}