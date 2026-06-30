#include <Arduino_RouterBridge.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// --- LEDs ---
#define NUM_LEDS  7
#define PIN_LED1  3
#define PIN_LED2  6

Adafruit_NeoPixel bandeau1(NUM_LEDS, PIN_LED1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel bandeau2(NUM_LEDS, PIN_LED2, NEO_GRB + NEO_KHZ800);

volatile int mode1 = 0, mode2 = 0;
unsigned long timer1 = 0, timer2 = 0;
bool etat1 = false, etat2 = false;
bool phares1_allumes = false, phares2_allumes = false;
bool eteint1_fait = false, eteint2_fait = false;

// --- Driver moteur Yahboom ---
#define MOTOR_ADDR 0x34

// Registres (tirés de la doc officielle Yahboom)
#define MOTOR_TYPE_REG      20
#define MOTOR_PHASE_REG     21
#define MOTOR_PULSELINE_REG 22
#define MOTOR_WHEELDIS_REG  23
#define MOTOR_DEADZONE_REG  24
#define MOTOR_SPEED_REG     51

// Joystick
volatile float joystick_x = 0.0f;
volatile float joystick_y = 0.0f;

// --- Fonctions I2C ---
void i2c_write_uint8(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MOTOR_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
    delay(100);
}

void i2c_write_uint16(uint8_t reg, uint16_t val) {
    Wire.beginTransmission(MOTOR_ADDR);
    Wire.write(reg);
    Wire.write((uint8_t)(val >> 8));
    Wire.write((uint8_t)(val & 0xFF));
    Wire.endTransmission();
    delay(100);
}

void i2c_write_float(uint8_t reg, float val) {
    uint8_t* p = (uint8_t*)&val;
    Wire.beginTransmission(MOTOR_ADDR);
    Wire.write(reg);
    Wire.write(p[3]);
    Wire.write(p[2]);
    Wire.write(p[1]);
    Wire.write(p[0]);
    Wire.endTransmission();
    delay(100);
}

void init_moteurs() {
    // Configuration pour moteur JGB3865-520R45-12 (type 1 = 520 motor)
    i2c_write_uint8(MOTOR_TYPE_REG, 1);       // Type 520
    i2c_write_uint8(MOTOR_PHASE_REG, 45);     // Ratio réduction 45:1
    i2c_write_uint8(MOTOR_PULSELINE_REG, 11); // Lignes magnétiques
    i2c_write_float(MOTOR_WHEELDIS_REG, 67.0f); // Diamètre roue ~67mm
    i2c_write_uint16(MOTOR_DEADZONE_REG, 1900); // Zone morte
}

void set_vitesse_moteurs(int gauche, int droite) {
    gauche = constrain(gauche, -100, 100);
    droite = constrain(droite, -100, 100);
    Wire.beginTransmission(MOTOR_ADDR);
    Wire.write(MOTOR_SPEED_REG);
    Wire.write((uint8_t)(int8_t)gauche);
    Wire.write((uint8_t)(int8_t)droite);
    Wire.write(0);
    Wire.write(0);
    Wire.endTransmission();
}

// --- Callbacks Bridge joystick ---
void set_joystick_x(float x) { joystick_x = x; }

void set_joystick_y(float y) {
    joystick_y = y;
    int gauche = (int)((joystick_y + joystick_x) * 100.0f);
    int droite = (int)((joystick_y - joystick_x) * 100.0f);
    set_vitesse_moteurs(gauche, droite);
}

// --- Callbacks Bridge LEDs ---
void set_mode_led1(int mode) {
    mode1 = mode; etat1 = false;
    phares1_allumes = false; eteint1_fait = false;
}

void set_mode_led2(int mode) {
    mode2 = mode; etat2 = false;
    phares2_allumes = false; eteint2_fait = false;
}

// --- LEDs ---
void remplir(Adafruit_NeoPixel& b, uint8_t r, uint8_t g, uint8_t bl) {
    for (int i = 0; i < NUM_LEDS; i++) b.setPixelColor(i, b.Color(r, g, bl));
    b.show();
}

void eteindre(Adafruit_NeoPixel& b) { remplir(b, 0, 0, 0); }

void animerBandeau(Adafruit_NeoPixel& b, int mode,
                   unsigned long& timer, bool& etat,
                   bool& pharesAllumes, bool& eteintFait) {
    unsigned long maintenant = millis();
    switch (mode) {
        case 0:
            if (!eteintFait) { eteindre(b); eteintFait = true; }
            break;
        case 1:
            if (maintenant - timer >= 150) {
                timer = maintenant; etat = !etat;
                if (etat) remplir(b, 0, 0, 200);
                else      remplir(b, 200, 0, 0);
            }
            break;
        case 2:
            if (maintenant - timer >= 400) {
                timer = maintenant; etat = !etat;
                if (etat) remplir(b, 200, 80, 0);
                else      eteindre(b);
            }
            break;
        case 3:
            if (!pharesAllumes) { remplir(b, 200, 200, 200); pharesAllumes = true; }
            break;
    }
}

// --- Scan I2C ---
const char* scan_i2c() {
    static String resultat;
    resultat = "";
    bool trouve = false;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            resultat += "0x";
            resultat += String(addr, HEX);
            resultat += " ";
            trouve = true;
        }
    }
    if (!trouve) resultat = "aucun";
    return resultat.c_str();
}

void setup() {
    bandeau1.begin(); bandeau2.begin();
    eteindre(bandeau1); eteindre(bandeau2);
    eteint1_fait = true; eteint2_fait = true;

    Wire.begin();
    delay(500);
    init_moteurs();
    delay(500);

    Bridge.begin();
    Bridge.provide_safe("mode_led1", set_mode_led1);
    Bridge.provide_safe("mode_led2", set_mode_led2);
    Bridge.provide_safe("joy_x",     set_joystick_x);
    Bridge.provide_safe("joy_y",     set_joystick_y);
    Bridge.provide_safe("scan_i2c",  scan_i2c);
}

void loop() {
    Bridge.update();
    animerBandeau(bandeau1, mode1, timer1, etat1, phares1_allumes, eteint1_fait);
    animerBandeau(bandeau2, mode2, timer2, etat2, phares2_allumes, eteint2_fait);
}