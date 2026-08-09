#include "securite.h"
#include "moteurs.h"
#include "obstacle.h"
#include "comm_bridge.h"

static int  mode             = MODE_MANUEL;
static bool manoeuvreEnCours = false;

// Une commande est une avance si les deux cotes poussent vers l'avant.
// Une rotation (signes opposes) et une marche arriere restent autorisees :
// ce sont les manoeuvres qui degagent le vehicule d'un obstacle.
static bool estUneAvance(int gauche, int droite) {
    return (gauche > 0 && droite > 0);
}

void Securite_Initialiser(void) {
    mode             = MODE_MANUEL;
    manoeuvreEnCours = false;
}

void Securite_DefinirMode(int nouveauMode) {
    mode = nouveauMode;
}

int Securite_ObtenirMode(void) {
    return mode;
}

// Le rouge et le jaune imposent l'arret. Le jaune est traite comme le rouge :
// devant un feu sur le point de passer, s'arreter est le comportement attendu.
static bool feuImposeArret(void) {
    if (!CommBridge_EstFeuPresent()) return false;
    int couleur = CommBridge_ObtenirCouleurFeu();
    return (couleur == COULEUR_ROUGE || couleur == COULEUR_JAUNE);
}

bool Securite_ObstacleBloquant(void) {
    return (mode == MODE_AUTONOME) && Obstacle_EstDetecte();
}

bool Securite_FeuBloquant(void) {
    return (mode == MODE_AUTONOME) && feuImposeArret();
}

bool Securite_VetoActif(void) {
    return Securite_ObstacleBloquant() || Securite_FeuBloquant();
}

void Securite_DefinirVitesse(int gauche, int droite) {
    if (Securite_VetoActif() && estUneAvance(gauche, droite)) {
        Moteurs_Arreter();
        return;
    }
    Moteurs_DefinirVitesse(gauche, droite);
}

void Securite_Arreter(void) {
    Moteurs_Arreter();
}

void Securite_DefinirManoeuvre(bool actif) { manoeuvreEnCours = actif; }
bool Securite_ManoeuvreEnCours(void)       { return manoeuvreEnCours; }
