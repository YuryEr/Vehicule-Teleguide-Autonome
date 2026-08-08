#include "ecran.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <qrcode.h>

/*
 * Generation des codes QR : librairie QRCode de Richard Moore (ricmoo),
 * licence MIT, https://github.com/ricmoo/QRCode
 * Son auteur cite la librairie C++ de Project Nayuki comme determinante
 * dans son developpement, licence MIT
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Symbologie : ISO/IEC 18004:2015, Information technology, Automatic
 * identification and data capture techniques, QR Code bar code
 * symbology specification.
 *
 * Format d'adhesion WiFi "WIFI:T:...;S:...;P:...;;" : schema de facto
 * defini par le projet ZXing, reconnu nativement par iOS 11 et
 * Android 10 et versions ulterieures.
 * https://github.com/zxing/zxing/wiki/Barcode-Contents
 */

// ======================== Ecran (SPI materiel) ========================

static Adafruit_ILI9341 ecran =
    Adafruit_ILI9341(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

#define ECRAN_LARGEUR   320
#define ECRAN_HAUTEUR   240

// Couleurs (RGB565)
#define COULEUR_FOND    ILI9341_MAROON
#define COULEUR_TITRE   ILI9341_WHITE
#define COULEUR_TEXTE   ILI9341_WHITE

// La police par defaut d'Adafruit GFX occupe six pixels de large et huit
// de haut par caractere, a l'echelle 1.
#define CARACTERE_LARGEUR  6

// ======================== Disposition ========================

// Nombre de modules d'un symbole, defini par la norme : 4 x version + 17.
#define QR_NB_MODULES    (4 * QR_VERSION + 17)
#define QR_COTE          (QR_NB_MODULES * QR_TAILLE_MODULE)
#define QR_SILENCE       (QR_ZONE_SILENCE * QR_TAILLE_MODULE)

// Les deux symboles partagent une bande blanche unique : la zone de
// silence exigee par la norme est mutualisee entre eux, ce qui laisse la
// place a des modules plus grands. La bande est centree dans la dalle.
#define BANDE_LARGEUR    (2 * QR_COTE + 4 * QR_SILENCE)
#define BANDE_HAUTEUR    (QR_COTE + 2 * QR_SILENCE)
#define BANDE_X          ((ECRAN_LARGEUR - BANDE_LARGEUR) / 2)
#define BANDE_Y          30

#define QR1_X            (BANDE_X + QR_SILENCE)
#define QR2_X            (QR1_X + QR_COTE + 2 * QR_SILENCE)
#define QR_Y             (BANDE_Y + QR_SILENCE)

#define QR1_CENTRE_X     (QR1_X + QR_COTE / 2)
#define QR2_CENTRE_X     (QR2_X + QR_COTE / 2)

#define LIGNE_ETIQUETTES (BANDE_Y + BANDE_HAUTEUR + 8)
#define LIGNE_INFOS      (LIGNE_ETIQUETTES + 18)
#define INTERLIGNE       12

// ======================== Primitives internes ========================

// Trace un QR code dont le coin superieur gauche est en (origineX, origineY).
// Le fond blanc n'est pas peint ici : les deux symboles se partagent une
// bande commune, tracee par l'appelant.
static void dessinerQr(const char *texte, int origineX, int origineY) {
    QRCode qr;
    uint8_t donnees[qrcode_getBufferSize(QR_VERSION)];
    qrcode_initText(&qr, donnees, QR_VERSION, ECC_LOW, texte);

    for (uint8_t y = 0; y < qr.size; y++) {
        for (uint8_t x = 0; x < qr.size; x++) {
            if (!qrcode_getModule(&qr, x, y)) continue;
            ecran.fillRect(origineX + x * QR_TAILLE_MODULE,
                           origineY + y * QR_TAILLE_MODULE,
                           QR_TAILLE_MODULE, QR_TAILLE_MODULE,
                           ILI9341_BLACK);
        }
    }
}

// Ecrit un texte centre sur une abscisse donnee.
static void ecrireCentre(const char *texte, int centreX, int y,
                          uint8_t echelle, uint16_t couleur) {
    int largeur = (int)strlen(texte) * CARACTERE_LARGEUR * echelle;
    ecran.setTextColor(couleur);
    ecran.setTextSize(echelle);
    ecran.setCursor(centreX - largeur / 2, y);
    ecran.print(texte);
}

// ======================== API ========================

void Ecran_Initialiser(void) {
    ecran.begin();
    ecran.setRotation(1);          // paysage 320x240
    Ecran_AfficherAttente();
}

void Ecran_AfficherAttente(void) {
    ecran.fillScreen(COULEUR_FOND);
    ecrireCentre("TankETS", ECRAN_LARGEUR / 2, 100, 3, COULEUR_TITRE);
    ecrireCentre("Demarrage du serveur...", ECRAN_LARGEUR / 2, 140, 1,
                 COULEUR_TEXTE);
}

void Ecran_AfficherConnexion(const char *ssid, const char *mdp,
                              const char *ip) {
    char reseau[96];
    char adresse[64];
    char ligne[64];

    snprintf(reseau,  sizeof(reseau),  "WIFI:T:WPA;S:%s;P:%s;;", ssid, mdp);
    snprintf(adresse, sizeof(adresse), "http://%s:7000", ip);

    ecran.fillScreen(COULEUR_FOND);
    ecrireCentre("TankETS", ECRAN_LARGEUR / 2, 8, 2, COULEUR_TITRE);

    ecran.fillRect(BANDE_X, BANDE_Y, BANDE_LARGEUR, BANDE_HAUTEUR,
                   ILI9341_WHITE);
    dessinerQr(reseau,  QR1_X, QR_Y);
    dessinerQr(adresse, QR2_X, QR_Y);

    ecrireCentre("1. Reseau",   QR1_CENTRE_X, LIGNE_ETIQUETTES, 1,
                 COULEUR_TEXTE);
    ecrireCentre("2. Controle", QR2_CENTRE_X, LIGNE_ETIQUETTES, 1,
                 COULEUR_TEXTE);

    snprintf(ligne, sizeof(ligne), "Reseau : %s", ssid);
    ecrireCentre(ligne, ECRAN_LARGEUR / 2, LIGNE_INFOS, 1, COULEUR_TEXTE);

    snprintf(ligne, sizeof(ligne), "Mot de passe : %s", mdp);
    ecrireCentre(ligne, ECRAN_LARGEUR / 2, LIGNE_INFOS + INTERLIGNE, 1,
                 COULEUR_TEXTE);

    ecrireCentre(adresse, ECRAN_LARGEUR / 2, LIGNE_INFOS + 2 * INTERLIGNE,
                 1, COULEUR_TEXTE);
}
