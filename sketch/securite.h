#ifndef SECURITE_H
#define SECURITE_H

#include <Arduino.h>

#define MODE_MANUEL    0
#define MODE_AUTONOME  1

/*
 * Securite_Initialiser
 *
 * PLACE LE VEHICULE EN MODE MANUEL. A APPELER UNE FOIS DANS setup(),
 * AVANT TOUTE COMMANDE MOTEUR.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Securite_Initialiser(void);

/*
 * Securite_DefinirMode
 *
 * CHOISIT LE REGIME DE CONDUITE. EN MODE MANUEL LE PILOTE GARDE LE
 * CONTROLE COMPLET ET AUCUN VETO N'EST APPLIQUE. EN MODE AUTONOME LE
 * VEHICULE REFUSE D'AVANCER VERS UN OBSTACLE.
 *
 * PARAMETRE :
 * mode — MODE_MANUEL OU MODE_AUTONOME
 *
 * RETOUR :
 * AUCUN
 */
void Securite_DefinirMode(int mode);

/*
 * Securite_ObtenirMode
 *
 * RETOURNE LE REGIME DE CONDUITE COURANT.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * MODE_MANUEL OU MODE_AUTONOME
 */
int Securite_ObtenirMode(void);

/*
 * Securite_DefinirVitesse
 *
 * POINT DE PASSAGE UNIQUE DES COMMANDES MOTEUR. APPLIQUE LE VETO PUIS
 * TRANSMET A LA CARTE MOTEUR. TOUTE COMMANDE, QUELLE QUE SOIT SON
 * ORIGINE, DOIT PASSER PAR ICI PLUTOT QUE PAR Moteurs_DefinirVitesse.
 *
 * PARAMETRES :
 * gauche — CONSIGNE DU COTE GAUCHE (-100 A 100)
 * droite — CONSIGNE DU COTE DROIT (-100 A 100)
 *
 * RETOUR :
 * AUCUN
 */
void Securite_DefinirVitesse(int gauche, int droite);

/*
 * Securite_Arreter
 *
 * ARRET IMMEDIAT DES MOTEURS. TOUJOURS AUTORISE, QUEL QUE SOIT LE MODE
 * ET L'ETAT DU VETO.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Securite_Arreter(void);

/*
 * Securite_VetoActif
 *
 * INDIQUE SI LE VETO EST EN VIGUEUR, C'EST-A-DIRE SI UNE COMMANDE
 * D'AVANCE SERAIT REFUSEE MAINTENANT.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI L'AVANCE EST INTERDITE, false SINON
 */
bool Securite_VetoActif(void);

/*
 * Securite_DefinirManoeuvre
 *
 * SIGNALE QU'UNE MANOEUVRE AUTOMATIQUE OCCUPE LE VEHICULE. TANT QU'ELLE
 * EST EN COURS, LES CONSIGNES EXTERIEURES — JOYSTICK ET SUIVI DE LIGNE —
 * SONT IGNOREES POUR NE PAS LA PERTURBER ENTRE DEUX DE SES ETAPES.
 *
 * PARAMETRE :
 * actif — true AU DEBUT DE LA MANOEUVRE, false A LA FIN
 *
 * RETOUR :
 * AUCUN
 */
void Securite_DefinirManoeuvre(bool actif);

/*
 * Securite_ManoeuvreEnCours
 *
 * INDIQUE SI UNE MANOEUVRE AUTOMATIQUE EST EN COURS.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * true SI UNE MANOEUVRE OCCUPE LE VEHICULE, false SINON
 */
bool Securite_ManoeuvreEnCours(void);

#endif
