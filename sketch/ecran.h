#ifndef ECRAN_H
#define ECRAN_H

#include <Arduino.h>

/*
 * Ecran_Initialiser
 *
 * PREPARE L'ECRAN ILI9341 (SPI MATERIEL, 320x240, ORIENTATION PAYSAGE)
 * ET AFFICHE LA PAGE D'ATTENTE. A APPELER UNE FOIS DANS setup().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_Initialiser(void);

/*
 * Ecran_AfficherAttente
 *
 * AFFICHE UN MESSAGE D'ATTENTE. LE MCU DEMARRE AVANT LE CONTENEUR
 * PYTHON ET NE CONNAIT PAS ENCORE LES PARAMETRES DU RESEAU.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherAttente(void);

/*
 * Ecran_AfficherConnexion
 *
 * AFFICHE LA PAGE DE CONNEXION : LE CODE QR D'ADHESION AU RESEAU ET
 * CELUI QUI OUVRE LA PAGE DE CONTROLE, COTE A COTE, AVEC LES MEMES
 * INFORMATIONS EN CLAIR POUR UNE SAISIE MANUELLE.
 *
 * PARAMETRES :
 * ssid : NOM DU RESEAU
 * mdp  : MOT DE PASSE DU RESEAU
 * ip   : ADRESSE IP DU SERVEUR SUR CE RESEAU
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_AfficherConnexion(const char *ssid, const char *mdp,
                              const char *ip);

/*
 * Ecran_DefinirAdresse
 *
 * ENREGISTRE L'ADRESSE IP FOURNIE PAR LE MPU. NE DESSINE RIEN : LE TRACE
 * A LIEU AU PROCHAIN Ecran_MettreAJour. UNE ADRESSE IDENTIQUE A CELLE DEJA
 * AFFICHEE EST IGNOREE.
 *
 * PARAMETRE :
 * ip : ADRESSE IPv4 EN NOTATION POINTEE
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_DefinirAdresse(const char *ip);

/*
 * Ecran_MettreAJour
 *
 * REDESSINE LA PAGE DE CONNEXION SI L'ADRESSE A CHANGE. SANS EFFET TANT
 * QU'AUCUN CHANGEMENT N'EST EN ATTENTE. A APPELER A CHAQUE ITERATION DE
 * loop().
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Ecran_MettreAJour(void);

#endif
