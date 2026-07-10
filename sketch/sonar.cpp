// =============================================================
//  sonar.cpp  —  Implémentation du module sonar HC-SR04
//  Vehicule miniature autonome teleguide
// =============================================================

#include "sonar.h"
#include "NewPing.h"   // Guillemets : fichier local dans le dossier du sketch

// Instance NewPing (privée à ce module, non visible par l'intégrateur)
static NewPing sonar(SONAR_TRIGGER_PIN, SONAR_ECHO_PIN, SONAR_DISTANCE_MAX);


// ============================================================
//  sonar_init()
//  À appeler dans setup() du sketch principal.
//  Initialise la communication série pour le débogage.
// ============================================================
void sonar_init() {
  // Aucune initialisation matérielle nécessaire pour NewPing
  // (le constructeur s'en charge). Réservé pour extensions futures
  // (ex. : servo de balayage, calibration, etc.)
  Serial.println(F("[Sonar] Module HC-SR04 pret (NewPing local)."));
}


// ============================================================
//  GetTempsEchoUs()
//  Temps aller-retour de l'écho, en microsecondes.
//  Retourne 0 si aucun écho (objet hors portée).
// ============================================================
unsigned long GetTempsEchoUs() {
  return sonar.ping();   // us aller-retour, 0 si hors portée
}


// ============================================================
//  ReadDistanceOnceCm()
//  Mesure UNIQUE rapide (1 tir). Utile pour le balayage radar.
//  Retourne la distance en cm, ou SONAR_DISTANCE_MAX si rien.
// ============================================================
int GetDistanceUneFoiseCm() {
  unsigned int cm = sonar.ping_cm();   // 0 si rien détecté
  if (cm == 0 || cm > SONAR_DISTANCE_MAX) return SONAR_DISTANCE_MAX;
  return (int)cm;
}


// ============================================================
//  GetDistanceCm()
//  Mesure filtrée : médiane de 5 tirs pour réduire les parasites.
//  Retourne la distance en cm, ou SONAR_DISTANCE_MAX si rien.
// ============================================================
int GetDistanceCm() {
  unsigned int us = sonar.ping_median(5);  // médiane de 5 tirs, en µs
  unsigned int cm = sonar.convert_cm(us);  // conversion µs → cm
  if (cm == 0 || cm > SONAR_DISTANCE_MAX) return SONAR_DISTANCE_MAX;
  return (int)cm;
}


// ============================================================
//  ObstacleEnAvant(seuilCm)
//  Retourne true si un obstacle est détecté à <= seuilCm.
// ============================================================
bool ObstacleEnAvant(int seuilCm) {
  return GetDistanceCm() <= seuilCm;
}
