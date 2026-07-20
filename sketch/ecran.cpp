#include "ecran.h"
#include "config.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ======================== Ecran (SPI materiel) ========================

static Adafruit_ILI9341 ecran =
    Adafruit_ILI9341(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

// Dimensions en orientation paysage
#define ECRAN_LARGEUR   320
#define ECRAN_HAUTEUR   240

// Couleurs (RGB565)
#define COULEUR_FOND    ILI9341_NAVY
#define COULEUR_VISAGE  ILI9341_YELLOW
#define COULEUR_TRAIT   ILI9341_BLACK

// Geometrie du visage
#define VISAGE_CX       160
#define VISAGE_CY       120
#define VISAGE_RAYON     90
#define OEIL_DECAL_X     32
#define OEIL_DECAL_Y     28
#define OEIL_RAYON       14
#define SOURIRE_LARGEUR  50

// ======================== Primitive interne ========================

// Courbe epaisse par echantillonnage d'une Bezier quadratique
// (utilisee pour le sourire). Reference : courbe de Bezier, formule
// B(t) = (1-t)^2 P0 + 2(1-t)t P1 + t^2 P2.
static void traceCourbe(int x0, int y0, int xc, int yc,
                        int x1, int y1, int epaisseur, uint16_t couleur) {
    const int nbPas = 22;
    for (int i = 0; i <= nbPas; i++) {
        float t = (float)i / nbPas;
        float u = 1.0f - t;
        int x = (int)(u * u * x0 + 2 * u * t * xc + t * t * x1);
        int y = (int)(u * u * y0 + 2 * u * t * yc + t * t * y1);
        ecran.fillCircle(x, y, epaisseur, couleur);
    }
}

// ======================== API ========================

void Ecran_Initialiser(void) {
    ecran.begin();
    ecran.setRotation(3);          // paysage 320x240
    Ecran_AfficherSourire();
}

void Ecran_AfficherSourire(void) {
    ecran.fillScreen(COULEUR_FOND);

    // Visage
    ecran.fillCircle(VISAGE_CX, VISAGE_CY, VISAGE_RAYON, COULEUR_VISAGE);

    // Yeux
    ecran.fillCircle(VISAGE_CX - OEIL_DECAL_X, VISAGE_CY - OEIL_DECAL_Y,
                     OEIL_RAYON, COULEUR_TRAIT);
    ecran.fillCircle(VISAGE_CX + OEIL_DECAL_X, VISAGE_CY - OEIL_DECAL_Y,
                     OEIL_RAYON, COULEUR_TRAIT);

    // Sourire (courbe vers le bas)
    traceCourbe(VISAGE_CX - SOURIRE_LARGEUR, VISAGE_CY + 20,
                VISAGE_CX,                   VISAGE_CY + 60,
                VISAGE_CX + SOURIRE_LARGEUR, VISAGE_CY + 20,
                4, COULEUR_TRAIT);
}