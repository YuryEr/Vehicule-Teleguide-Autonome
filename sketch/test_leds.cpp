#include "test_leds.h"
#include "config.h"
#include "leds.h"

#ifdef TEST_LEDS_ACTIF

#define PERIODE_ETAPE_MS  1000

// Etapes de la demonstration, dans l'ordre de defilement.
enum EtapeDemo {
    DEMO_ETEINT,
    DEMO_POSITION,
    DEMO_GYROPHARE,
    DEMO_PHARES,
    DEMO_CLIGNO_GAUCHE,
    DEMO_CLIGNO_DROITE,
    DEMO_TERMINEE
};

static EtapeDemo     etape    = DEMO_ETEINT;
static unsigned long tEtape   = 0;
static bool          demarree = false;

// Applique la combinaison correspondant a l'etape courante.
static void appliquerEtape(void) {
    switch (etape) {

        case DEMO_ETEINT:
            Leds_DefinirModeBandeaux(MODE_BANDEAUX_ETEINT);
            Leds_DefinirPhares(0);
            Leds_DefinirVirage(VIRAGE_AUCUN);
            break;

        case DEMO_POSITION:
            Leds_DefinirModeBandeaux(MODE_BANDEAUX_POSITION);
            break;

        case DEMO_GYROPHARE:
            Leds_DefinirModeBandeaux(MODE_BANDEAUX_GYROPHARE);
            break;

        case DEMO_PHARES:
            Leds_DefinirModeBandeaux(MODE_BANDEAUX_ETEINT);
            Leds_DefinirPhares(1);
            break;

        case DEMO_CLIGNO_GAUCHE:
            Leds_DefinirVirage(VIRAGE_GAUCHE);
            break;

        case DEMO_CLIGNO_DROITE:
            Leds_DefinirVirage(VIRAGE_DROITE);
            break;

        case DEMO_TERMINEE:
            // Etat neutre : les commandes de l'interface reprennent la main.
            Leds_DefinirModeBandeaux(MODE_BANDEAUX_ETEINT);
            Leds_DefinirPhares(0);
            Leds_DefinirVirage(VIRAGE_AUCUN);
            break;
    }
}

void TestLeds_MettreAJour(void) {
    if (etape == DEMO_TERMINEE) return;

    unsigned long maintenant = millis();

    if (!demarree) {
        demarree = true;
        tEtape   = maintenant;
        appliquerEtape();
        return;
    }

    if (maintenant - tEtape < PERIODE_ETAPE_MS) return;

    tEtape = maintenant;
    etape  = (EtapeDemo)(etape + 1);
    appliquerEtape();

    if (etape == DEMO_TERMINEE) {
        Serial.println("[leds] Demonstration terminee");
    }
}

bool TestLeds_EstActif(void) { return etape != DEMO_TERMINEE; }

#else

void TestLeds_MettreAJour(void) { }
bool TestLeds_EstActif(void)    { return false; }

#endif
