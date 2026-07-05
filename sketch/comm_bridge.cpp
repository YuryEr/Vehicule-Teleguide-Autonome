#include "comm_bridge.h"
#include "Arduino_RouterBridge.h"

static volatile bool feuPresent      = false;
static volatile int  couleurFeu      = COULEUR_AUCUNE;
static volatile int  confianceFeu    = 0;
static volatile bool lignesDetectees = false;
static volatile int  ecartLignes     = 0;
static volatile bool changement      = false;

static void on_feu(bool present, int couleur, int confiance) {
    feuPresent   = present;
    couleurFeu   = present ? couleur : COULEUR_AUCUNE;
    confianceFeu = confiance;
    changement   = true;
}

static void on_lignes(bool detecte, int ecart) {
    lignesDetectees = detecte;
    ecartLignes     = ecart;
    changement      = true;
}

void CommBridge_Initialiser(void) {
    Bridge.begin();
    Bridge.provide_safe("on_feu",    on_feu);
    Bridge.provide_safe("on_lignes", on_lignes);
}

bool CommBridge_EstFeuPresent(void)      { return feuPresent; }
int  CommBridge_ObtenirCouleurFeu(void)  { return couleurFeu; }
int  CommBridge_ObtenirConfianceFeu(void){ return confianceFeu; }
bool CommBridge_SontLignesDetectees(void){ return lignesDetectees; }
int  CommBridge_ObtenirEcartLignes(void) { return ecartLignes; }

bool CommBridge_YAChangement(void) {
    if (changement) {
        changement = false;
        return true;
    }
    return false;
}