#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

/*
 * Leds_Initialiser
 *
 * INITIALISE LES BANDEAUX LED ADRESSABLES (WS2813, PROTOCOLE
 * WS2812 / 800 KHZ). REGLE LA LUMINOSITE ET EFFECTUE UN TEST
 * VISUEL AU DEMARRAGE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Leds_Initialiser(void);

/*
 * Leds_DefinirMode
 *
 * CHANGE LE MODE D'ANIMATION D'UN BANDEAU LED.
 *
 * PARAMETRES :
 * bandeau — NUMERO DU BANDEAU (1 OU 2)
 * mode    — 0=ETEINT, 1=GYROPHARE, 2=CLIGNOTANT, 3=PHARES
 *
 * RETOUR :
 * AUCUN
 */
void Leds_DefinirMode(int bandeau, int mode);

/*
 * Leds_MettreAJour
 *
 * FAIT PROGRESSER LES ANIMATIONS DES BANDEAUX. DOIT ETRE APPELE
 * A CHAQUE ITERATION DE LOOP() (NON BLOQUANT).
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Leds_MettreAJour(void);

#endif