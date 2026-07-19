#ifndef CONFIG_H
#define CONFIG_H

// Adresses I2C (bus Wire1 / Qwiic)
#define ADRESSE_MOTEUR        0x34
#define ADRESSE_GYRO          0x68

// Registres carte moteur Hiwonder
#define REG_TYPE_MOTEUR       20
#define REG_POLARITE_ENCODEUR 21
#define REG_VITESSE_FIXE      51
#define REG_ENCODEUR_TOTAL    60
#define TYPE_JGB37_520        3

// Registres MPU-6050
#define REG_PWR_MGMT_1        0x6B
#define REG_GYRO_XOUT_H       0x43
#define SENSIBILITE_GYRO      131.0

// Calibration mecanique
#define DIAMETRE_ROUE_MM      65.0
#define RATIO_REDUCTEUR       50.0
#define IMPULSIONS_PAR_TOUR   44.0
#define IMPULSIONS_PAR_ROUE   (IMPULSIONS_PAR_TOUR * RATIO_REDUCTEUR)

// Vitesses moteur (consignes)
#define VITESSE_DEPLACEMENT   12
#define VITESSE_ROTATION      12
#define VITESSE_JOYSTICK      30

// Compensation rotation
#define ROT_MARGE_ARRET_DEG   10.0
#define ROT_MARGE_LENTE_DEG   20.0

// Timeouts securite (ms)
#define TIMEOUT_AVANCE_MS     12000
#define TIMEOUT_ROTATION_MS   8000

// Bandeaux LED adressables (WS2813, protocole WS2812 / 800 kHz)
#define PIN_BANDEAU_1          6
#define PIN_BANDEAU_2          9
#define NB_LEDS_PAR_BANDEAU    4
#define LUMINOSITE_LEDS        60      // 0-255
#define PERIODE_GYROPHARE_MS   250
#define PERIODE_CLIGNOTANT_MS  400

#endif