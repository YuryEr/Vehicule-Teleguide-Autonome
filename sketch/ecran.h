#ifndef ECRAN_H
#define ECRAN_H

#include <Arduino.h>

/*
 * Ecran_Initialiser
 *
 * INITIALISE L'ECRAN TFT ILI9341 (SPI MATERIEL, 320x240,
 * ORIENTATION PAYSAGE) ET AFFICHE LE VISAGE SOURIANT.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_Initialiser(void);

/*
 * Ecran_AfficherSourire
 *
 * DESSINE UN BONHOMME SOURIANT SIMPLE (VISAGE, DEUX YEUX ET
 * UN SOURIRE) CENTRE SUR L'ECRAN.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherSourire(void);

#endif