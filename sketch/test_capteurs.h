#ifndef TEST_CAPTEURS_H
#define TEST_CAPTEURS_H

#include <Arduino.h>

// Commenter la ligne suivante pour desactiver le releve periodique.
#define TEST_CAPTEURS_ACTIF

/*
 * TestCapteurs_MettreAJour
 *
 * AFFICHE PERIODIQUEMENT SUR LE PORT SERIE LA DISTANCE MESUREE PAR
 * L'ULTRASON ET LE LIDAR, AINSI QUE L'ORIENTATION DU SERVO. SANS EFFET
 * SI TEST_CAPTEURS_ACTIF N'EST PAS DEFINI. A APPELER DANS loop().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void TestCapteurs_MettreAJour(void);

#endif
