#include "imu.h"
#include "bus_i2c.h"
#include "config.h"
#include <Wire.h>

static float offsetZ = 0.0f;

static float lireGyroZBrut(void) {
    Wire1.beginTransmission(ADRESSE_GYRO);
    Wire1.write(REG_GYRO_XOUT_H);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint8_t)ADRESSE_GYRO, (uint8_t)6);
    Wire1.read(); Wire1.read();
    Wire1.read(); Wire1.read();
    int16_t rawZ = Wire1.read() << 8 | Wire1.read();
    return rawZ / SENSIBILITE_GYRO;
}

void Imu_Initialiser(void) {
    if (!Imu_EstPresent()) return;

    Wire1.beginTransmission(ADRESSE_GYRO);
    Wire1.write(REG_PWR_MGMT_1);
    Wire1.write(0x00);
    Wire1.endTransmission();
    delay(100);
}

void Imu_Calibrer(void) {
    // Sans ce garde-fou, 200 transactions partiraient en timeout sur un
    // bus sans capteur, bloquant setup() pendant plus d'une seconde.
    offsetZ = 0.0f;
    if (!Imu_EstPresent()) return;

    float somme = 0.0f;
    const int n = 200;
    for (int i = 0; i < n; i++) {
        somme += lireGyroZBrut();
        delay(5);
    }
    offsetZ = somme / n;
}

float Imu_LireGyroZ(void) {
    if (!Imu_EstPresent()) return 0.0f;
    return lireGyroZBrut() - offsetZ;
}

bool Imu_EstPresent(void) { return BusI2C_EstPresent(ADRESSE_GYRO); }