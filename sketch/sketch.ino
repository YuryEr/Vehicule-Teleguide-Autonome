#include <Wire.h>
#include "Arduino_RouterBridge.h"
#include "config.h"
#include "moteurs.h"
#include "imu.h"
#include "deplacement.h"
#include "comm_bridge.h"
#include "leds.h" 

// ======================== LEDs ========================

static void rpc_mode_bandeaux(int mode) { Leds_DefinirModeBandeaux(mode); }
static void rpc_mode_phares(int actif)  { Leds_DefinirPhares(actif); }

// ======================== Setup ========================

void setup() {
    Serial.begin(9600);
    Wire1.begin();
  
    delay(500);

    Moteurs_Initialiser();
    Leds_Initialiser();
    //Imu_Initialiser();
    //Imu_Calibrer();
    CommBridge_Initialiser();

    Bridge.provide_safe("joy_x",              Deplacement_JoystickX);
    Bridge.provide_safe("joy_y",              Deplacement_JoystickY);
    Bridge.provide_safe("avancer_metres",     Deplacement_AvancerMetres);
    Bridge.provide_safe("reculer_metres",     Deplacement_ReculerMetres);
    Bridge.provide_safe("tourner_gauche_deg", Deplacement_TournerGauche);
    Bridge.provide_safe("tourner_droite_deg", Deplacement_TournerDroite);
    Bridge.provide_safe("arreter_mouvement",  Deplacement_Arreter);
    Bridge.provide_safe("mouvement_actif",    Deplacement_EstActif);
    Bridge.provide_safe("mode_bandeaux", rpc_mode_bandeaux);
    Bridge.provide_safe("mode_phares",   rpc_mode_phares);

    Moteurs_Arreter();
    Serial.println("[MCU] TankETS pret — Bridge actif");
}

// ======================== Loop ========================

void loop()
{
    Bridge.update();
    Leds_MettreAJour();
    Deplacement_MettreAJour();
  
}