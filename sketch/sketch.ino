#include <Arduino_RouterBridge.h>
#include <Wire.h>

#define MOTOR_ADDR 0x34
#define MOTOR_TYPE_ADDR               20
#define MOTOR_ENCODER_POLARITY_ADDR   21
#define MOTOR_FIXED_SPEED_ADDR        51
#define MOTOR_TYPE_JGB37_520_12V_110RPM  3

volatile float joystick_x = 0.0f;
volatile float joystick_y = 0.0f;

void i2c_write(uint8_t reg, uint8_t *val, unsigned int len) {
    Wire1.beginTransmission(MOTOR_ADDR);
    Wire1.write(reg);
    for (unsigned int i = 0; i < len; i++) Wire1.write(val[i]);
    Wire1.endTransmission();
}

void init_moteurs() {
    uint8_t motorType = MOTOR_TYPE_JGB37_520_12V_110RPM;
    uint8_t polarity = 0;
    i2c_write(MOTOR_TYPE_ADDR, &motorType, 1);
    delay(5);
    i2c_write(MOTOR_ENCODER_POLARITY_ADDR, &polarity, 1);
    delay(100);
}

void set_vitesse_moteurs(int gauche, int droite) {
    gauche = constrain(gauche, -100, 100);
    droite = constrain(droite, -100, 100);
    int8_t cmd[4] = {(int8_t)gauche, (int8_t)droite, 0, 0};
    i2c_write(MOTOR_FIXED_SPEED_ADDR, (uint8_t*)cmd, 4);
}

void set_joystick_x(float x) { joystick_x = x; }

void set_joystick_y(float y) {
    joystick_y = y;
    int gauche = (int)((joystick_y + joystick_x) * 30.0f);
    int droite = (int)((joystick_y - joystick_x) * 30.0f);
    set_vitesse_moteurs(gauche, droite);
}

const char* scan_i2c() {
    static String resultat;
    resultat = "";
    bool trouve = false;
    for (byte addr = 1; addr < 127; addr++) {
        Wire1.beginTransmission(addr);
        if (Wire1.endTransmission() == 0) {
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
    Wire1.begin();
    delay(500);
    init_moteurs();
    delay(500);
    Bridge.begin();
    Bridge.provide_safe("joy_x", set_joystick_x);
    Bridge.provide_safe("joy_y", set_joystick_y);
    Bridge.provide_safe("scan_i2c", scan_i2c);
}

void loop() {
    Bridge.update();
}
