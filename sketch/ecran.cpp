#include "ecran.h"
#include "config.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <qrcode.h>

/*
 * Generation des codes QR : librairie QRCode de Richard Moore (ricmoo),
 * licence MIT — https://github.com/ricmoo/QRCode
 * Son auteur cite la librairie C++ de Project Nayuki comme determinante
 * dans son developpement, licence MIT
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Symbologie : ISO/IEC 18004:2015, Information technology — Automatic
 * identification and data capture techniques — QR Code bar code
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

// Couleurs (RGB565)
#define COULEUR_FOND    ILI9341_NAVY
#define COULEUR_TITRE   ILI9341_WHITE
#define COULEUR_TEXTE   ILI9341_YELLOW

// Le symbole occupe la moitie gauche, les informations en clair la droite.
#define QR_CENTRE_X     100
#define QR_CENTRE_Y     130
#define TEXTE_X         205

// ======================== Primitives internes ========================

// Trace un QR code centre sur (centreX, centreY). La zone de silence est
// peinte en blanc autour du symbole : sans elle, les lecteurs echouent a
// isoler le motif du fond.
static void dessinerQr(const char *texte, int centreX, int centreY) {
    QRCode qr;
    uint8_t donnees[qrcode_getBufferSize(QR_VERSION)];
    qrcode_initText(&qr, donnees, QR_VERSION, ECC_LOW, texte);

    int cote     = qr.size * QR_TAILLE_MODULE;
    int silence  = QR_ZONE_SILENCE * QR_TAILLE_MODULE;
    int origineX = centreX - cote / 2;
    int origineY = centreY - cote / 2;

    ecran.fillRect(origineX - silence, origineY - silence,
                   cote + 2 * silence, cote + 2 * silence, ILI9341_WHITE);

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

static void ecrireTitre(const char *titre) {
    ecran.fillScreen(COULEUR_FOND);
    ecran.setTextColor(COULEUR_TITRE);
    ecran.setTextSize(2);
    ecran.setCursor(10, 10);
    ecran.print(titre);
}

static void ecrireLigne(int y, const char *etiquette, const char *valeur) {
    ecran.setTextColor(COULEUR_TEXTE);
    ecran.setTextSize(1);
    ecran.setCursor(TEXTE_X, y);
    ecran.print(etiquette);
    ecran.setCursor(TEXTE_X, y + 12);
    ecran.print(valeur);
}

// ======================== API ========================

void Ecran_Initialiser(void) {
    ecran.begin();
    ecran.setRotation(1);          // paysage 320x240
    Ecran_AfficherAttente();
}

void Ecran_AfficherAttente(void) {
    ecrireTitre("TankETS");
    ecran.setTextColor(COULEUR_TEXTE);
    ecran.setTextSize(1);
    ecran.setCursor(10, 60);
    ecran.print("Demarrage du serveur...");
}

void Ecran_AfficherQrReseau(const char *ssid, const char *mdp) {
    char charge[96];
    snprintf(charge, sizeof(charge), "WIFI:T:WPA;S:%s;P:%s;;", ssid, mdp);

    ecrireTitre("1. Reseau");
    dessinerQr(charge, QR_CENTRE_X, QR_CENTRE_Y);
    ecrireLigne(70,  "Reseau :",       ssid);
    ecrireLigne(110, "Mot de passe :", mdp);
}

void Ecran_AfficherQrControle(const char *ip) {
    char charge[64];
    snprintf(charge, sizeof(charge), "http://%s:7000", ip);

    ecrireTitre("2. Controle");
    dessinerQr(charge, QR_CENTRE_X, QR_CENTRE_Y);
    ecrireLigne(70, "Adresse :", charge);
}
