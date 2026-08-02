#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "leds.h"

// ======================== Couleurs ========================

// La barre reste volontairement discrete : elle partage le gain du bandeau
// avec les feux, qui doivent eux sortir a pleine puissance.
#define BARRE_INTENSITE   60
#define GYRO_INTENSITE   200
#define PHARE_BLANC      255
#define FEU_ROUGE        120
#define CLIGNO_ROUGE     255
#define CLIGNO_VERT       90

// ======================== Bandeaux ========================

static Adafruit_NeoPixel bandeauAvant(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_AVANT,
                                       NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel bandeauArriere(NB_LEDS_PAR_BANDEAU, PIN_BANDEAU_ARRIERE,
                                         NEO_GRB + NEO_KHZ800);

struct Couleur { uint8_t r, v, b; };

// Tampon de composition : chaque couche y ecrit, un seul envoi a la fin.
// Sans tampon, deux effets simultanes se chasseraient l'un l'autre a chaque
// rendu et le dernier appele gagnerait toujours.
static Couleur tamponAvant[NB_LEDS_PAR_BANDEAU];
static Couleur tamponArriere[NB_LEDS_PAR_BANDEAU];
static Couleur renduAvant[NB_LEDS_PAR_BANDEAU];
static Couleur renduArriere[NB_LEDS_PAR_BANDEAU];

static volatile int  modeBandeaux     = MODE_BANDEAUX_ETEINT;
static volatile bool pharesActifs     = false;
static volatile int  directionClignot = VIRAGE_AUCUN;

// ======================== Primitive de composition ========================

static void peindre(Couleur *tampon, int debut, int fin,
                     uint8_t r, uint8_t v, uint8_t b) {
    for (int i = debut; i <= fin; i++) {
        tampon[i].r = r;
        tampon[i].v = v;
        tampon[i].b = b;
    }
}

// ======================== Couche 1 : barre haute ========================

static void peindreBarre(unsigned long maintenant) {
    static unsigned long chrono     = 0;
    static bool          alternance = false;

    switch (modeBandeaux) {

        case MODE_BANDEAUX_POSITION:
            peindre(tamponAvant,   ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                    BARRE_INTENSITE, BARRE_INTENSITE, BARRE_INTENSITE);
            peindre(tamponArriere, ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                    BARRE_INTENSITE, 0, 0);
            break;

        case MODE_BANDEAUX_GYROPHARE:
            if (maintenant - chrono >= PERIODE_GYROPHARE_MS) {
                chrono     = maintenant;
                alternance = !alternance;
            }
            if (alternance) {
                peindre(tamponAvant,   ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                        0, 0, GYRO_INTENSITE);
                peindre(tamponArriere, ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                        GYRO_INTENSITE, 0, 0);
            } else {
                peindre(tamponAvant,   ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                        GYRO_INTENSITE, 0, 0);
                peindre(tamponArriere, ZONE_BARRE_DEBUT, ZONE_BARRE_FIN,
                        0, 0, GYRO_INTENSITE);
            }
            break;

        default:   // MODE_BANDEAUX_ETEINT
            peindre(tamponAvant,   ZONE_BARRE_DEBUT, ZONE_BARRE_FIN, 0, 0, 0);
            peindre(tamponArriere, ZONE_BARRE_DEBUT, ZONE_BARRE_FIN, 0, 0, 0);
            break;
    }
}

// ======================== Couche 2 : feux ========================

static void peindreFeux(void) {
    if (pharesActifs) {
        peindre(tamponAvant,   ZONE_PHARES_DEBUT, ZONE_PHARES_FIN,
                PHARE_BLANC, PHARE_BLANC, PHARE_BLANC);
        peindre(tamponArriere, ZONE_PHARES_DEBUT, ZONE_PHARES_FIN,
                FEU_ROUGE, 0, 0);
    } else {
        peindre(tamponAvant,   ZONE_PHARES_DEBUT, ZONE_PHARES_FIN, 0, 0, 0);
        peindre(tamponArriere, ZONE_PHARES_DEBUT, ZONE_PHARES_FIN, 0, 0, 0);
    }
}

// ======================== Couche 3 : clignotants ========================

// Le clignotant ecrase le pixel de son cote, a l'avant comme a l'arriere :
// une seule LED par cote tient lieu de feu et de repetiteur, comme sur un
// vehicule a signalisation integree.
static void peindreClignotants(unsigned long maintenant) {
    static unsigned long chrono              = 0;
    static bool          allume              = false;
    static int           directionPrecedente = VIRAGE_AUCUN;

    int direction = directionClignot;

    // Un virage qui debute allume immediatement, sans attendre un demi-cycle.
    if (direction != directionPrecedente) {
        directionPrecedente = direction;
        chrono              = maintenant;
        allume              = (direction != VIRAGE_AUCUN);
    }

    if (direction == VIRAGE_AUCUN) return;

    if (maintenant - chrono >= PERIODE_CLIGNOTANT_MS) {
        chrono = maintenant;
        allume = !allume;
    }
    if (!allume) return;

    int pixel = (direction == VIRAGE_GAUCHE) ? PIXEL_COTE_GAUCHE
                                             : PIXEL_COTE_DROIT;
    peindre(tamponAvant,   pixel, pixel, CLIGNO_ROUGE, CLIGNO_VERT, 0);
    peindre(tamponArriere, pixel, pixel, CLIGNO_ROUGE, CLIGNO_VERT, 0);
}

// ======================== Rendu ========================

// N'envoie le tampon que s'il a change. Un show() bloque la boucle le temps
// d'emettre le train de bits ; le repeter a chaque iteration perturberait
// les impulsions du servo de balayage.
static void rendreSiChange(Adafruit_NeoPixel &bandeau,
                            const Couleur *tampon, Couleur *rendu) {
    bool change = false;
    for (int i = 0; i < NB_LEDS_PAR_BANDEAU; i++) {
        if (tampon[i].r != rendu[i].r
            || tampon[i].v != rendu[i].v
            || tampon[i].b != rendu[i].b) {
            change = true;
            break;
        }
    }
    if (!change) return;

    for (int i = 0; i < NB_LEDS_PAR_BANDEAU; i++) {
        rendu[i] = tampon[i];
        bandeau.setPixelColor(i, bandeau.Color(tampon[i].r,
                                               tampon[i].v,
                                               tampon[i].b));
    }
    bandeau.show();
}

// ======================== API ========================

void Leds_Initialiser(void) {
    bandeauAvant.begin();
    bandeauArriere.begin();
    bandeauAvant.setBrightness(LUMINOSITE_BANDEAU);
    bandeauArriere.setBrightness(LUMINOSITE_BANDEAU);

    peindre(tamponAvant,   0, NB_LEDS_PAR_BANDEAU - 1, 0, 0, 0);
    peindre(tamponArriere, 0, NB_LEDS_PAR_BANDEAU - 1, 0, 0, 0);

    // Valeur qu'aucune couche ne produit : force le premier envoi et
    // garantit que les bandeaux sont eteints des le demarrage.
    peindre(renduAvant,   0, NB_LEDS_PAR_BANDEAU - 1, 1, 1, 1);
    peindre(renduArriere, 0, NB_LEDS_PAR_BANDEAU - 1, 1, 1, 1);

    modeBandeaux     = MODE_BANDEAUX_ETEINT;
    pharesActifs     = false;
    directionClignot = VIRAGE_AUCUN;
}

void Leds_DefinirModeBandeaux(int mode) { modeBandeaux = mode; }
void Leds_DefinirPhares(int actif)      { pharesActifs = (actif != 0); }
void Leds_DefinirVirage(int direction)  { directionClignot = direction; }

void Leds_MettreAJour(void) {
    unsigned long maintenant = millis();

    peindreBarre(maintenant);
    peindreFeux();
    peindreClignotants(maintenant);

    rendreSiChange(bandeauAvant,   tamponAvant,   renduAvant);
    rendreSiChange(bandeauArriere, tamponArriere, renduArriere);
}
