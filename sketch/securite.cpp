#include "securite.h"
#include "moteurs.h"
#include "obstacle.h"

static int mode = MODE_MANUEL;

// Une commande est une avance si les deux cotes poussent vers l'avant.
// Une rotation (signes opposes) et une marche arriere restent autorisees :
// ce sont les manoeuvres qui degagent le vehicule d'un obstacle.
static bool estUneAvance(int gauche, int droite) {
    return (gauche > 0 && droite > 0);
}

void Securite_Initialiser(void) {
    mode = MODE_MANUEL;
}

void Securite_DefinirMode(int nouveauMode) {
    mode = nouveauMode;
}

int Securite_ObtenirMode(void) {
    return mode;
}

bool Securite_VetoActif(void) {
    return (mode == MODE_AUTONOME) && Obstacle_EstDetecte();
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
