#include "deplacement.h"
#include "config.h"
#include "moteurs.h"
#include "securite.h"
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
    if (etatMouvement != INACTIF || Securite_ManoeuvreEnCours()) return;
    int gauche = (int)((y - joystickX) * VITESSE_JOYSTICK);
    int droite = (int)((y + joystickX) * VITESSE_JOYSTICK);
    Securite_DefinirVitesse(gauche, droite);
}

void Deplacement_Roues(int gauche, int droite) {
    if (etatMouvement != INACTIF || Securite_ManoeuvreEnCours()) return;
    Securite_DefinirVitesse(gauche, droite);
}

// ======================== Demarrage mouvements ========================

static void demarrerAvance(float distance_m, int sens) {
    encodeurDepart  = Moteurs_LireEncodeurGauche();
    distanceCible   = fabs(distance_m);
    sensAvance      = sens;
    tDebutMouvement = millis();
    etatMouvement   = AVANCE;
    Securite_DefinirVitesse(sens * VITESSE_DEPLACEMENT,
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
    Securite_DefinirVitesse(signe *  VITESSE_ROTATION,
                            signe * -VITESSE_ROTATION);
}

// ======================== API Bridge ========================

int Deplacement_AvancerMetres(float d)  { demarrerAvance(d, +1);   return 1; }
int Deplacement_ReculerMetres(float d)  { demarrerAvance(d, -1);   return 1; }
int Deplacement_TournerGauche(float a)  { demarrerRotation(a, +1); return 1; }
int Deplacement_TournerDroite(float a)  { demarrerRotation(a, -1); return 1; }

int Deplacement_Arreter(void) {
    Securite_Arreter();
    etatMouvement = INACTIF;
    return 1;
}

int Deplacement_EstActif(void) {
    return (etatMouvement != INACTIF) ? 1 : 0;
}

int Deplacement_DirectionVirage(void) {
    if (etatMouvement != ROTATION) return VIRAGE_AUCUN;
    return (signeRotation > 0) ? VIRAGE_GAUCHE : VIRAGE_DROITE;
}

// ======================== Mise a jour ========================

void Deplacement_MettreAJour(void) {
    if (etatMouvement == INACTIF) return;

    // Le veto interrompt une avance en cours plutot que de la laisser tourner
    // a vitesse nulle jusqu'a son timeout.
    if (etatMouvement == AVANCE && sensAvance > 0 && Securite_VetoActif()) {
        Securite_Arreter();
        etatMouvement = INACTIF;
        return;
    }

    if (etatMouvement == AVANCE) {
        // Re-emission de la commande moteur : une ecriture I2C ratee est
        // rattrapee a l'iteration suivante -> plus d'avance "morte".
        Securite_DefinirVitesse(sensAvance * VITESSE_DEPLACEMENT,
                                sensAvance * VITESSE_DEPLACEMENT);

        long delta     = labs(Moteurs_LireEncodeurGauche() - encodeurDepart);
        float distance = Moteurs_PulsesEnMetres(delta);
        bool timeout   = (millis() - tDebutMouvement) > TIMEOUT_AVANCE_MS;
        if (distance >= distanceCible || timeout) {
            Securite_Arreter();
            etatMouvement = INACTIF;
        }
    }
    else if (etatMouvement == ROTATION) {
        unsigned long tMaint = millis();
        float dt = (tMaint - tPrecRotation) / 1000.0f;
        tPrecRotation = tMaint;
        if (dt > 0.05f) dt = 0.05f;   // plafond anti-saut d'integration

        float vitesseZ = Imu_LireGyroZ();
        angleCumule += vitesseZ * dt;

        float reste  = angleCibleAbs - fabs(angleCumule);
        bool timeout = (millis() - tDebutMouvement) > TIMEOUT_ROTATION_MS;
        bool tempsMiniEcoule = (millis() - tDebutMouvement) > ROT_TEMPS_MIN_MS;

        if (!rotationLente && reste < ROT_MARGE_LENTE_DEG) {
            rotationLente = true;
        }

        // Re-emission de la commande moteur a chaque iteration : si une
        // ecriture I2C vers la carte moteur est ratee, la suivante la
        // rattrape -> plus de virage "mort" bloque jusqu'au timeout.
        int vitesse = rotationLente ? (VITESSE_ROTATION / 2) : VITESSE_ROTATION;
        Securite_DefinirVitesse(signeRotation *  vitesse,
                                signeRotation * -vitesse);

        if ((tempsMiniEcoule && fabs(angleCumule) >= (angleCibleAbs - ROT_MARGE_ARRET_DEG))
            || timeout) {
            Securite_Arreter();
            etatMouvement = INACTIF;
        }
    }
}