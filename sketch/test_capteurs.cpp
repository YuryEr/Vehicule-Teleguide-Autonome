#include "test_capteurs.h"
#include "ultrason.h"
#include "lidar.h"
#include "servo_lidar.h"

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
}

#else

void TestCapteurs_MettreAJour(void) { }

#endif
