#include "obstacle.h"
#include "config.h"
#include "ultrason.h"
#include "lidar.h"
#include "servo_lidar.h"

// ======================== Secteurs ========================

// Angle servo vise pour chaque secteur. OBSTACLE_SENS_SERVO inverse
// gauche et droite selon l'orientation du support sur le chassis.
static const int anglesSecteurs[NB_SECTEURS] = {
    SERVO_ANGLE_CENTRE - OBSTACLE_SENS_SERVO * OBSTACLE_ECART_SONDAGE_DEG,
    SERVO_ANGLE_CENTRE,
    SERVO_ANGLE_CENTRE + OBSTACLE_SENS_SERVO * OBSTACLE_ECART_SONDAGE_DEG,
};

static int distancesSecteurs[NB_SECTEURS] = { -1, -1, -1 };

// ======================== Etat ========================

enum EtatSondage { INACTIF, DEPLACEMENT, STABILISATION, RETOUR };

static EtatSondage   etat             = INACTIF;
static int           secteurCourant   = 0;
static unsigned long tArrivee         = 0;
static unsigned long tRetourAxe       = 0;
static int           distanceFrontale = ULTRASON_DISTANCE_MAX;

static bool          sondageAuto      = true;
static bool          detectePrecedent = false;
static unsigned long tDernierSondage  = 0;

// ======================== Distance frontale ========================

// Ramene une mesure au pare-choc avant, reference commune aux deux capteurs.
// Sans cette correction, la fusion comparerait deux distances prises depuis
// deux origines differentes.
static int ramenerAuPareChoc(int distanceCm, int reculCm) {
    int corrigee = distanceCm - reculCm;
    return (corrigee < 0) ? 0 : corrigee;
}

// Retient la plus petite des mesures valides. L'ultrason couvre un cone
// large et voit de pres ; le LiDAR est precis mais n'eclaire que deux
// degres et sa mesure est rejetee sous LIDAR_DISTANCE_MIN. Les deux se
// completent : une cible vue par un seul d'entre eux suffit a la signaler.
static void evaluerFrontal(void) {
    int retenue = ULTRASON_DISTANCE_MAX;

    int ultrason = Ultrason_DistanceCm();
    if (Ultrason_EstPresent() && ultrason > 0) {
        int corrigee = ramenerAuPareChoc(ultrason, ULTRASON_RECUL_CM);
        if (corrigee < retenue) retenue = corrigee;
    }

    // Pendant un sondage le servo quitte l'axe : la mesure LiDAR ne decrit
    // plus l'avant du vehicule. Au retour au centre, le cache peut encore
    // contenir une mesure laterale pendant une periode de rafraichissement.
    if (etat == INACTIF
        && (millis() - tRetourAxe) >= LIDAR_PERIODE_MS) {
        int lidar = Lidar_DistanceCm();
        if (lidar > 0) {
            int corrigee = ramenerAuPareChoc(lidar, LIDAR_RECUL_CM);
            if (corrigee < retenue) retenue = corrigee;
        }
    }

    distanceFrontale = retenue;
}

// ======================== Sondage ========================

static void viserSecteurCourant(void) {
    ServoLidar_DefinirAngle(anglesSecteurs[secteurCourant]);
    etat = DEPLACEMENT;
}

// Une direction sans echo exploitable est traitee comme degagee : a 45
// degres de l'axe, l'absence de retour signifie le plus souvent qu'aucune
// surface ne se trouve dans la portee du faisceau.
static int degagementSecteur(int secteur) {
    int distance = distancesSecteurs[secteur];
    return (distance < 0) ? LIDAR_DISTANCE_MAX : distance;
}

// ======================== API ========================

void Obstacle_Initialiser(void) {
    for (int i = 0; i < NB_SECTEURS; i++) distancesSecteurs[i] = -1;
    etat             = INACTIF;
    secteurCourant   = 0;
    distanceFrontale = ULTRASON_DISTANCE_MAX;
    ServoLidar_DefinirAngle(SERVO_ANGLE_CENTRE);
}

void Obstacle_MettreAJour(void) {
    static unsigned long tPrecedent = 0;
    unsigned long maintenant = millis();

    if (maintenant - tPrecedent >= OBSTACLE_PERIODE_MS) {
        tPrecedent = maintenant;
        evaluerFrontal();
    }

    // Front montant de la detection : le LiDAR part chercher de quel cote
    // contourner. La temporisation empeche une mesure instable autour du
    // seuil de relancer le servo en continu.
    bool detecte = Obstacle_EstDetecte();
    if (sondageAuto && detecte && !detectePrecedent
        && (maintenant - tDernierSondage) >= OBSTACLE_RELANCE_MIN_MS) {
        tDernierSondage = maintenant;
        Obstacle_LancerSondage();
    }
    detectePrecedent = detecte;

    switch (etat) {

        case INACTIF:
            break;

        case DEPLACEMENT:
            if (!ServoLidar_EstEnMouvement()) {
                tArrivee = maintenant;
                etat     = STABILISATION;
            }
            break;

        case STABILISATION:
            // Le SG90 oscille brievement autour de sa consigne : mesurer
            // trop tot associerait la distance au mauvais secteur.
            if (maintenant - tArrivee >= OBSTACLE_STABILISATION_MS) {
                int distCm;
                distancesSecteurs[secteurCourant] =
                    Lidar_MesurerMaintenant(distCm) ? distCm : -1;

                secteurCourant++;
                if (secteurCourant < NB_SECTEURS) {
                    viserSecteurCourant();
                } else {
                    ServoLidar_DefinirAngle(SERVO_ANGLE_CENTRE);
                    etat = RETOUR;
                }
            }
            break;

        case RETOUR:
            if (!ServoLidar_EstEnMouvement()) {
                tRetourAxe = maintenant;
                etat       = INACTIF;
            }
            break;
    }
}

int  Obstacle_DistanceFrontaleCm(void) { return distanceFrontale; }
bool Obstacle_EstDetecte(void) { return distanceFrontale <= OBSTACLE_SEUIL_CM; }
bool Obstacle_SondageEnCours(void) { return etat != INACTIF; }

void Obstacle_LancerSondage(void) {
    if (etat != INACTIF || !Lidar_EstPresent()) return;

    for (int i = 0; i < NB_SECTEURS; i++) distancesSecteurs[i] = -1;
    secteurCourant = 0;
    viserSecteurCourant();
}

int Obstacle_DistanceSecteur(int secteur) {
    if (secteur < 0 || secteur >= NB_SECTEURS) return -1;
    return distancesSecteurs[secteur];
}

void Obstacle_DefinirSondageAuto(bool actif) { sondageAuto = actif; }

int Obstacle_CoteLePlusDegage(void) {
    return (degagementSecteur(SECTEUR_GAUCHE)
            >= degagementSecteur(SECTEUR_DROITE))
           ? SECTEUR_GAUCHE : SECTEUR_DROITE;
}
