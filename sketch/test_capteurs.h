#ifndef TEST_CAPTEURS_H
#define TEST_CAPTEURS_H

#include <Arduino.h>

// A N'ACTIVER QUE POUR UN DIAGNOSTIC, JAMAIS EN DEMONSTRATION.
//
// La sortie serie du MCU et les appels RPC du Bridge partagent le meme lien
// vers le MPU. Ce releve ecrit environ 500 octets par seconde en continu, et
// sous cette charge le multiplexage finit par perdre une frontiere de trame :
// la boucle de lecture du Bridge tente alors de decoder du binaire en UTF-8,
// leve une exception et meurt. Tous les appels RPC expirent ensuite au bout
// de dix secondes, sans retour possible avant un redemarrage.
//
// Symptome observe : une sequence de blocs qui s'interrompt apres une dizaine
// de commandes, avec dans le journal
//   [Bridge.read_loop] Unexpected error: 'utf-8' codec can't decode byte ...
//
// Decommenter la ligne suivante pour activer le releve.
// #define TEST_CAPTEURS_ACTIF

/*
 * TestCapteurs_MettreAJour
 *
 * AFFICHE PERIODIQUEMENT SUR LE PORT SERIE LES DISTANCES MESUREES, LA CARTE
 * DES SECTEURS ET LES DEUX COMPTEURS D'ENCODEUR. SANS EFFET SI
 * TEST_CAPTEURS_ACTIF N'EST PAS DEFINI. A APPELER DANS loop().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void TestCapteurs_MettreAJour(void);

#endif
