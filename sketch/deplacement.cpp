#include "deplacement.h"
#include "config.h"
#include "moteurs.h"
#include "imu.h"

// ======================== Machine a etats ========================

enum EtatMouvement { INACTIF, AVANCE, ROTATION };
static volatile EtatMouvement etatMouvement = INACTIF;

// Contexte avance
static long  encodeurDepart = 0;
static float distanceCible  = 0.0f;
static int   sensAvance     = 1;

// Contexte rotation
static float         angleCumule   = 0.0f;
static float         angleCibleAbs = 0.0f;
static int           signeRotation = 1;
static bool          rotationLente = false;
static unsigned long tPrecRotation = 0;

// Commun
static unsigned long tDebutMouvement = 0;

// ======================== Joystick ========================

static volatile float joystickX = 0.0f;

void Deplacement_JoystickX(float x) { joystickX = x; }

void Deplacement_JoystickY(float y) {
    if (etatMouvement != INACTIF) return;
    int gauche = (int)((y - joystickX) * VITESSE_JOYSTICK);
    int droite = (int)((y + joystickX) * VITESSE_JOYSTICK);
    Moteurs_DefinirVitesse(gauche, droite);
}

// ======================== Demarrage mouvements ========================

static void demarrerAvance(float distance_m, int sens) {
    encodeurDepart  = Moteurs_LireEncodeurGauche();
    distanceCible   = fabs(distance_m);
    sensAvance      = sens;
    tDebutMouvement = millis();
    etatMouvement   = AVANCE;
    Moteurs_DefinirVitesse(sens * VITESSE_DEPLACEMENT,
                           sens * VITESSE_DEPLACEMENT);
}

static void demarrerRotation(float angle_deg, int signe) {
    angleCumule     = 0.0f;
    angleCibleAbs   = fabs(angle_deg);
    signeRotation   = signe;
    rotationLente   = false;
    tPrecRotation   = millis();
    tDebutMouvement = millis();
    etatMouvement   = ROTATION;
    Moteurs_DefinirVitesse(signe *  VITESSE_ROTATION,
                           signe * -VITESSE_ROTATION);
}

// ======================== API Bridge ========================

int Deplacement_AvancerMetres(float d)  { demarrerAvance(d, +1);   return 1; }
int Deplacement_ReculerMetres(float d)  { demarrerAvance(d, -1);   return 1; }
int Deplacement_TournerGauche(float a)  { demarrerRotation(a, +1); return 1; }
int Deplacement_TournerDroite(float a)  { demarrerRotation(a, -1); return 1; }

int Deplacement_Arreter(void) {
    Moteurs_Arreter();
    etatMouvement = INACTIF;
    return 1;
}

int Deplacement_EstActif(void) {
    return (etatMouvement != INACTIF) ? 1 : 0;
}

// ======================== Mise a jour ========================

void Deplacement_MettreAJour(void) {
    if (etatMouvement == INACTIF) return;

    if (etatMouvement == AVANCE) {
        long delta     = labs(Moteurs_LireEncodeurGauche() - encodeurDepart);
        float distance = Moteurs_PulsesEnMetres(delta);
        bool timeout   = (millis() - tDebutMouvement) > TIMEOUT_AVANCE_MS;
        if (distance >= distanceCible || timeout) {
            Moteurs_Arreter();
            etatMouvement = INACTIF;
        }
    }
    else if (etatMouvement == ROTATION) {
        unsigned long tMaint = millis();
        float dt = (tMaint - tPrecRotation) / 1000.0f;
        tPrecRotation = tMaint;

        float vitesseZ = Imu_LireGyroZ();
        angleCumule += vitesseZ * dt;

        float reste  = angleCibleAbs - fabs(angleCumule);
        bool timeout = (millis() - tDebutMouvement) > TIMEOUT_ROTATION_MS;

        if (!rotationLente && reste < ROT_MARGE_LENTE_DEG) {
            rotationLente = true;
            int lente = VITESSE_ROTATION / 2;
            Moteurs_DefinirVitesse(signeRotation *  lente,
                                   signeRotation * -lente);
        }
        if (fabs(angleCumule) >= (angleCibleAbs - ROT_MARGE_ARRET_DEG)
            || timeout) {
            Moteurs_Arreter();
            etatMouvement = INACTIF;
        }
    }
}