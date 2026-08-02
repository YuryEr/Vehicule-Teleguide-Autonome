#include "evitement.h"
#include "config.h"
#include "obstacle.h"
#include "deplacement.h"
#include "securite.h"

// ======================== Machine a etats ========================

enum EtatEvitement {
    REPOS,
    SONDAGE,
    ROTATION_ALLER,
    LONGEMENT,
    ROTATION_RETOUR
};

static EtatEvitement etat        = REPOS;
static int           coteChoisi  = SECTEUR_GAUCHE;
static int           angleTourne = 0;
static int           essais      = 0;
static bool          abandonne   = false;

// ======================== Rotations ========================

// Pivote vers le cote retenu et cumule l'angle parcouru. La rotation de
// retour annule ce cumul : les deux mouvements sous-tournent de la meme
// marge, leurs erreurs se compensent et le cap initial est preserve.
static void tournerVersCote(int angle) {
    if (coteChoisi == SECTEUR_GAUCHE) Deplacement_TournerGauche(angle);
    else                              Deplacement_TournerDroite(angle);
    angleTourne += angle;
}

static void tournerRetour(void) {
    if (coteChoisi == SECTEUR_GAUCHE) Deplacement_TournerDroite(angleTourne);
    else                              Deplacement_TournerGauche(angleTourne);
}

static void terminer(void) {
    Securite_DefinirManoeuvre(false);
    etat = REPOS;
}

// ======================== API ========================

void Evitement_Initialiser(void) {
    etat        = REPOS;
    angleTourne = 0;
    essais      = 0;
    abandonne   = false;
}

bool Evitement_EnCours(void) { return etat != REPOS; }

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

            // Le veto a deja immobilise le vehicule : la manoeuvre part d'un
            // arret franc, condition necessaire a un sondage exploitable.
            Securite_DefinirManoeuvre(true);
            angleTourne = 0;
            essais      = 0;
            Obstacle_LancerSondage();
            etat = SONDAGE;
            break;

        case SONDAGE:
            if (Obstacle_SondageEnCours()) return;
            coteChoisi = Obstacle_CoteLePlusDegage();
            tournerVersCote(OBSTACLE_ECART_SONDAGE_DEG);
            etat = ROTATION_ALLER;
            break;

        case ROTATION_ALLER:
            if (Deplacement_EstActif()) return;

            // La voie n'est toujours pas degagee : pivoter d'un cran de plus
            // plutot que de lancer une avance que le veto refuserait.
            if (Securite_VetoActif()) {
                essais++;
                if (essais < EVITEMENT_ESSAIS_MAX) {
                    tournerVersCote(OBSTACLE_ECART_SONDAGE_DEG);
                    return;
                }
                abandonne = true;   // obstacle infranchissable : le veto maintient l'arret
                terminer();
                return;
            }

            Deplacement_AvancerMetres(EVITEMENT_DISTANCE_M);
            etat = LONGEMENT;
            break;

        case LONGEMENT:
            if (Deplacement_EstActif()) return;
            tournerRetour();
            etat = ROTATION_RETOUR;
            break;

        case ROTATION_RETOUR:
            if (Deplacement_EstActif()) return;
            terminer();
            break;
    }
}
