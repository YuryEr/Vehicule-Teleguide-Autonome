#include <Wire.h>
#include "Arduino_RouterBridge.h"
#include "comm_bridge.h"

// ======================== Carte moteur Hiwonder ========================
#define MOTOR_ADDR 0x34
#define REG_MOTOR1 0x01
#define REG_MOTOR2 0x02
#define REG_MOTOR3 0x03
#define REG_MOTOR4 0x04

// ======================== Etat joystick ========================
static float joyX = 0.0;
static float joyY = 0.0;

// ======================== Etat machine a etats ========================
enum EtatDeplacement { INACTIF, AVANCE, ROTATION };
static volatile EtatDeplacement etatMouvement = INACTIF;

// ======================== LEDs ========================
static int modeLed1 = 0;
static int modeLed2 = 0;

// ======================== Helpers moteur ========================

void setMoteur(uint8_t reg, int16_t vitesse) {
    vitesse = constrain(vitesse, -100, 100);
    Wire1.beginTransmission(MOTOR_ADDR);
    Wire1.write(reg);
    Wire1.write((uint8_t)((vitesse >= 0) ? 0 : 1));
    Wire1.write((uint8_t)abs(vitesse));
    Wire1.endTransmission();
}

void arreterMoteurs() {
    setMoteur(REG_MOTOR1, 0);
    setMoteur(REG_MOTOR2, 0);
    setMoteur(REG_MOTOR3, 0);
    setMoteur(REG_MOTOR4, 0);
}

// ======================== Handlers RPC (Python -> MCU) ========================

static void rpc_joy_x(float val) { joyX = val; }
static void rpc_joy_y(float val) { joyY = val; }

static void rpc_avancer_metres(float dist) {
    etatMouvement = AVANCE;
    int vit = 30;
    setMoteur(REG_MOTOR1, vit);
    setMoteur(REG_MOTOR2, vit);
    setMoteur(REG_MOTOR3, vit);
    setMoteur(REG_MOTOR4, vit);
    // TODO: asservissement encodeurs (PR #2)
}

static void rpc_reculer_metres(float dist) {
    etatMouvement = AVANCE;
    int vit = -30;
    setMoteur(REG_MOTOR1, vit);
    setMoteur(REG_MOTOR2, vit);
    setMoteur(REG_MOTOR3, vit);
    setMoteur(REG_MOTOR4, vit);
}

static void rpc_tourner_gauche_deg(float angle) {
    etatMouvement = ROTATION;
    int vit = 25;
    setMoteur(REG_MOTOR1, -vit);
    setMoteur(REG_MOTOR2, -vit);
    setMoteur(REG_MOTOR3, vit);
    setMoteur(REG_MOTOR4, vit);
}

static void rpc_tourner_droite_deg(float angle) {
    etatMouvement = ROTATION;
    int vit = 25;
    setMoteur(REG_MOTOR1, vit);
    setMoteur(REG_MOTOR2, vit);
    setMoteur(REG_MOTOR3, -vit);
    setMoteur(REG_MOTOR4, -vit);
}

static void rpc_arreter_mouvement() {
    etatMouvement = INACTIF;
    arreterMoteurs();
}

static int rpc_mouvement_actif() {
    return (etatMouvement != INACTIF) ? 1 : 0;
}

static void rpc_mode_led1(int mode) { modeLed1 = mode; }
static void rpc_mode_led2(int mode) { modeLed2 = mode; }

// ======================== Setup ========================

void setup() {
    Serial.begin(9600);
    Wire1.begin();

    // Bridge + vision handlers
    CommBridge_Initialiser();

    // Pilotage manuel
    Bridge.provide_safe("joy_x", rpc_joy_x);
    Bridge.provide_safe("joy_y", rpc_joy_y);

    // Deplacements asservis
    Bridge.provide_safe("avancer_metres", rpc_avancer_metres);
    Bridge.provide_safe("reculer_metres", rpc_reculer_metres);
    Bridge.provide_safe("tourner_gauche_deg", rpc_tourner_gauche_deg);
    Bridge.provide_safe("tourner_droite_deg", rpc_tourner_droite_deg);
    Bridge.provide_safe("arreter_mouvement", rpc_arreter_mouvement);
    Bridge.provide_safe("mouvement_actif", rpc_mouvement_actif);

    // LEDs
    Bridge.provide_safe("mode_led1", rpc_mode_led1);
    Bridge.provide_safe("mode_led2", rpc_mode_led2);

    arreterMoteurs();
    Serial.println("[MCU] TankETS pret — Bridge actif");
}

// ======================== Loop ========================

void loop() {
    // Pilotage manuel par joystick
    if (etatMouvement == INACTIF) {
        int gauche = (int)((joyY + joyX) * 30.0);
        int droite = (int)((joyY - joyX) * 30.0);
        setMoteur(REG_MOTOR1, gauche);
        setMoteur(REG_MOTOR2, gauche);
        setMoteur(REG_MOTOR3, droite);
        setMoteur(REG_MOTOR4, droite);
    }

    delay(20);
}