#ifndef ULTRASON_H
#define ULTRASON_H

#include <Arduino.h>

/*
 * Ultrason_Initialiser
 *
 * CONFIGURE LES BROCHES DU CAPTEUR HC-SR04 : TRIGGER EN SORTIE,
 * ECHO EN ENTREE. A APPELER UNE FOIS DANS setup().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ultrason_Initialiser(void);

/*
 * Ultrason_MettreAJour
 *
 * DECLENCHE UNE MESURE (THROTTLEE, NON BLOQUANTE) ET MET A JOUR
 * LA DISTANCE EN CACHE. DOIT ETRE APPELE A CHAQUE ITERATION DE
 * LOOP().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ultrason_MettreAJour(void);

/*
 * Ultrason_DistanceCm
 *
 * RETOURNE LA DERNIERE DISTANCE FRONTALE MESUREE, EN CM. BORNEE
 * A ULTRASON_DISTANCE_MAX (VOIE DEGAGEE). -1 SI AUCUNE MESURE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * DISTANCE EN CM
 */
int Ultrason_DistanceCm(void);

#endif