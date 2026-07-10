/*
 * ============================================================
 *  TFLunaScan.h
 *  TF Luna LiDAR (I2C) + servo SG90 sur Arduino UNO Q (cote MCU)
 *  Cartographie frontale pour vehicule autonome teleguide
 * ============================================================
 *
 *  CABLAGE (I2C)
 *  -------------
 *    TF Luna (6 broches)        UNO Q
 *    -------------------        -----
 *    1 VCC          --------->  5V (ou 3.3V, le TF Luna marche aussi en 3.3V)
 *    2 SDA          --------->  SDA   (ou Qwiic)
 *    3 SCL          --------->  SCL   (ou Qwiic)
 *    4 GND          --------->  GND
 *    5 Config       --------->  GND   <-- OBLIGATOIRE pour activer le mode I2C !
 *    6 Data-ready   --------->  (optionnel : voir note "trigger / pin 6" dans le .cpp)
 *
 *  - Broche 5 a la masse = mode I2C (sinon le TF Luna reste en UART).
 *  - Logique I2C en 3.3V -> compatible direct avec l'UNO Q, aucun
 *    adaptateur de niveau necessaire.
 *  - Le connecteur Qwiic de l'UNO Q fournit SDA/SCL/3.3V/GND, mais
 *    il faut QUAND MEME relier la broche 5 du TF Luna a GND a part.
 *
 *  REPERE / GEOMETRIE
 *  -------------------
 *  Origine = vehicule. Lidar centre en X (x=0), decale en
 *  profondeur via decalageZ (cm, signe).
 *      Z = profondeur :  Z = decalageZ + d*cos(beam)
 *      X = lateral    :  X = d*sin(beam)
 *  Servo : 90 deg = droit devant.  beam = servo - 90.
 *  Un SG90 (180 deg) couvre les deux quadrants AVANT.
 *  (Inverse le signe de X si ton servo tourne dans l'autre sens.)
 * ============================================================
 */

#ifndef TF_LUNA_SCAN_H
#define TF_LUNA_SCAN_H

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

// ---------- TF Luna I2C ----------
#define TFL_ADDR            0x10     // adresse esclave par defaut
#define TFL_REG_DIST_LOW    0x00     // 6 registres a lire : dist L/H, amp L/H, temp L/H

// ---------- Servo ----------
#define SERVO_PIN             9
#define SERVO_CENTRE          90       // angle "droit devant"
#define SG90_VITESSE_MAX_DPS  300      // vitesse max prudente du SG90 (deg/s) pour rester fluide

// ---------- Validite des lectures ----------
#define TFLUNA_FORCE_MIN    100      // en-dessous : signal trop faible -> rejet
#define TFLUNA_MAX_CM       800      // portee fiable max (~8 m)

// ---------- Variables globales (definies dans TFLunaScan.cpp) ----------
extern Servo lidarServo;
extern int   courantAngleServoDeg;

// ---------- Structure de sortie ----------
struct Lidar_Echantillon {
  int      angleServoDeg;           // angle servo (0..180)
  int      angleFaisceauDeg;        // angle faisceau / avant (angleServoDeg - 90)
  float    distanceCm;              // distance BRUTE le long du faisceau
  float    x;                       // position laterale (cm), origine = vehicule
  float    z;                       // profondeur (cm) depuis l'origine (inclut decalageZ)
  uint16_t force;                   // force du signal (amp)
  float    temperatureC;            // temperature de la puce (C)
  bool     valide;                  // lecture fiable ?
};

// ============================================================
//  Lecture bas niveau / validite
// ============================================================
bool TfLunaLireI2C(float &distCm, uint16_t &force, float &tempC);
bool LectureValide(float distCm, uint16_t force);

// ============================================================
//  Servo / echantillonnage
// ============================================================
void DeplaceServo(int angleCibleDeg, float vitesseDps);
Lidar_Echantillon ConstruEchanti(int angleServoDeg, float decalageZ);
Lidar_Echantillon LireAPos(int angleServoDeg, float decalageZ,
                           float vitesseDps = 200, uint16_t fixageMs = 40);

// ============================================================
//  FONCTION 1 : balayage fluide de l'environnement
// ============================================================
int BalayageEnvironnement(Lidar_Echantillon *sortie, int capacite,
                           int angleMax, float vitesseDps, float decalageZ,
                           int pasDeg = 2, uint16_t fixageMs = 25);

// ============================================================
//  FONCTION 2 : pointer vers une distance X donnee
// ============================================================
Lidar_Echantillon PointerEnX(float cibleX, float posZAttendu, float decalageZ,
                              float vitesseDps = 200);

Lidar_Echantillon PointerEnXRecherche(float targetX, float decalageZ, float vitesseDps,
                                       int angleMax, int pasDeg,
                                       Lidar_Echantillon *buf, int capacite);

// ============================================================
//  FONCTION 3 : autres fonctions utiles au projet
// ============================================================
int  TrouverObstacleProche(Lidar_Echantillon *s, int n);
int  TrouverVoieDegage(Lidar_Echantillon *s, int n);
bool ObstacleEnAvant(Lidar_Echantillon *s, int n, int demiAngle, float seuilCm);
void CentrerLidar(float vitesseDps = 200);
bool tfLunaPresent();
void printSample(const Lidar_Echantillon &s);

#endif // TF_LUNA_SCAN_H
