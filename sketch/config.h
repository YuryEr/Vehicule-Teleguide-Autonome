#ifndef CONFIG_H
#define CONFIG_H

// Adresses I2C (bus Wire1 / Qwiic)
#define ADRESSE_MOTEUR        0x34
#define ADRESSE_GYRO          0x68
#define ADRESSE_LIDAR         0x10

// Registres carte moteur Hiwonder. Adresses et unites reprises de l'exemple
// officiel du fabricant, fourni avec les moteurs a encodeur.
#define REG_TYPE_MOTEUR       20      // 0x14 : type de moteur a encodeur
#define REG_POLARITE_ENCODEUR 21      // 0x15 : polarite du sens de comptage
#define REG_PWM_FIXE          31      // 0x1F : PWM fixe, boucle ouverte (-100 a 100)
#define REG_VITESSE_FIXE      51      // 0x33 : vitesse fixe, boucle fermee
#define REG_ENCODEUR_TOTAL    60      // 0x3C : impulsions cumulees des 4 canaux

// JGB37-520 12V 110RPM : 44 impulsions par tour d'aimant, reduction 90.
#define TYPE_JGB37_520        3

// Registres MPU-6050
#define REG_PWR_MGMT_1        0x6B
#define REG_SMPRT_DIV         0x19    // diviseur de cadence d'echantillonnage
#define REG_CONFIG            0x1A    // filtre passe-bas numerique (DLPF)
#define REG_GYRO_CONFIG       0x1B    // pleine echelle du gyroscope
#define REG_GYRO_ZOUT_H       0x47    // axe Z seul, 2 octets
#define SENSIBILITE_GYRO      131.0   // LSB par deg/s a pleine echelle +/-250

// Le capteur demarre avec 256 Hz de bande passante, tres au-dela de ce que le
// chassis produit : on n'integre alors que la vibration des chenilles. La
// valeur 4 ramene la bande a 20 Hz, sous la moitie de la cadence de lecture.
#define DLPF_20HZ             4
#define SMPRT_DIV_200HZ       4       // 1000 / (1 + 4) = 200 Hz interne

// Calibration du zero. La version longue sert au demarrage, la courte est
// rejouee avant chaque mouvement asservi, le vehicule etant alors immobile.
#define IMU_CAL_ECHANTILLONS        200
#define IMU_CAL_ECHANTILLONS_RAPIDE 40
#define IMU_CAL_DISPERSION_MAX      3.0f  // deg/s : au-dela, le vehicule bougeait

// Calibration mecanique. DIAMETRE_ROUE_MM et RATIO_REDUCTEUR n'interviennent
// que par leur produit dans Moteurs_PulsesEnMetres, et le glissement de la
// chenille s'y cache : ce sont des constantes ajustees par la mesure, pas des
// grandeurs physiques. Le rapport annonce par le fabricant pour le JGB37-520
// est de 90, mais il donne le double de la distance reelle sur ce chassis.
// Reglage : RATIO_nouveau = RATIO_actuel * (distance_commandee / mesuree),
// mesure sur 2 m pour limiter le poids des transitoires.
#define DIAMETRE_ROUE_MM      65.0
#define RATIO_REDUCTEUR       50.0  
#define IMPULSIONS_PAR_TOUR   44.0
#define IMPULSIONS_PAR_ROUE   (IMPULSIONS_PAR_TOUR * RATIO_REDUCTEUR)

// Vitesses moteur (consignes). En boucle fermee, l'unite est le nombre
// d'impulsions par 10 ms : la plage utile va jusqu'a environ 50 selon la
// charge et la tension. Les valeurs retenues sont celles de l'exemple du
// fabricant, 23 en translation et 20 en rotation sur place.
#define VITESSE_DEPLACEMENT   23
#define VITESSE_ROTATION      11
#define VITESSE_JOYSTICK      80

// Compensation rotation
// A REPRENDRE : la marge n'a pas ete recalibree depuis le passage de
// VITESSE_ROTATION de 12 a 20, et les rotations depassent d'environ 15 degres.
// Methode : commander 90 puis 180 degres. Un ecart constant designe l'inertie,
// donc cette marge ; un ecart proportionnel designe SENSIBILITE_GYRO. Garder
// ROT_MARGE_LENTE_DEG nettement au-dessus, sans quoi la phase d'approche lente
// n'a plus lieu avant l'arret.
#define ROT_MARGE_ARRET_DEG   10.0
#define ROT_MARGE_LENTE_DEG   20.0

// Repetition de la consigne moteur pendant un mouvement asservi (ms)
#define REEMISSION_MOTEUR_MS  100

// Compensation de derive en ligne droite. Les deux moteurs tiennent la meme
// vitesse, verifie aux encodeurs a 1 % pres ; ce sont les chenilles qui ne
// parcourent pas la meme distance. Le decalage ajoute des impulsions du cote
// qui patine.
// La consigne moteur etant entiere, une unite complete represente plus de 4 %
// de differentiel, souvent trop. Elle est donc repartie sur les re-emissions
// successives : AVANCE_TRIM_NUM sur AVANCE_TRIM_DEN d'entre elles la portent,
// soit un decalage effectif de NUM/DEN d'unite.
// Signe positif pour accelerer la chenille gauche, negatif pour la droite.
// Valeurs relevees sur 1 m : la derive residuelle y est nulle. Sur 2 m le
// vehicule decrit une legere courbe en S, signe qu'un transitoire de demarrage
// s'ajoute a la derive etablie. Une constante ne peut annuler leur somme qu'a
// une seule distance : regler a la distance reellement utilisee, ou composer
// les longs trajets avec plusieurs blocs de la distance calibree.
#define AVANCE_TRIM_NUM       8     // de -AVANCE_TRIM_DEN a +AVANCE_TRIM_DEN
#define AVANCE_TRIM_DEN       16


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
#define OBSTACLE_SENS_SERVO         -1     // -1 si gauche et droite sont inverses
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