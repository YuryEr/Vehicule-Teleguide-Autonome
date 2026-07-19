#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "leds.h"

static Adafruit_NeoPixel bandeau1(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_1,NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel bandeau2(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_2, NEO_GRB + NEO_KHZ800);

// Etat d'animation de chaque bandeau
static volatile int mode1 = 0, mode2 = 0;
static unsigned long timer1 = 0, timer2 = 0;
static bool alternance1 = false, alternance2 = false;
static bool statique1 = false, statique2 = false;

static void Remplir(Adafruit_NeoPixel &b, uint8_t r, uint8_t v, uint8_t bl) {
    for (int i = 0; i < NB_LEDS_PAR_BANDEAU; i++) {
        b.setPixelColor(i, b.Color(r, v, bl));
    }
    b.show();
}

static void Animer(Adafruit_NeoPixel &b, int mode, unsigned long &timer,
                    bool &alternance, bool &statique) {
    unsigned long maintenant = millis();

    switch (mode) {

        case 0:  // Eteint — rendu une seule fois
            if (!statique) {
                Remplir(b, 0, 0, 0);
                statique = true;
            }
            break;

        case 1:  // Gyrophare — alternance bleu / rouge
            if (maintenant - timer >= PERIODE_GYROPHARE_MS) {
                timer = maintenant;
                alternance = !alternance;
                if (alternance) Remplir(b, 0, 0, 200);
                else            Remplir(b, 200, 0, 0);
            }
            break;

        case 2:  // Clignotant — orange intermittent
            if (maintenant - timer >= PERIODE_CLIGNOTANT_MS) {
                timer = maintenant;
                alternance = !alternance;
                if (alternance) Remplir(b, 200, 80, 0);
                else            Remplir(b, 0, 0, 0);
            }
            break;

        case 3:  // Phares — blanc continu, rendu une seule fois
            if (!statique) {
                Remplir(b, 200, 200, 200);
                statique = true;
            }
            break;
    }
}

void Leds_Initialiser(void) {
    bandeau1.begin();
    bandeau2.begin();
    bandeau1.setBrightness(LUMINOSITE_LEDS);
    bandeau2.setBrightness(LUMINOSITE_LEDS);

    
     // --- Test visuel au demarrage (a retirer apres validation) ---
    Remplir(bandeau1, 0, 100, 0);
    Remplir(bandeau2, 0, 100, 0);
    delay(2000);
    // -------------------------------------------------------------

  
    Remplir(bandeau1, 0, 0, 0);
    Remplir(bandeau2, 0, 0, 0);
    statique1 = true;
    statique2 = true;
}

void Leds_DefinirMode(int bandeau, int mode) {
    if (bandeau == 1) {
        mode1 = mode;
        alternance1 = false;
        statique1 = false;   // force le rendu au prochain passage
    } else if (bandeau == 2) {
        mode2 = mode;
        alternance2 = false;
        statique2 = false;
    }
}

void Leds_MettreAJour(void) {
    Animer(bandeau1, mode1, timer1, alternance1, statique1);
    Animer(bandeau2, mode2, timer2, alternance2, statique2);
}3