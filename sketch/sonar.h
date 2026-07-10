// =============================================================
//  sonar.h  —  Interface du module sonar HC-SR04
//  Vehicule miniature autonome teleguide
//
//  Dépendance : NewPing.h / NewPing.cpp (dans le même dossier)
//  Plateforme  : Arduino UNO Q (core Zephyr, pas de registres AVR)
//               Méthodes dispo : ping(), ping_cm(), ping_median(),
//               convert_cm()  — PAS de ping_timer() ni timer_*()
//
//  Utilisation par l'intégrateur :
//    1. Copier sonar.h, sonar.cpp, NewPing.h, NewPing.cpp
//       dans le dossier du sketch principal.
//    2. #include "sonar.h"  dans le sketch principal.
//    3. Appeler sonar_init() dans setup(), puis les fonctions
//       ci-dessous dans loop() ou la tâche voulue.
// =============================================================

#ifndef SONAR_H
#define SONAR_H

#include <Arduino.h>

// ---------- Brochage (modifiable ici si besoin) --------------
#define SONAR_TRIGGER_PIN   12   // TRIG du HC-SR04
#define SONAR_ECHO_PIN      11   // ECHO du HC-SR04
                                 // (5 V → pont diviseur vers 3,3 V !)
#define SONAR_DISTANCE_MAX 200   // Portée max utile (cm)
#define SONAR_SEUIL_MIN      5   // Distance minimale avant alarme (cm)

// ---------- Initialisation -----------------------------------
// À appeler UNE SEULE FOIS dans setup().
void sonar_init();

// ---------- Mesures ------------------------------------------

// Temps aller-retour de l'echo en microsecondes.
// Retourne 0 si aucun écho (objet hors portée).
unsigned long GetTempsEchoUs();

// Mesure unique rapide (1 tir), en cm.
// Retourne SONAR_DISTANCE_MAX si rien n'est détecté.
// Utile pour un balayage radar à haute fréquence.
int GetDistanceUneFoiseCm();

// Mesure filtrée (médiane de 5 tirs), en cm.
// Retourne SONAR_DISTANCE_MAX si rien n'est détecté.
// Méthode recommandée pour la navigation autonome.
int GetDistanceCm();

// ---------- Logique d'évitement ------------------------------

// Retourne true si un obstacle se trouve à distance <= seuilCm.
// Exemple : ObstacleEnAvant(SONAR_SEUIL_MIN)
bool ObstacleEnAvant(int seuilCm);

#endif // SONAR_H
