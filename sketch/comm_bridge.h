#ifndef COMM_BRIDGE_H
#define COMM_BRIDGE_H

#include <Arduino.h>

#define COULEUR_AUCUNE  0
#define COULEUR_ROUGE   1
#define COULEUR_JAUNE   2
#define COULEUR_VERT    3

/*
 * CommBridge_Initialiser
 *
 * INITIALISE LA COMMUNICATION BRIDGE (RPC) ET ENREGISTRE LES
 * HANDLERS QUI RECOIVENT LES DONNEES DE VISION DEPUIS LE MPU.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 *
 */
void CommBridge_Initialiser(void);

/*
 * CommBridge_EstFeuPresent
 *
 * INDIQUE SI UN FEU DE SIGNALISATION EST DETECTE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI UN FEU EST DETECTE, false SINON
 *
 */
bool CommBridge_EstFeuPresent(void);

/*
 * CommBridge_ObtenirCouleurFeu
 *
 * RETOURNE LA COULEUR DU FEU DETECTE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * COULEUR_ROUGE, COULEUR_JAUNE, COULEUR_VERT OU COULEUR_AUCUNE
 *
 */
int CommBridge_ObtenirCouleurFeu(void);

#endif
