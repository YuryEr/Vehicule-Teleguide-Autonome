#ifndef CONFIG_H
#define CONFIG_H

// Adresses I2C (bus Wire1 / Qwiic)
#define ADRESSE_MOTEUR        0x34
#define ADRESSE_GYRO          0x68
#define ADRESSE_LIDAR         0x10

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
#define ROT_TEMPS_MIN_MS      400     // duree minimale d'une rotation (anti-saut)

// Timeouts securite (ms)
#define TIMEOUT_AVANCE_MS     12000
#define TIMEOUT_ROTATION_MS   8000

// Bandeaux LED adressables (WS2812B, protocole 800 kHz)
// Chaque bandeau porte 7 LEDs en serie sur une seule ligne de donnees :
// les pixels 0 a 4 forment la barre haute, les pixels 5 et 6 occupent la
// position des feux avant et arriere.
#define PIN_BANDEAU_AVANT      6
#define PIN_BANDEAU_ARRIERE    7
#define NB_LEDS_PAR_BANDEAU    7

#define ZONE_BARRE_DEBUT       0
#define ZONE_BARRE_FIN         4
#define ZONE_PHARES_DEBUT      5
#define ZONE_PHARES_FIN        6

// Cote du vehicule occupe par chaque pixel de la zone des feux.
#define PIXEL_COTE_DROIT       5
#define PIXEL_COTE_GAUCHE      6

// Gain global du bandeau. Les deux zones partagent la meme ligne de
// donnees : le contraste entre la barre et les feux se fait par les
// valeurs RVB, pas par des luminosites separees.
#define LUMINOSITE_BANDEAU     255
#define PERIODE_GYROPHARE_MS   150
#define PERIODE_CLIGNOTANT_MS  400

// Signalisation de virage. Vocabulaire partage entre le module de
// deplacement, qui produit la direction, et celui des LEDs, qui l'affiche.
#define VIRAGE_AUCUN           0
#define VIRAGE_GAUCHE          1
#define VIRAGE_DROITE          2

// Ecran TFT ILI9341 (SPI materiel)
#define PIN_TFT_CS   10
#define PIN_TFT_DC   9
#define PIN_TFT_RST  8

// Codes QR affiches a l'ecran
#define QR_VERSION           3     // 29x29 modules, 53 octets en correction basse
#define QR_TAILLE_MODULE     4     // cote d'un module, en pixels
#define QR_ZONE_SILENCE      4     // marge blanche en modules, exigee par la norme

// Capteur ultrason HC-SR04 (ECHO relie en direct, sans pont diviseur)
#define PIN_ULTRASON_TRIG      2
#define PIN_ULTRASON_ECHO      3
#define ULTRASON_DISTANCE_MAX  100    // portee utile (cm)
#define ULTRASON_PERIODE_MS    100    // rafraichissement (~10 Hz)
// Bornes de mesure explicites : pulseIn n'honore pas son timeout sur le
// core Zephyr et fige la boucle si ECHO reste a l'etat haut. Le timeout
// borne aussi l'occupation de la boucle : au dela de la portee utile,
// l'attente est abandonnee et le balayage servo reprend la main.
#define ULTRASON_TIMEOUT_US           6000UL    // 100 cm aller-retour : ~5.8 ms
#define ULTRASON_TIMEOUT_PRESENCE_US  60000UL   // > 38 ms : echo emis sans obstacle

// LiDAR TF-Luna (I2C sur Wire1 / Qwiic, broche CFG a la masse)
#define REG_LIDAR_DIST        0x00    // 6 registres : dist L/H, force L/H, temp L/H
#define LIDAR_FORCE_MIN       100     // en dessous : signal trop faible -> rejet
#define LIDAR_DISTANCE_MIN    20      // zone morte : en deca, la datasheet
                                      // declare la mesure non fiable
#define LIDAR_DISTANCE_MAX    800     // portee fiable (cm)
#define LIDAR_PERIODE_MS      100     // rafraichissement (~10 Hz)

// Servo de balayage SG90 (support du LiDAR)
#define PIN_SERVO             5       // D5
#define SERVO_ANGLE_MIN       0
#define SERVO_ANGLE_MAX       180
#define SERVO_ANGLE_CENTRE    90      // 90 deg = droit devant
#define SERVO_MS_PAR_DEGRE    5       // 5 ms/deg ~= 200 deg/s (max SG90 ~300)
#define SERVO_PULSE_MIN_US    600     // largeur d'impulsion a 0 deg
#define SERVO_PULSE_MAX_US    2400    // largeur d'impulsion a 180 deg
#define SERVO_MAINTIEN_MS     400     // maintien actif apres l'arrivee a la cible

// Detection d'obstacles (fusion ultrason + LiDAR)
#define OBSTACLE_SEUIL_CM           40    // en deca : obstacle signale
#define OBSTACLE_PERIODE_MS         50    // reevaluation de la distance frontale
#define OBSTACLE_ECART_SONDAGE_DEG  45    // ecart des secteurs lateraux (deg)
#define OBSTACLE_SENS_SERVO         1     // -1 si gauche et droite sont inverses
#define OBSTACLE_STABILISATION_MS   40    // attente apres arrivee, avant la mesure

// Feux de signalisation (donnees fournies par la vision du MPU)
#define FEU_AGE_MAX_MS              3000  // au dela, la detection est perimee

// Evitement d'obstacle (contournement lateral)
#define EVITEMENT_DISTANCE_M        0.30  // longement le long de l'obstacle (m)
#define EVITEMENT_ESSAIS_MAX        3     // rotations avant d'abandonner

// Recul de chaque capteur par rapport au pare-choc avant (cm). Les mesures
// sont ramenees a cette reference commune : sans cela, la fusion comparerait
// deux distances prises depuis deux origines differentes. A mesurer sur la
// coque finale, les deux capteurs n'etant pas superposes.
#define ULTRASON_RECUL_CM           0
#define LIDAR_RECUL_CM              0

#endif