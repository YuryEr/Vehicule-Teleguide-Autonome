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
    float avance   = y         * VITESSE_MANUEL;
    float rotation = joystickX * VITESSE_ROTATION_MANUEL;
    Securite_DefinirVitesse((int)(avance - rotation),
                            (int)(avance + rotation));
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

// ======================== Compensation de derive ========================

// Repartit le decalage fractionnaire sur les re-emissions successives par
// accumulation : l'unite entiere n'est appliquee que lorsque l'accumulateur
// franchit le denominateur, ce qui etale la correction au lieu de la grouper
// en salves. A n'appeler qu'au moment d'une re-emission.
static int trimAvance(void) {
    static int accumulateur = 0;
    if (AVANCE_TRIM_NUM == 0) return 0;

    accumulateur += (AVANCE_TRIM_NUM > 0) ? AVANCE_TRIM_NUM : -AVANCE_TRIM_NUM;
    if (accumulateur < AVANCE_TRIM_DEN) return 0;

    accumulateur -= AVANCE_TRIM_DEN;
    return (AVANCE_TRIM_NUM > 0) ? 1 : -1;
}

// Le decalage porte sur une seule chenille : l'ajouter d'un cote et le
// retrancher de l'autre doublerait le differentiel pour rien.
static void consignesAvance(int sens, int *gauche, int *droite) {
    int base = sens * VITESSE_DEPLACEMENT;
    int trim = trimAvance();
    *gauche  = base + ((trim > 0) ? sens : 0);
    *droite  = base + ((trim < 0) ? sens : 0);
}

// ======================== Demarrage mouvements ========================

static void demarrerAvance(float distance_m, int sens) {
    encodeurDepart  = Moteurs_LireEncodeurGauche();
    distanceCible   = fabs(distance_m);
    sensAvance      = sens;
    tDebutMouvement = millis();
    etatMouvement   = AVANCE;

    int gauche, droite;
    consignesAvance(sens, &gauche, &droite);
    Securite_DefinirVitesse(gauche, droite);
}

static void demarrerRotation(float angle_deg, int signe) {
    Imu_CalibrerRapide();

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
        if (reemissionDue()) {
            int gauche, droite;
            consignesAvance(sensAvance, &gauche, &droite);
            Securite_DefinirVitesse(gauche, droite);
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
