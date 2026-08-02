#ifndef MOTEURS_H
#define MOTEURS_H

#include <Arduino.h>

/*
 * Moteurs_Initialiser
 *
 * CONFIGURE LA CARTE MOTEUR HIWONDER (TYPE JGB37-520,
 * POLARITE ENCODEUR) VIA I2C SUR LE BUS WIRE1.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Moteurs_Initialiser(void);

/*
 * Moteurs_DefinirVitesse
 *
 * APPLIQUE UNE CONSIGNE DE VITESSE AUX MOTEURS GAUCHE ET DROITE.
 * LES VALEURS SONT BORNEES ENTRE -100 ET 100.
 *
 * PARAMETRES :
 * gauche — VITESSE DES MOTEURS COTE GAUCHE (-100 A 100)
 * droite — VITESSE DES MOTEURS COTE DROIT (-100 A 100)
 *
 * RETOUR :
 * AUCUN
 */
void Moteurs_DefinirVitesse(int gauche, int droite);

/*
 * Moteurs_Arreter
 *
 * ARRETE TOUS LES MOTEURS (VITESSE = 0).
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * AUCUN
 */
void Moteurs_Arreter(void);

/*
 * Moteurs_LireEncodeurGauche
 *
 * LIT LE COMPTEUR TOTAL D'IMPULSIONS DE L'ENCODEUR DU MOTEUR
 * GAUCHE DEPUIS LA CARTE HIWONDER.
 *
 * PARAMETRE :
 * AUCUN
 *
 * RETOUR :
 * NOMBRE D'IMPULSIONS (SIGNE)
 */
int32_t Moteurs_LireEncodeurGauche(void);

/*
 * Moteurs_PulsesEnMetres
 *
 * CONVERTIT UN NOMBRE D'IMPULSIONS ENCODEUR EN DISTANCE (METRES).
 * FORMULE : (pulses / IMPULSIONS_PAR_ROUE) * CIRCONFERENCE_ROUE
 *
 * PARAMETRE :
 * pulses — NOMBRE D'IMPULSIONS
 *
 * RETOUR :
 * DISTANCE EN METRES
 */
float Moteurs_PulsesEnMetres(long pulses);

// RETOURNE VRAI SI LA CARTE MOTEUR HIWONDER A REPONDU SUR LE BUS I2C.
bool Moteurs_EstPresent(void);

#endif