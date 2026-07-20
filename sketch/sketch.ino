#include <Wire.h>
#include "Arduino_RouterBridge.h"
#include "config.h"
#include "moteurs.h"
#include "imu.h"
#include "deplacement.h"
#include "leds.h"
#include "comm_bridge.h"


// ======================== LEDs ========================

static void rpc_mode_led1(int mode) { Leds_DefinirMode(1, mode); }
static void rpc_mode_led2(int mode) { Leds_DefinirMode(2, mode); }

// ======================== Setup ========================

void setup() {
    Serial.begin(9600);
    Wire1.begin();
    delay(500);

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

    Moteurs_Initialiser();
    Imu_Initialiser();
    Imu_Calibrer();
    Leds_Initialiser();

    Moteurs_Arreter();
    Serial.println("[MCU] TankETS pret — Bridge actif");

}

// ======================== Loop ========================

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();
    Leds_MettreAJour();
}