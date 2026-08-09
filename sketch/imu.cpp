#include "imu.h"
#include "bus_i2c.h"
#include "config.h"
#include <Wire.h>

static float offsetZ = 0.0f;

// Seul l'axe Z est lu : deux octets au lieu des six de la rafale complete,
// ce qui allege d'autant le bus a cadence d'echantillonnage elevee.
static float lireGyroZBrut(void) {
    Wire1.beginTransmission(ADRESSE_GYRO);
    Wire1.write(REG_GYRO_ZOUT_H);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint8_t)ADRESSE_GYRO, (uint8_t)2);
    int16_t rawZ = Wire1.read() << 8 | Wire1.read();
    return rawZ / SENSIBILITE_GYRO;
}

static void ecrireRegistre(uint8_t registre, uint8_t valeur) {
    Wire1.beginTransmission(ADRESSE_GYRO);
    Wire1.write(registre);
    Wire1.write(valeur);
    Wire1.endTransmission();
}

void Imu_Initialiser(void) {
    if (!Imu_EstPresent()) return;

    ecrireRegistre(REG_PWR_MGMT_1, 0x00);      // sortie de veille
    delay(100);

    // Le capteur demarre avec une bande passante de 256 Hz alors que le
    // chassis tourne a quelques degres par seconde : sans filtre, la mesure
    // porte surtout la vibration des chenilles, que l'integration accumule en
    // derive de cap sans qu'aucun reglage aval ne puisse l'en distinguer.
    ecrireRegistre(REG_CONFIG, DLPF_20HZ);
    ecrireRegistre(REG_SMPRT_DIV, SMPRT_DIV_200HZ);

    // Pleine echelle +/-250 deg/s, la plus sensible, coherente avec
    // SENSIBILITE_GYRO. Le chassis ne depasse pas quelques dizaines de deg/s.
    ecrireRegistre(REG_GYRO_CONFIG, 0x00);
    delay(50);
}

// Moyenne n echantillons pour etablir le zero. La dispersion est surveillee :
// si elle depasse le seuil, le vehicule n'etait pas immobile et le zero
// precedent, lui, avait ete mesure dans de bonnes conditions.
static void calibrer(int n, unsigned long pauseMs) {
    // Sans ce garde-fou, les transactions partiraient en timeout sur un bus
    // sans capteur, bloquant l'appelant pendant plus d'une seconde.
    if (!Imu_EstPresent()) return;

    float somme = 0.0f;
    float mini  = 0.0f;
    float maxi  = 0.0f;

    for (int i = 0; i < n; i++) {
        float brut = lireGyroZBrut();
        somme += brut;
        if (i == 0 || brut < mini) mini = brut;
        if (i == 0 || brut > maxi) maxi = brut;
        delay(pauseMs);
    }

    if ((maxi - mini) > IMU_CAL_DISPERSION_MAX) return;
    offsetZ = somme / n;
}

void Imu_Calibrer(void)       { calibrer(IMU_CAL_ECHANTILLONS, 5); }
void Imu_CalibrerRapide(void) { calibrer(IMU_CAL_ECHANTILLONS_RAPIDE, 2); }

float Imu_LireGyroZ(void) {
    if (!Imu_EstPresent()) return 0.0f;
    return lireGyroZBrut() - offsetZ;
}

bool Imu_EstPresent(void) { return BusI2C_EstPresent(ADRESSE_GYRO); }
