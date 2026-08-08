#include "lidar.h"
#include "bus_i2c.h"
#include "config.h"
#include <Wire.h>

static int distanceCache = -1;

// Lit les 6 registres du TF-Luna : dist L/H, force L/H, temp L/H.
// Le TF-Luna partage le bus Qwiic (Wire1) avec la carte moteur (0x34)
// et l'IMU (0x68) ; son adresse 0x10 n'entre pas en conflit.
static bool lireCapteur(int &distCm, uint16_t &force) {
    Wire1.beginTransmission(ADRESSE_LIDAR);
    Wire1.write(REG_LIDAR_DIST);
    if (Wire1.endTransmission() != 0) return false;   // pas d'ACK : capteur absent

    if (Wire1.requestFrom((uint8_t)ADRESSE_LIDAR, (uint8_t)6) < 6) return false;

    uint8_t b[6];
    for (int i = 0; i < 6; i++) b[i] = Wire1.read();

    distCm = (int)(b[0] | (b[1] << 8));
    force  = (uint16_t)(b[2] | (b[3] << 8));
    return true;
}

// Lecture fiable : signal suffisant, pas de saturation, hors zone morte.
static bool lectureValide(int distCm, uint16_t force) {
    if (force < LIDAR_FORCE_MIN) return false;   // surface absorbante / trop loin
    if (force == 65535)          return false;   // saturation (trop proche/reflechissant)
    if (distCm < LIDAR_DISTANCE_MIN || distCm > LIDAR_DISTANCE_MAX) return false;
    return true;
}

void Lidar_Initialiser(void) {
    // Wire1 est deja initialise dans setup(). Le mode I2C du capteur
    // est selectionne materiellement (broche CFG a la masse).
    distanceCache = -1;
}

bool Lidar_EstPresent(void) { return BusI2C_EstPresent(ADRESSE_LIDAR); }

void Lidar_MettreAJour(void) {
    if (!Lidar_EstPresent()) return;   // capteur absent : aucune transaction I2C

    static unsigned long tPrecedent = 0;
    unsigned long maintenant = millis();
    if (maintenant - tPrecedent < LIDAR_PERIODE_MS) return;
    tPrecedent = maintenant;

    int distCm;
    uint16_t force;
    if (lireCapteur(distCm, force) && lectureValide(distCm, force)) {
        distanceCache = distCm;
    } else {
        distanceCache = -1;
    }
}

int Lidar_DistanceCm(void) {
    return distanceCache;
}

bool Lidar_MesurerMaintenant(int &distCm) {
    if (!Lidar_EstPresent()) return false;

    int      lu;
    uint16_t force;
    if (!lireCapteur(lu, force) || !lectureValide(lu, force)) return false;

    // Le cache de Lidar_MettreAJour() n'est pas mis a jour ici : il alimente
    // la RPC lire_lidar_cm, qui decrit la distance frontale. Une mesure prise
    // en cours de sondage correspond a un autre angle.
    distCm = lu;
    return true;
}
