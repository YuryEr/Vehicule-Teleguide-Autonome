#ifndef SERVO_LIDAR_H
#define SERVO_LIDAR_H

#include <Arduino.h>

/*
 * ServoLidar_Initialiser
 *
 * ATTACHE LE SERVO DE BALAYAGE (SG90) ET LE PLACE AU CENTRE
 * (90 DEGRES = DROIT DEVANT). A APPELER UNE FOIS DANS setup().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void ServoLidar_Initialiser(void);

/*
 * ServoLidar_DefinirAngle
 *
 * DEFINIT L'ANGLE CIBLE DU SERVO. LE MOUVEMENT EST PROGRESSIF ET
 * NON BLOQUANT : IL EST EXECUTE PAR ServoLidar_MettreAJour().
 * L'ANGLE EST BORNE ENTRE SERVO_ANGLE_MIN ET SERVO_ANGLE_MAX.
 *
 * PARAMETRE :
 * angle : ANGLE CIBLE EN DEGRES (0 A 180, 90 = DROIT DEVANT)
 *
 * RETOUR :
 * AUCUN
 */
void ServoLidar_DefinirAngle(int angle);

/*
 * ServoLidar_MettreAJour
 *
 * FAIT PROGRESSER LE SERVO D'UN DEGRE VERS L'ANGLE CIBLE, A LA
 * CADENCE DEFINIE PAR SERVO_MS_PAR_DEGRE. NON BLOQUANT : DOIT
 * ETRE APPELE A CHAQUE ITERATION DE LOOP().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void ServoLidar_MettreAJour(void);

/*
 * ServoLidar_AngleActuel
 *
 * RETOURNE LA POSITION COURANTE DU SERVO EN DEGRES.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * ANGLE EN DEGRES (0 A 180)
 */
int ServoLidar_AngleActuel(void);

/*
 * ServoLidar_EstEnMouvement
 *
 * INDIQUE SI LE SERVO N'A PAS ENCORE ATTEINT SON ANGLE CIBLE.
 * UTILE POUR ATTENDRE LA STABILISATION AVANT UNE MESURE LIDAR.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI UN MOUVEMENT EST EN COURS, false SINON
 */
bool ServoLidar_EstEnMouvement(void);


#endif
