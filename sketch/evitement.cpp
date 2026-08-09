#include "evitement.h"
#include "config.h"
#include "obstacle.h"
#include "deplacement.h"
#include "securite.h"

// ======================== Machine a etats ========================

// Chaque etape est precedee d'une immobilisation : le vehicule s'arrete, puis
// agit. Sans cet arret explicite, les chenilles gardent la derniere consigne du
// suivi de ligne et la manoeuvre s'enchaine en roulant.
enum EtatEvitement {
    REPOS,
    PAUSE,
    SONDAGE,
    ROTATION_ALLER,
    AVANCE_DIAGONALE,
    ROTATION_PARALLELE,
    AVANCE_PARALLELE,
    ROTATION_RETOUR
};

static EtatEvitement etat          = REPOS;
static EtatEvitement etapeSuivante = REPOS;
static unsigned long tPause        = 0;
static int           coteChoisi    = SECTEUR_GAUCHE;
static int           angleTourne   = 0;
static int           essais        = 0;
static bool          abandonne     = false;

// ======================== Rotations ========================

// Pivote a l'oppose de la ligne, vers le cote retenu par le sondage, et cumule
// l'angle parcouru. Le cumul importe car la voie peut rester bloquee apres une
// premiere rotation : un cran supplementaire est alors ajoute, et la remise
// parallele doit rendre le total, pas seulement le premier cran.
static void tournerVersCote(int angle) {
    if (coteChoisi == SECTEUR_GAUCHE) Deplacement_TournerGauche(angle);
    else                              Deplacement_TournerDroite(angle);
    angleTourne += angle;
}

// Pivote dans l'autre sens, donc vers la ligne. Sert deux fois : d'abord pour
// annuler l'angle d'ecartement et se remettre parallele a la ligne, ensuite
// pour se reorienter vers elle en fin de manoeuvre.
static void tournerVersLigne(int angle) {
    if (coteChoisi == SECTEUR_GAUCHE) Deplacement_TournerDroite(angle);
    else                              Deplacement_TournerGauche(angle);
}

static void terminer(void) {
    Securite_Arreter();
    Securite_DefinirManoeuvre(false);
    etat = REPOS;
}

// ======================== Enchainement des etapes ========================

// Lance l'action propre a une etape, a la sortie de la pause qui la precede.
static void entrerEtape(EtatEvitement etape) {
    etat = etape;
    switch (etape) {
        case SONDAGE:
            Obstacle_LancerSondage();
            break;
        case ROTATION_ALLER:
            tournerVersCote(EVITEMENT_ANGLE_DEG);
            break;
        case AVANCE_DIAGONALE:
            Deplacement_AvancerMetres(EVITEMENT_DISTANCE_M);
            break;
        case ROTATION_PARALLELE:
            // Rendre exactement l'angle d'ecartement remet le cap d'origine,
            // donc parallele a la ligne, decale lateralement de la diagonale.
            tournerVersLigne(angleTourne);
            break;
        case AVANCE_PARALLELE:
            Deplacement_AvancerMetres(EVITEMENT_LONGEMENT_M);
            break;
        case ROTATION_RETOUR:
            // Un cran de plus dans le meme sens : le vehicule quitte le cap
            // parallele et repart en diagonale vers la ligne, qu'il recroise.
            // L'angle est plus faible que celui d'ecartement pour garder la
            // ligne dans le champ de la camera pendant l'approche.
            tournerVersLigne(EVITEMENT_ANGLE_RETOUR_DEG);
            break;
        default:
            break;
    }
}

// Immobilise le vehicule avant de passer a l'etape suivante.
static void marquerPause(EtatEvitement suivante) {
    Securite_Arreter();
    etapeSuivante = suivante;
    tPause        = millis();
    etat          = PAUSE;
}

// ======================== API ========================

void Evitement_Initialiser(void) {
    etat          = REPOS;
    etapeSuivante = REPOS;
    angleTourne   = 0;
    essais        = 0;
    abandonne     = false;
}

void Evitement_MettreAJour(void) {
    switch (etat) {

        case REPOS:
            if (Securite_ObtenirMode() != MODE_AUTONOME) return;

            // Seul un obstacle declenche un contournement : un feu rouge
            // immobilise aussi le vehicule, mais on ne contourne pas un feu.
            // La disparition de l'obstacle rearme une manoeuvre abandonnee ;
            // sans ce verrou, un obstacle infranchissable relancerait le
            // contournement sans fin et le vehicule tournerait sur lui-meme.
            if (!Securite_ObstacleBloquant()) {
                abandonne = false;
                return;
            }
            if (abandonne)              return;
            if (Deplacement_EstActif()) return;

            // Le suivi de ligne cesse d'emettre des que le drapeau est pose,
            // mais la carte moteur tient sa derniere consigne : c'est la pause
            // qui immobilise reellement le vehicule avant le sondage.
            Securite_DefinirManoeuvre(true);
            angleTourne = 0;
            essais      = 0;
            marquerPause(SONDAGE);
            break;

        case PAUSE:
            if (millis() - tPause < EVITEMENT_PAUSE_MS) return;
            entrerEtape(etapeSuivante);
            break;

        case SONDAGE:
            if (Obstacle_SondageEnCours()) return;
            coteChoisi = Obstacle_CoteLePlusDegage();
            marquerPause(ROTATION_ALLER);
            break;

        case ROTATION_ALLER:
            if (Deplacement_EstActif()) return;

            // La voie n'est toujours pas degagee : pivoter d'un cran de plus
            // plutot que de lancer une avance que le veto refuserait.
            if (Securite_VetoActif()) {
                essais++;
                if (essais < EVITEMENT_ESSAIS_MAX) {
                    marquerPause(ROTATION_ALLER);
                    return;
                }
                abandonne = true;   // obstacle infranchissable : le veto maintient l'arret
                terminer();
                return;
            }

            marquerPause(AVANCE_DIAGONALE);
            break;

        case AVANCE_DIAGONALE:
            if (Deplacement_EstActif()) return;
            marquerPause(ROTATION_PARALLELE);
            break;

        case ROTATION_PARALLELE:
            if (Deplacement_EstActif()) return;
            marquerPause(AVANCE_PARALLELE);
            break;

        case AVANCE_PARALLELE:
            if (Deplacement_EstActif()) return;
            marquerPause(ROTATION_RETOUR);
            break;

        case ROTATION_RETOUR:
            if (Deplacement_EstActif()) return;
            terminer();
            break;
    }
}
