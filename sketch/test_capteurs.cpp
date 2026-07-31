#include "test_capteurs.h"
#include "ultrason.h"
#include "lidar.h"
#include "servo_lidar.h"
#include "obstacle.h"

#ifdef TEST_CAPTEURS_ACTIF

#define PERIODE_TEST_CAPTEURS_MS  500

// Affiche une distance, ou l'etat du capteur si la mesure est indisponible.
static void tracerDistance(int cm, bool present) {
    if (!present) {
        Serial.print("absent");
    } else if (cm < 0) {
        Serial.print("invalide");
    } else {
        Serial.print(cm);
        Serial.print(" cm");
    }
}

void TestCapteurs_MettreAJour(void) {
    static unsigned long tPrecedent = 0;
    unsigned long maintenant = millis();
    if (maintenant - tPrecedent < PERIODE_TEST_CAPTEURS_MS) return;
    tPrecedent = maintenant;

    Serial.print("[capteurs] ultrason = ");
    tracerDistance(Ultrason_DistanceCm(), Ultrason_EstPresent());
    Serial.print(" | lidar = ");
    tracerDistance(Lidar_DistanceCm(), Lidar_EstPresent());
    Serial.print(" | servo = ");
    Serial.print(ServoLidar_AngleActuel());
    Serial.println(" deg");

    // Distance frontale fusionnee, puis les trois secteurs du dernier
    // sondage : "-" quand aucune mesure exploitable n'est disponible.
    Serial.print("[fusion] frontal = ");
    Serial.print(Obstacle_DistanceFrontaleCm());
    Serial.print(" cm");
    if (Obstacle_EstDetecte()) Serial.print("  << OBSTACLE");

    Serial.print(" | secteurs G/C/D = ");
    for (int secteur = SECTEUR_GAUCHE; secteur <= SECTEUR_DROITE; secteur++) {
        int distance = Obstacle_DistanceSecteur(secteur);
        if (secteur > SECTEUR_GAUCHE) Serial.print("/");
        if (distance < 0) Serial.print("-");
        else              Serial.print(distance);
    }
    Serial.println();
}

#else

void TestCapteurs_MettreAJour(void) { }

#endif
