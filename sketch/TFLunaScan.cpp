/*
 * ============================================================
 *  TFLunaScan.cpp
 *  Implementation des fonctions declarees dans TFLunaScan.h
 * ============================================================
 */

#include "TFLunaScan.h"

// ---------- Definitions des variables globales ----------
Servo lidarServo;
int   courantAngleServoDeg = SERVO_CENTRE;


// ============================================================
//  Lecture du TF Luna en I2C
//  On ecrit le registre de depart (0x00), puis on lit 6 octets :
//  dist_L, dist_H, amp_L, amp_H, temp_L, temp_H.
//  En I2C, chaque lecture est deja "fraiche" (requete/reponse).
// ============================================================
bool TfLunaLireI2C(float &distCm, uint16_t &force, float &tempC) {
  Wire.beginTransmission(TFL_ADDR);
  Wire.write(TFL_REG_DIST_LOW);
  if (Wire.endTransmission() != 0) return false;          // pas d'ACK -> capteur absent/mauvais cablage

  uint8_t n = Wire.requestFrom((uint8_t)TFL_ADDR, (uint8_t)6);
  if (n < 6) return false;

  uint8_t b[6];
  for (int i = 0; i < 6; i++) b[i] = Wire.read();

  uint16_t dist =  b[0] | (b[1] << 8);
  uint16_t amp  =  b[2] | (b[3] << 8);
  int16_t  traw =  b[4] | (b[5] << 8);

  distCm   = (float)dist;
  force = amp;
  tempC    = (float)traw / 100.0f;    // registre I2C : unite 0.01 C
                                      // Si la temperature parait fausse, essaie a la place :
                                      // tempC = (float)traw / 8.0f - 256.0f;   // (formule de la trame UART)
  return true;
}

// Lecture fiable ?  (signal suffisant, pas de saturation, dans la portee)
bool LectureValide(float distCm, uint16_t force) {
  if (force < TFLUNA_FORCE_MIN) return false;    // trop faible / surface absorbante
  if (force == 65535)              return false;    // saturation (trop proche/reflechissant)
  if (distCm <= 0 || distCm > TFLUNA_MAX_CM) return false;
  return true;
}


// ============================================================
//  Mouvement DOUX du servo (evite le "clunky").
// ============================================================
void DeplaceServo(int angleCibleDeg, float vitesseDps) {
  angleCibleDeg = constrain(angleCibleDeg, 0, 180);
  if (vitesseDps > SG90_VITESSE_MAX_DPS) vitesseDps = SG90_VITESSE_MAX_DPS;
  if (vitesseDps < 1) vitesseDps = 1;

  int dir = (angleCibleDeg >= courantAngleServoDeg) ? 1 : -1;
  int delaiPasMs = (int)(1000.0f / vitesseDps);         // duree pour 1 deg

  while (courantAngleServoDeg != angleCibleDeg) {
    courantAngleServoDeg += dir;
    lidarServo.write(courantAngleServoDeg);
    delay(delaiPasMs);
  }
}

// Construit un Lidar_Echantillon a partir d'un angle servo + offset (lecture comprise).
Lidar_Echantillon ConstruEchanti(int angleServoDeg, float decalageZ) {
  Lidar_Echantillon s;
  s.angleServoDeg = angleServoDeg;
  s.angleFaisceauDeg  = angleServoDeg - SERVO_CENTRE;
  float angleFaisceauRad = radians((float)s.angleFaisceauDeg);

  float d; uint16_t amp; float t;
  if (TfLunaLireI2C(d, amp, t)) {
    s.distanceCm    = d;
    s.force         = amp;
    s.temperatureC  = t;
    s.x             = d * sin(angleFaisceauRad);
    s.z             = d * cos(angleFaisceauRad) -decalageZ ;
    s.valide        = LectureValide(d, amp);
  } else {
    s.distanceCm    = s.x = s.z = 0.0f;
    s.force         = 0;
    s.temperatureC  = 0.0f;
    s.valide        = false;
  }
  return s;
}

// Pointe vers un angle servo precis et renvoie la lecture (mouvement doux).
Lidar_Echantillon LireAPos(int angleServoDeg, float decalageZ,
                   float vitesseDps, uint16_t fixageMs) {
  DeplaceServo(angleServoDeg, vitesseDps);
  delay(fixageMs);                                     // stabilisation avant lecture
  return ConstruEchanti(angleServoDeg, decalageZ);
}


// ============================================================
//  FONCTION 1 : balayage fluide de l'environnement
//  Balaye 0..angleMax, remplit "sortie" (capacite "capacity"),
//  renvoie le nombre d'echantillons.
//    vitesseDps : vitesse de balayage (bornee au max SG90)
//    decalageZ  : decalage Z du lidar / origine (cm, signe)
//    pasDeg  : resolution angulaire (deg/pas)
//    fixageMs : stabilisation avant chaque lecture
// ============================================================
int BalayageEnvironnement(Lidar_Echantillon *sortie, int capacite,
                     int angleMax, float vitesseDps, float decalageZ,
                     int pasDeg, uint16_t fixageMs) {
  angleMax = constrain(angleMax, 1, 180);
  if (vitesseDps > SG90_VITESSE_MAX_DPS) vitesseDps = SG90_VITESSE_MAX_DPS;
  if (pasDeg < 1) pasDeg = 1;

  int delaiPasMs = (int)((1000.0f * pasDeg) / vitesseDps);
  int count = 0;

  DeplaceServo(0, vitesseDps);                        // depart en douceur a 0 deg
  for (int a = 0; a <= angleMax && count < capacite; a += pasDeg) {
    lidarServo.write(a);
    courantAngleServoDeg = a;
    delay(delaiPasMs + fixageMs);                     // glisse + se stabilise
    sortie[count++] = ConstruEchanti(a, decalageZ);
  }
  return count;
}


// ============================================================
//  FONCTION 2 : pointer vers une distance X donnee
//  2a) GEOMETRIQUE : si tu connais la profondeur Z attendue.
// ============================================================
Lidar_Echantillon PointerEnX(float cibleX, float posZAttendu, float decalageZ,
                             float vitesseDps) {
  float dz = posZAttendu - decalageZ;
  if (dz < 1.0f) dz = 1.0f;
  float angleFaisceauRad  = atan2(cibleX, dz);
  int   angleServoDeg = SERVO_CENTRE + (int)lround(degrees(angleFaisceauRad));
  angleServoDeg = constrain(angleServoDeg, 0, 180);
  return LireAPos(angleServoDeg, decalageZ, vitesseDps);
}

//  2b) PAR RECHERCHE : balaye, trouve l'angle dont le X mesure est
//      le plus proche de targetX, s'y positionne et relit.
Lidar_Echantillon PointerEnXRecherche(float targetX, float decalageZ, float vitesseDps,
                          int angleMax, int pasDeg,
                          Lidar_Echantillon *buf, int capacite) {
  int n = BalayageEnvironnement(buf, capacite, angleMax, vitesseDps, decalageZ, pasDeg);
  int   top = -1;
  float topErr = 1e9;
  for (int i = 0; i < n; i++) {
    if (!buf[i].valide) continue;
    float e = fabs(buf[i].x - targetX);
    if (e < topErr) { topErr = e; top = i; }
  }
  if (top < 0) { Lidar_Echantillon s; s.valide = false; return s; }
  return LireAPos(buf[top].angleServoDeg, decalageZ, vitesseDps);
}


// ============================================================
//  FONCTION 3 : autres fonctions utiles au projet
// ============================================================

// 3a. Obstacle le plus proche dans un scan -> index (-1 si rien)
int TrouverObstacleProche(Lidar_Echantillon *s, int n) {
  int top = -1; float topD = 1e9;
  for (int i = 0; i < n; i++)
    if (s[i].valide && s[i].distanceCm < topD) { topD = s[i].distanceCm; top = i; }
  return top;
}

// 3b. Direction la plus degagee -> angle servo a viser
int TrouverVoieDegage(Lidar_Echantillon *s, int n) {
  int top = -1; float topDegage = -1;
  for (int i = 0; i < n; i++) {
    float degage = s[i].valide ? s[i].distanceCm : (float)TFLUNA_MAX_CM;
    if (degage > topDegage) { topDegage = degage; top = i; }
  }
  return (top < 0) ? SERVO_CENTRE : s[top].angleServoDeg;
}

// 3c. Obstacle dans un secteur frontal +/- demiAngle a moins de seuilCm ?
bool ObstacleEnAvant(Lidar_Echantillon *s, int n, int demiAngle, float seuilCm) {
  for (int i = 0; i < n; i++)
    if (s[i].valide && abs(s[i].angleFaisceauDeg) <= demiAngle && s[i].distanceCm <= seuilCm)
      return true;
  return false;
}

// 3d. Recentrer le lidar droit devant.
void CentrerLidar(float vitesseDps) { DeplaceServo(SERVO_CENTRE, vitesseDps); }

// 3e. Verifie la presence du capteur sur le bus I2C (debug).
bool tfLunaPresent() {
  Wire.beginTransmission(TFL_ADDR);
  return (Wire.endTransmission() == 0);
}

// 3f. Impression lisible d'un echantillon (debug Serial).
void printSample(const Lidar_Echantillon &s) {
  Serial.print(F("servo=")); Serial.print(s.angleServoDeg);
  Serial.print(F(" beam="));  Serial.print(s.angleFaisceauDeg);
  Serial.print(F(" Z="));     Serial.print(s.z, 1);
  Serial.print(F(" X="));     Serial.print(s.x, 1);
  Serial.print(F(" d="));     Serial.print(s.distanceCm, 1);
  Serial.print(F(" str="));   Serial.print(s.force);
  Serial.print(F(" T="));     Serial.print(s.temperatureC, 1);
  Serial.print(F("C "));      Serial.println(s.valide ? F("[ok]") : F("[x]"));
}
