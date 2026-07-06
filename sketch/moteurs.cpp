#include "moteurs.h"
#include "config.h"
#include <Wire.h>

static void I2C_Ecrire(uint8_t addr, uint8_t reg,
                        uint8_t *val, unsigned int len) {
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    for (unsigned int i = 0; i < len; i++) Wire1.write(val[i]);
    Wire1.endTransmission();
}

static bool I2C_Lire(uint8_t addr, uint8_t reg,
                      uint8_t *val, unsigned int len) {
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    Wire1.requestFrom(addr, len);
    unsigned int i = 0;
    while (Wire1.available() && i < len) val[i++] = Wire1.read();
    return (i == len);
}

void Moteurs_Initialiser(void) {
    uint8_t type = TYPE_JGB37_520;
    uint8_t polarite = 0;
    I2C_Ecrire(ADRESSE_MOTEUR, REG_TYPE_MOTEUR, &type, 1);
    delay(5);
    I2C_Ecrire(ADRESSE_MOTEUR, REG_POLARITE_ENCODEUR, &polarite, 1);
    delay(100);
}

void Moteurs_DefinirVitesse(int gauche, int droite) {
    gauche = constrain(gauche, -100, 100);
    droite = constrain(droite, -100, 100);
    int8_t cmd[4] = { (int8_t)gauche, (int8_t)droite, 0, 0 };
    I2C_Ecrire(ADRESSE_MOTEUR, REG_VITESSE_FIXE, (uint8_t*)cmd, 4);
}

void Moteurs_Arreter(void) {
    Moteurs_DefinirVitesse(0, 0);
}

int32_t Moteurs_LireEncodeurGauche(void) {
    uint8_t buf[16];
    int32_t enc[4];
    if (!I2C_Lire(ADRESSE_MOTEUR, REG_ENCODEUR_TOTAL, buf, 16)) return 0;
    memcpy(enc, buf, sizeof(enc));
    return enc[0];
}

float Moteurs_PulsesEnMetres(long pulses) {
    float circonference = (DIAMETRE_ROUE_MM / 1000.0f) * PI;
    return ((float)pulses / IMPULSIONS_PAR_ROUE) * circonference;
}