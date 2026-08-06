#include "deplacement.h"
#include "config.h"
#include "moteurs.h"
#include "securite.h"
#include "imu.h"

// ======================== Machine a etats ========================

enum EtatMouvement { INACTIF, AVANCE, ROTATION };
static volatile EtatMouvement etatMouvement = INACTIF;

// Contexte avance
static long          encodeurDepart = 0;
static float         distanceCible  = 0.0f;
static int           sensAvance     = 1;
static float         capCumule      = 0.0f;
static unsigned long tPrecAvance    = 0;

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

// ======================== Re-emission des consignes ========================

// La consigne moteur est repetee pendant tout le mouvement : une ecriture I2C
// perdue laisserait sinon les chenilles a l'arret alors que la machine a etats
// croit avancer, jusqu'au timeout. La repetition est cadencee car le bus porte
// deja une lecture d'encodeur ou de gyroscope a chaque iteration.
static bool reemissionDue(void) {
    static unsigned long tPrecedente = 0;
    unsigned long maintenant = millis();
    if (maintenant - tPrecedente < REEMISSION_MOTEUR_MS) return false;
    tPrecedente = maintenant;
    return true;
}

// ======================== Demarrage mouvements ========================

static void demarrerAvance(float distance_m, int sens) {
    encodeurDepart  = Moteurs_LireEncodeurGauche();
    distanceCible   = fabs(distance_m);
    sensAvance      = sens;
    capCumule       = 0.0f;
    tPrecAvance     = millis();
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
        // Asservissement de cap : les deux chenilles ne convertissent pas la
        // meme consigne en la meme distance, et le vehicule derive en ligne
        // droite. On integre la derive angulaire et on la compense par un
        // differentiel ; la distance, elle, reste asservie par l'encodeur.
        unsigned long tMaintAvance = millis();
        float dtCap = (tMaintAvance - tPrecAvance) / 1000.0f;
        tPrecAvance = tMaintAvance;
        capCumule += Imu_LireGyroZ() * dtCap;

        int correction = (int)(CAP_SENS * CAP_KP * capCumule);
        correction = constrain(correction, -CAP_CORRECTION_MAX,
                                            CAP_CORRECTION_MAX);

        if (reemissionDue()) {
            Securite_DefinirVitesse(sensAvance * VITESSE_DEPLACEMENT + correction,
                                    sensAvance * VITESSE_DEPLACEMENT - correction);
        }

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

        float vitesseZ = Imu_LireGyroZ();
        angleCumule += vitesseZ * dt;

        float reste  = angleCibleAbs - fabs(angleCumule);
        bool timeout = (millis() - tDebutMouvement) > TIMEOUT_ROTATION_MS;

        // Le passage en approche lente doit prendre effet tout de suite :
        // attendre la prochaine re-emission laisserait le vehicule tourner a
        // pleine vitesse jusqu'a REEMISSION_MOTEUR_MS de plus, soit plusieurs
        // degres parcourus avant le ralentissement.
        bool changementVitesse = false;
        if (!rotationLente && reste < ROT_MARGE_LENTE_DEG) {
            rotationLente     = true;
            changementVitesse = true;
        }

        int vitesse = rotationLente ? (VITESSE_ROTATION / 2) : VITESSE_ROTATION;
        if (changementVitesse || reemissionDue()) {
            Securite_DefinirVitesse(signeRotation *  vitesse,
                                    signeRotation * -vitesse);
        }

        // La marge compense l'inertie de fin de rotation, mais elle ne doit
        // jamais depasser la cible : avec une marge de 10 degres, une consigne
        // de 10 degres serait atteinte des le premier passage, angleCumule
        // valant encore zero, et la rotation se terminerait sans avoir eu lieu.
        float marge = (angleCibleAbs > ROT_MARGE_ARRET_DEG)
                      ? ROT_MARGE_ARRET_DEG : 0.0f;

        if (fabs(angleCumule) >= (angleCibleAbs - marge) || timeout) {
            Securite_Arreter();
            etatMouvement = INACTIF;
        }
    }
}