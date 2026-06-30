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

/*
 * CommBridge_ObtenirConfianceFeu
 *
 * RETOURNE LE POURCENTAGE DE CONFIANCE DE LA DETECTION (0-100).
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * ENTIER DE 0 A 100
 *
 */
int CommBridge_ObtenirConfianceFeu(void);

/*
 * CommBridge_SontLignesDetectees
 *
 * INDIQUE SI DES LIGNES DE ROUTE SONT DETECTEES.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI DES LIGNES SONT DETECTEES, false SINON
 *
 */
bool CommBridge_SontLignesDetectees(void);

/*
 * CommBridge_ObtenirEcartLignes
 *
 * RETOURNE L'ECART LATERAL EN PIXELS ENTRE LE CENTRE DE VOIE
 * ET LE CENTRE IMAGE. POSITIF = DROITE, NEGATIF = GAUCHE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * ECART EN PIXELS (SIGNE)
 *
 */
int CommBridge_ObtenirEcartLignes(void);

/*
 * CommBridge_YAChangement
 *
 * INDIQUE SI DE NOUVELLES DONNEES ONT ETE RECUES DEPUIS LE
 * DERNIER APPEL. LE DRAPEAU EST REMIS A false APRES LECTURE.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI LES DONNEES ONT CHANGE, false SINON
 *
 */
bool CommBridge_YAChangement(void);

#endif