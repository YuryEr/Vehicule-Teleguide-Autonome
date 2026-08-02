#include "bus_i2c.h"
#include "config.h"
#include <Wire.h>

// Seules les adresses connues du projet sont sondees. Un balayage
// complet (1 a 126) serait plus lent : sur un bus sans peripherique
// les lignes flottent et chaque adresse absente part en timeout.
static const uint8_t adressesConnues[] = {
    ADRESSE_MOTEUR,
    ADRESSE_GYRO,
    ADRESSE_LIDAR,
};

#define NB_ADRESSES_I2C  (sizeof(adressesConnues) / sizeof(adressesConnues[0]))

static bool presence[NB_ADRESSES_I2C] = { false };

void BusI2C_Scanner(void) {
    for (uint8_t i = 0; i < NB_ADRESSES_I2C; i++) {
        Wire1.beginTransmission(adressesConnues[i]);
        presence[i] = (Wire1.endTransmission() == 0);
    }
}

bool BusI2C_EstPresent(uint8_t adresse) {
    for (uint8_t i = 0; i < NB_ADRESSES_I2C; i++) {
        if (adressesConnues[i] == adresse) return presence[i];
    }
    return false;   // adresse non declaree : traitee comme absente
}

void BusI2C_Tracer(void) {
    for (uint8_t i = 0; i < NB_ADRESSES_I2C; i++) {
        Serial.print("[i2c] 0x");
        Serial.print(adressesConnues[i], HEX);
        Serial.println(presence[i] ? " : present" : " : absent");
    }
}