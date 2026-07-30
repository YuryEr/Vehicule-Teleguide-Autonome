#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

/*
 * Leds_Initialiser
 *
 * INITIALISE LES DEUX BANDEAUX (AVANT/ARRIERE) ET LES PHARES
 * AVANT, PUIS LES ETEINT. A APPELER UNE FOIS DANS SETUP().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Leds_Initialiser(void);

/*
 * Leds_DefinirModeBandeaux
 *
 * CHANGE LE MODE DES DEUX BANDEAUX SIMULTANEMENT. LE RENDU EST
 * ASSURE PAR Leds_MettreAJour() A CHAQUE ITERATION DE LOOP().
 *
 * PARAMETRE :
 * mode — 0 ETEINT
 *        1 FEUX DE POSITION (AVANT BLANC, ARRIERE ROUGE)
 *        2 GYROPHARE (ALTERNANCE BLEU/ROUGE)
 *
 * RETOUR :
 * AUCUN
 */
void Leds_DefinirModeBandeaux(int mode);

/*
 * Leds_DefinirPhares
 *
 * ALLUME OU ETEINT LES DEUX LEDS DE PHARE AVANT (BLANC PLEINE
 * PUISSANCE). RENDU IMMEDIAT, PAS D'ANIMATION.
 *
 * PARAMETRE :
 * actif — 0 ETEINT, 1 ALLUME
 *
 * RETOUR :
 * AUCUN
 */
void Leds_DefinirPhares(int actif);

/*
 * Leds_MettreAJour
 *
 * FAIT PROGRESSER L'ANIMATION DES BANDEAUX (NON BLOQUANT, BASE
 * SUR millis()). DOIT ETRE APPELE A CHAQUE ITERATION DE LOOP().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Leds_MettreAJour(void);

#endif