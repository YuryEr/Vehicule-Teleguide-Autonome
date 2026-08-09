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
#include "securite.h"
#include "evitement.h"
#include "test_capteurs.h"


// ======================== LEDs ========================

static void rpc_mode_bandeaux(int mode) { Leds_DefinirModeBandeaux(mode); }
static void rpc_mode_phares(int actif)  { Leds_DefinirPhares(actif); }

// ======================== Mode de conduite ========================

static void rpc_definir_mode(int mode) { Securite_DefinirMode(mode); }
static int  rpc_veto_actif(void)       { return Securite_VetoActif() ? 1 : 0; }

// 0 : rien, 1 : obstacle, 2 : feu. Le Bridge ne transmettant pas de valeur
// negative, l'absence de cause est codee par zero.
static int rpc_cause_arret(void) {
    if (Securite_ObstacleBloquant()) return 1;
    if (Securite_FeuBloquant())      return 2;
    return 0;
}

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
    Bridge.provide_safe("definir_mode",        rpc_definir_mode);
    Bridge.provide_safe("veto_actif",          rpc_veto_actif);
    Bridge.provide_safe("cause_arret",         rpc_cause_arret);
    Bridge.provide_safe("mode_bandeaux",       rpc_mode_bandeaux);
    Bridge.provide_safe("mode_phares",         rpc_mode_phares);
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
    // La page de connexion attend l'adresse IP du MPU : elle depend du
    // reseau rejoint, que le MCU n'a aucun moyen de connaitre.
    Ecran_AfficherAttente();
    Ultrason_Initialiser();
    Lidar_Initialiser();
    ServoLidar_Initialiser();
    Obstacle_Initialiser();
    Securite_Initialiser();
    Evitement_Initialiser();

    Securite_Arreter();
}

// ======================== Loop ========================

// Bilan de demarrage, emis depuis loop() : le moniteur serie s'attache
// apres l'execution de setup().
static void tracerDemarrage(void) {
    static bool tracee = false;
    if (tracee || millis() < 3000) return;
    tracee = true;

    BusI2C_Tracer();
    Serial.println("[MCU] VTA pret, Bridge actif");
}

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();

    // Les LEDs affichent, elles n'interrogent pas le deplacement : la
    // direction du clignotant leur est fournie ici.
    Leds_DefinirVirage(Deplacement_DirectionVirage());
    Leds_MettreAJour();
    Ecran_MettreAJour();
    Ultrason_MettreAJour();
    Lidar_MettreAJour();
    ServoLidar_MettreAJour();
    Obstacle_MettreAJour();
    Evitement_MettreAJour();
    tracerDemarrage();
    TestCapteurs_MettreAJour();
}
