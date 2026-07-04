/*
 * sketch.ino — Contrôle temps réel du châssis (MCU STM32U585 / Zephyr)
 *
 * Responsabilités (temps réel) :
 *   - Pilotage manuel des moteurs via joystick (Bridge : joy_x / joy_y)
 *   - Déplacements asservis en boucle fermée pour le mode blocs :
 *       * avancer_metres : odométrie par encodeurs magnétiques
 *       * tourner_degres : intégration de la vitesse angulaire (gyro MPU-6050)
 *   - Machine à états NON bloquante : le Bridge reste disponible pendant
 *     le mouvement (permet l'arrêt d'urgence et évite les timeouts RPC).
 *
 * Matériel :
 *   - Carte moteur Hiwonder (I2C 0x34, bus Wire1/Qwiic)
 *   - IMU MPU-6050 (I2C 0x68, bus Wire1) pour la rotation
 *   - Encodeurs magnétiques JGB37-520 (44 impulsions/tour moteur)
 */

#include <Arduino_RouterBridge.h>
#include <Wire.h>

// ============================================================
// CONSTANTES MATÉRIELLES
// ============================================================
#define MOTOR_ADDR                       0x34
#define MOTOR_TYPE_ADDR                  20
#define MOTOR_ENCODER_POLARITY_ADDR      21
#define MOTOR_FIXED_SPEED_ADDR           51
#define MOTOR_ENCODER_TOTAL_ADDR         60
#define MOTOR_TYPE_JGB37_520_12V_110RPM  3

#define GYRO_ADDR         0x68
#define GYRO_PWR_MGMT_1   0x6B
#define GYRO_XOUT_H       0x43
#define GYRO_SENSITIVITY  131.0   // LSB par °/s (±250 °/s)

// ============================================================
// CALIBRATION (valeurs validées expérimentalement — équipe Vlad/Yury)
// ============================================================
#define WHEEL_DIAMETER_MM     65.0
#define GEARBOX_RATIO         50.0
#define PULSES_PER_MOTOR_REV  44.0
#define PULSES_PER_WHEEL_REV  (PULSES_PER_MOTOR_REV * GEARBOX_RATIO)

#define VITESSE_DEPLACEMENT   12    // consigne moteur pour avancer/reculer
#define VITESSE_ROTATION      12    // consigne moteur pour tourner
#define SENS_AVANT            -1    // consigne calibrée : avant = négatif

// Compensation coasting rotation : on arrête 10° avant, ralenti sous 20°
#define ROT_MARGE_ARRET_DEG   10.0
#define ROT_MARGE_LENTE_DEG   20.0

// Sécurités anti-blocage (ms)
#define TIMEOUT_AVANCE_MS     12000
#define TIMEOUT_ROTATION_MS   8000

// ============================================================
// ÉTAT DE LA MACHINE À MOUVEMENT
// ============================================================
enum EtatMouvement { INACTIF, AVANCE, ROTATION };
volatile EtatMouvement etatMouvement = INACTIF;

// Contexte « avancer »
long   encodeurDepart   = 0;
float  distanceCible    = 0.0f;
int    sensAvance       = SENS_AVANT;

// Contexte « tourner »
float  angleCumule      = 0.0f;
float  angleCibleAbs    = 0.0f;
int    signeRotation    = 1;       // +1 gauche, -1 droite
bool   rotationLente    = false;
unsigned long tPrecRotation = 0;

// Commun
unsigned long tDebutMouvement = 0;
float  gyroOffsetZ      = 0.0f;

// ============================================================
// I2C (bus Wire1 / Qwiic)
// ============================================================
void I2C_Write(uint8_t addr, uint8_t reg, uint8_t *val, unsigned int len) {
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    for (unsigned int i = 0; i < len; i++) Wire1.write(val[i]);
    Wire1.endTransmission();
}

bool I2C_Read(uint8_t addr, uint8_t reg, uint8_t *val, unsigned int len) {
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    Wire1.requestFrom(addr, len);
    unsigned int i = 0;
    while (Wire1.available() && i < len) val[i++] = Wire1.read();
    return (i == len);
}

// ============================================================
// MOTEURS
// ============================================================
void Moteurs_Init() {
    uint8_t motorType = MOTOR_TYPE_JGB37_520_12V_110RPM;
    uint8_t polarity  = 0;
    I2C_Write(MOTOR_ADDR, MOTOR_TYPE_ADDR, &motorType, 1);
    delay(5);
    I2C_Write(MOTOR_ADDR, MOTOR_ENCODER_POLARITY_ADDR, &polarity, 1);
    delay(100);
}

void Moteurs_SetVitesse(int gauche, int droite) {
    gauche = constrain(gauche, -100, 100);
    droite = constrain(droite, -100, 100);
    int8_t cmd[4] = { (int8_t)gauche, (int8_t)droite, 0, 0 };
    I2C_Write(MOTOR_ADDR, MOTOR_FIXED_SPEED_ADDR, (uint8_t*)cmd, 4);
}

void Moteurs_Stop() { Moteurs_SetVitesse(0, 0); }

// ============================================================
// ENCODEURS
// ============================================================
int32_t Encodeur_LireMoteurGauche() {
    uint8_t  buf[16];
    int32_t  enc[4];
    if (!I2C_Read(MOTOR_ADDR, MOTOR_ENCODER_TOTAL_ADDR, buf, 16)) return 0;
    memcpy(enc, buf, sizeof(enc));
    return enc[0];
}

float Encodeur_PulsesEnMetres(long pulses) {
    float circonference = (WHEEL_DIAMETER_MM / 1000.0f) * PI;
    return ((float)pulses / PULSES_PER_WHEEL_REV) * circonference;
}

// ============================================================
// IMU (MPU-6050)
// ============================================================
void Imu_Init() {
    Wire1.beginTransmission(GYRO_ADDR);
    Wire1.write(GYRO_PWR_MGMT_1);
    Wire1.write(0x00);            // réveil du capteur
    Wire1.endTransmission();
    delay(100);
}

float Imu_LireGyroZ() {
    Wire1.beginTransmission(GYRO_ADDR);
    Wire1.write(GYRO_XOUT_H);
    Wire1.endTransmission(false);
    Wire1.requestFrom(GYRO_ADDR, 6);
    Wire1.read(); Wire1.read();   // saute X
    Wire1.read(); Wire1.read();   // saute Y
    int16_t rawZ = Wire1.read() << 8 | Wire1.read();
    return rawZ / GYRO_SENSITIVITY;
}

void Imu_Calibrer() {
    float somme = 0.0f;
    const int n = 200;
    for (int i = 0; i < n; i++) { somme += Imu_LireGyroZ(); delay(5); }
    gyroOffsetZ = somme / n;
}

// ============================================================
// PILOTAGE MANUEL (Bridge : joy_x / joy_y)
// ============================================================
volatile float joystick_x = 0.0f;

void Manuel_JoystickX(float x) { joystick_x = x; }

void Manuel_JoystickY(float y) {
    if (etatMouvement != INACTIF) return;   // ignoré pendant un bloc
    int gauche = (int)((y - joystick_x) * 30.0f);
    int droite = (int)((y + joystick_x) * 30.0f);
    Moteurs_SetVitesse(gauche, droite);
}

// ============================================================
// DÉPLACEMENTS ASSERVIS (Bridge, machine à états NON bloquante)
// ============================================================

/*
 * Deplacement_AvancerMetres
 *
 * Démarre un déplacement rectiligne asservi sur les encodeurs.
 * Retourne immédiatement ; la progression est gérée dans loop().
 *
 * PARAMETRE :
 * distance - distance en mètres (positif = avant, négatif = arrière)
 *
 * RETOUR :
 * 1 (mouvement démarré)
 */
int Deplacement_AvancerMetres(float distance) {
    encodeurDepart   = Encodeur_LireMoteurGauche();
    distanceCible    = fabs(distance);
    sensAvance       = (distance >= 0) ? SENS_AVANT : -SENS_AVANT;
    tDebutMouvement  = millis();
    etatMouvement    = AVANCE;
    Moteurs_SetVitesse(sensAvance * VITESSE_DEPLACEMENT,
                       sensAvance * VITESSE_DEPLACEMENT);
    return 1;
}

/*
 * Deplacement_TournerDegres
 *
 * Démarre une rotation sur place asservie sur le gyroscope.
 * Retourne immédiatement ; la progression est gérée dans loop().
 *
 * PARAMETRE :
 * angle - angle en degrés (positif = gauche, négatif = droite)
 *
 * RETOUR :
 * 1 (mouvement démarré)
 */
int Deplacement_TournerDegres(float angle) {
    angleCumule     = 0.0f;
    angleCibleAbs   = fabs(angle);
    signeRotation   = (angle >= 0) ? 1 : -1;
    rotationLente   = false;
    tPrecRotation   = millis();
    tDebutMouvement = millis();
    etatMouvement   = ROTATION;
    Moteurs_SetVitesse(signeRotation *  VITESSE_ROTATION,
                       signeRotation * -VITESSE_ROTATION);
    return 1;
}

/*
 * Deplacement_Arreter
 *
 * Arrêt d'urgence : stoppe les moteurs et libère la machine à états.
 * Appelable à tout moment (utilisé par le bouton STOP du mode blocs).
 *
 * RETOUR : 1
 */
int Deplacement_Arreter() {
    Moteurs_Stop();
    etatMouvement = INACTIF;
    return 1;
}

/*
 * Deplacement_EstActif
 *
 * RETOUR : 1 si un déplacement asservi est en cours, sinon 0.
 * (interrogé par Python pour savoir quand passer au bloc suivant)
 */
int Deplacement_EstActif() {
    return (etatMouvement != INACTIF) ? 1 : 0;
}

// Fait progresser la machine à états d'un pas (appelé dans loop())
void Deplacement_MettreAJour() {
    if (etatMouvement == INACTIF) return;

    if (etatMouvement == AVANCE) {
        long delta      = labs(Encodeur_LireMoteurGauche() - encodeurDepart);
        float distance  = Encodeur_PulsesEnMetres(delta);
        bool timeout    = (millis() - tDebutMouvement) > TIMEOUT_AVANCE_MS;
        if (distance >= distanceCible || timeout) {
            Moteurs_Stop();
            etatMouvement = INACTIF;
        }
    }
    else if (etatMouvement == ROTATION) {
        unsigned long tMaint = millis();
        float dt = (tMaint - tPrecRotation) / 1000.0f;
        tPrecRotation = tMaint;

        float vitesseZ = Imu_LireGyroZ() - gyroOffsetZ;
        angleCumule   += vitesseZ * dt;

        float reste  = angleCibleAbs - fabs(angleCumule);
        bool timeout = (millis() - tDebutMouvement) > TIMEOUT_ROTATION_MS;

        if (!rotationLente && reste < ROT_MARGE_LENTE_DEG) {
            rotationLente = true;
            int lente = VITESSE_ROTATION / 2;
            Moteurs_SetVitesse(signeRotation *  lente,
                               signeRotation * -lente);
        }
        if (fabs(angleCumule) >= (angleCibleAbs - ROT_MARGE_ARRET_DEG) || timeout) {
            Moteurs_Stop();
            etatMouvement = INACTIF;
        }
    }
}

// ============================================================
// DIAGNOSTIC
// ============================================================
const char* Scan_I2C() {
    static String resultat;
    resultat = "";
    bool trouve = false;
    for (byte addr = 1; addr < 127; addr++) {
        Wire1.beginTransmission(addr);
        if (Wire1.endTransmission() == 0) {
            resultat += "0x"; resultat += String(addr, HEX); resultat += " ";
            trouve = true;
        }
    }
    if (!trouve) resultat = "aucun";
    return resultat.c_str();
}

// ============================================================
// SETUP / LOOP
// ============================================================
void setup() {
    Serial.begin(9600);          // debug optionnel (pas de while(!Serial) : robot headless)
    Wire1.begin();
    delay(500);

    Moteurs_Init();
    Imu_Init();
    Imu_Calibrer();              // ~1 s — le véhicule doit être IMMOBILE au démarrage

    Bridge.begin();
    // Pilotage manuel
    Bridge.provide_safe("joy_x", Manuel_JoystickX);
    Bridge.provide_safe("joy_y", Manuel_JoystickY);
    // Mode blocs (asservi)
    Bridge.provide_safe("avancer_metres",    Deplacement_AvancerMetres);
    Bridge.provide_safe("tourner_degres",    Deplacement_TournerDegres);
    Bridge.provide_safe("arreter_mouvement", Deplacement_Arreter);
    Bridge.provide_safe("mouvement_actif",   Deplacement_EstActif);
    // Diagnostic
    Bridge.provide_safe("scan_i2c", Scan_I2C);
}

void loop() {
    Bridge.update();
    Deplacement_MettreAJour();
}