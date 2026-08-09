#include "comm_bridge.h"
#include "config.h"
#include "Arduino_RouterBridge.h"

static volatile bool          feuPresent   = false;
static volatile int           couleurFeu   = COULEUR_AUCUNE;
static volatile unsigned long tDernierFeu  = 0;

static void on_feu(bool present, int couleur, int confiance) {
    (void)confiance;   // transmis par la vision, sans consommateur cote MCU
    feuPresent  = present;
    couleurFeu  = present ? couleur : COULEUR_AUCUNE;
    tDernierFeu = millis();
}

void CommBridge_Initialiser(void) {
    Bridge.begin();
    Bridge.provide_safe("on_feu", on_feu);
}

bool CommBridge_EstFeuPresent(void) {
    // Une detection qui cesse d'etre rafraichie ne decrit plus la scene :
    // sans peremption, une coupure du MPU immobiliserait le vehicule sur un
    // feu rouge fantome. Le pipeline de vision reemet toutes les 500 ms tant
    // que le feu reste visible.
    if (!feuPresent) return false;
    return (millis() - tDernierFeu) <= FEU_AGE_MAX_MS;
}

int CommBridge_ObtenirCouleurFeu(void) { return couleurFeu; }
