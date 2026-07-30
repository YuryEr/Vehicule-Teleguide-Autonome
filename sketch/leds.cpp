#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "leds.h"

static Adafruit_NeoPixel bandeauAvant(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_AVANT,
                                       NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel bandeauArriere(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_ARRIERE,
                                         NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel pharesAvant(NB_LEDS_PHARES, PIN_PHARES,
                                      NEO_GRB + NEO_KHZ800);

// Etat courant des bandeaux
static volatile int modeBandeaux = 0;
static unsigned long chrono      = 0;
static bool alternance           = false;
static bool rendusStatique       = false;

static void Remplir(Adafruit_NeoPixel &b, uint8_t r, uint8_t v, uint8_t bl,
                     int nbLeds) {
    for (int i = 0; i < nbLeds; i++) {
        b.setPixelColor(i, b.Color(r, v, bl));
    }
    b.show();
}

void Leds_Initialiser(void) {
    bandeauAvant.begin();
    bandeauArriere.begin();
    pharesAvant.begin();

    bandeauAvant.setBrightness(LUMINOSITE_BANDEAUX);
    bandeauArriere.setBrightness(LUMINOSITE_BANDEAUX);
    pharesAvant.setBrightness(LUMINOSITE_PHARES);

    Remplir(bandeauAvant,   0, 0, 0, NB_LEDS_PAR_BANDEAU);
    Remplir(bandeauArriere, 0, 0, 0, NB_LEDS_PAR_BANDEAU);
    Remplir(pharesAvant,    0, 0, 0, NB_LEDS_PHARES);

    rendusStatique = true;
}

void Leds_DefinirModeBandeaux(int mode) {
    modeBandeaux   = mode;
    alternance     = false;
    rendusStatique = false;   // force le rendu au prochain passage
}

void Leds_DefinirPhares(int actif) {
    if (actif) Remplir(pharesAvant, 255, 255, 255, NB_LEDS_PHARES);
    else       Remplir(pharesAvant, 0,   0,   0,   NB_LEDS_PHARES);
}

void Leds_MettreAJour(void) {
    unsigned long maintenant = millis();

    switch (modeBandeaux) {

        case 0:  // Eteint — rendu une seule fois
            if (!rendusStatique) {
                Remplir(bandeauAvant,   0, 0, 0, NB_LEDS_PAR_BANDEAU);
                Remplir(bandeauArriere, 0, 0, 0, NB_LEDS_PAR_BANDEAU);
                rendusStatique = true;
            }
            break;

        case 1:  // Feux de position — avant blanc, arriere rouge
            if (!rendusStatique) {
                Remplir(bandeauAvant,   255, 255, 255, NB_LEDS_PAR_BANDEAU);
                Remplir(bandeauArriere, 255, 0,   0,   NB_LEDS_PAR_BANDEAU);
                rendusStatique = true;
            }
            break;

        case 2:  // Gyrophare — alternance bleu / rouge sur les deux bandeaux
            if (maintenant - chrono >= PERIODE_GYROPHARE_MS) {
                chrono     = maintenant;
                alternance = !alternance;
                if (alternance) {
                    Remplir(bandeauAvant,   0,   0, 200, NB_LEDS_PAR_BANDEAU);
                    Remplir(bandeauArriere, 200, 0, 0,   NB_LEDS_PAR_BANDEAU);
                } else {
                    Remplir(bandeauAvant,   200, 0, 0,   NB_LEDS_PAR_BANDEAU);
                    Remplir(bandeauArriere, 0,   0, 200, NB_LEDS_PAR_BANDEAU);
                }
            }
            break;
    }
}