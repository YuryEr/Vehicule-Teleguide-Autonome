#ifndef ECRAN_H
#define ECRAN_H

#include <Arduino.h>

/*
 * Ecran_Initialiser
 *
 * PREPARE L'ECRAN ILI9341 (SPI MATERIEL, 320x240, ORIENTATION PAYSAGE)
 * ET AFFICHE LA PAGE D'ATTENTE. A APPELER UNE FOIS DANS setup().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_Initialiser(void);

/*
 * Ecran_AfficherAttente
 *
 * AFFICHE UN MESSAGE D'ATTENTE. LE MCU DEMARRE AVANT LE CONTENEUR
 * PYTHON ET NE CONNAIT PAS ENCORE LES PARAMETRES DU RESEAU.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherAttente(void);

/*
 * Ecran_AfficherQrReseau
 *
 * AFFICHE LE CODE QR D'ADHESION AU RESEAU WIFI, ACCOMPAGNE DU NOM DU
 * RESEAU ET DU MOT DE PASSE EN CLAIR POUR UNE SAISIE MANUELLE.
 *
 * PARAMETRES :
 * ssid — NOM DU RESEAU
 * mdp  — MOT DE PASSE DU RESEAU
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherQrReseau(const char *ssid, const char *mdp);

/*
 * Ecran_AfficherQrControle
 *
 * AFFICHE LE CODE QR OUVRANT LA PAGE DE CONTROLE, ACCOMPAGNE DE
 * L'ADRESSE COMPLETE EN CLAIR.
 *
 * PARAMETRE :
 * ip — ADRESSE IP DU SERVEUR SUR LE RESEAU
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherQrControle(const char *ip);

#endif
