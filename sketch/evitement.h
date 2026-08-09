#ifndef EVITEMENT_H
#define EVITEMENT_H

#include <Arduino.h>

/*
 * Evitement_Initialiser
 *
 * PLACE LA MANOEUVRE D'EVITEMENT AU REPOS. A APPELER UNE FOIS DANS
 * setup(), APRES Securite_Initialiser().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Evitement_Initialiser(void);

/*
 * Evitement_MettreAJour
 *
 * FAIT PROGRESSER LE CONTOURNEMENT : SONDAGE, ROTATION VERS LE COTE
 * DEGAGE, LONGEMENT DE L'OBSTACLE, ROTATION DE RETOUR AU CAP INITIAL.
 * NE S'ENGAGE QU'EN MODE AUTONOME, VEHICULE IMMOBILISE PAR LE VETO.
 * NON BLOQUANTE. A APPELER A CHAQUE ITERATION DE loop().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Evitement_MettreAJour(void);

#endif
