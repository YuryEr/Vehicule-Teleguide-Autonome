#include "comm_bridge.h"
#include "config.h"
#include "Arduino_RouterBridge.h"

static volatile bool          feuPresent      = false;
static volatile int           couleurFeu      = COULEUR_AUCUNE;
static volatile int           confianceFeu    = 0;
static volatile unsigned long tDernierFeu     = 0;
static volatile bool          lignesDetectees = false;
static volatile int           ecartLignes     = 0;
static volatile bool          changement      = false;

static void on_feu(bool present, int couleur, int confiance) {
    feuPresent   = present;
    couleurFeu   = present ? couleur : COULEUR_AUCUNE;
    confianceFeu = confiance;
    tDernierFeu  = millis();
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

bool CommBridge_EstFeuPresent(void) {
    // Une detection qui cesse d'etre rafraichie ne decrit plus la scene :
    // sans peremption, une coupure du MPU immobiliserait le vehicule sur un
    // feu rouge fantome. Le pipeline de vision reemet toutes les 500 ms tant
    // que le feu reste visible.
    if (!feuPresent) return false;
    return (millis() - tDernierFeu) <= FEU_AGE_MAX_MS;
}

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