# 😀 TankEts_ELE795

Vehicule teleguide autonome. PFE ELE795, Ecole de technologie superieure, ete 2026.

## Architecture materielle

- **Arduino UNO Q** : double processeur (Qualcomm QRB2210 MPU + STM32U585 MCU)
- **Hiwonder Tank** : chassis chenille, carte moteur I2C (0x34)
- **MPU-6050** : IMU gyroscope/accelerometre I2C (0x68)
- **Webcam USB** : detection routiere (YOLO, OpenCV)
- **Bandeaux LED WS2812B** : 7 LEDs par bandeau : barre haute (gyrophare, feux de
  position), feux avant/arriere et clignotants de virage
- **Ecran TFT ILI9341** : 320x240, SPI materiel
- **HC-SR04** : capteur ultrason, detection de presence frontale (cone large)
- **TF-Luna** : LiDAR I2C (0x10), telemetre frontal (FOV 2 deg)
- **Servo SG90** : support orientable du LiDAR, sondage par secteurs

## Architecture logicielle

    sketch/              MCU (STM32) : temps reel, capteurs, actionneurs
      sketch.ino           setup/loop, enregistrement des RPC Bridge
      config.h             Constantes materielles centralisees (broches, adresses, seuils)
      bus_i2c.h/cpp        Scan I2C unique au demarrage + cache de presence
      comm_bridge.h/cpp    Bridge RPC, reception des donnees de vision
      moteurs.h/cpp        Carte Hiwonder, encodeurs, I2C bas niveau
      imu.h/cpp            MPU-6050, calibration gyroscope
      deplacement.h/cpp    Machine a etats, joystick, asservissement encodeurs + gyro
      leds.h/cpp           Bandeaux WS2812B : barre, feux, clignotants
      ecran.h/cpp          Ecran ILI9341 : codes QR de connexion, infos reseau
      ultrason.h/cpp       HC-SR04, mesure d'echo bornee sur micros()
      lidar.h/cpp          TF-Luna, distance par I2C
      servo_lidar.h/cpp    SG90, PWM logiciel (support du LiDAR)
      obstacle.h/cpp       Fusion ultrason + LiDAR, sondage par secteurs
      securite.h/cpp       Veto sur les commandes moteur (mode autonome)
      evitement.h/cpp      Contournement lateral : sondage, rotation, longement
      test_capteurs.h/cpp  Releve serie periodique (bascule TEST_CAPTEURS_ACTIF)

    python/              MPU (Qualcomm Linux) : serveur web, vision, Bridge
      main.py              Point d'entree
      serveur_web.py       Flask + SocketIO + streaming video
      comm_bridge.py       Communication Bridge RPC (MPU <-> MCU)
      vision.py            Detection YOLO + lignes (seuillage adaptatif + centroide)
      boucle_vision.py     Pipeline de vision a cadences
      navigation.py        Suivi de ligne autonome (proportionnel)
      requirements.txt     Dependances Python

    assets/              Interface web
      index.html           Page principale
      style.css            Styles
      app.js               Logique JS (joystick, Blockly, SocketIO)
      libs/                Blockly + Socket.IO embarques

## Brochage

Toutes les broches numeriques sont declarees dans `sketch/config.h` : c'est la
seule source de verite du code. Les tableaux ci-dessous doivent lui correspondre.

### Vue d'ensemble des broches occupees

| Broche UNO Q | Utilisation |
|---|---|
| Qwiic (Wire1) | Carte moteur 0x34, IMU 0x68, LiDAR 0x10 |
| D2 | Ultrason : TRIG |
| D3 | Ultrason : ECHO |
| D5 | Servo SG90 : signal |
| D6 | Bandeau LED avant : data |
| D7 | Bandeau LED arriere : data |
| D8 | Ecran : RESET |
| D9 | Ecran : DC |
| D10 | Ecran : CS |
| D11 | Ecran : MOSI (SPI materiel) |
| D12 | Ecran : MISO (SPI materiel, non utilise en ecriture) |
| D13 | Ecran : SCK (SPI materiel) |

### Peripheriques I2C sur le bus Qwiic (`Wire1`)

Les trois peripheriques partagent le meme bus. Aucune adresse n'entre en conflit.

| Peripherique | Adresse | Alimentation | Particularite |
|---|---|---|---|
| Carte moteur Hiwonder | 0x34 | Alimentation propre du chassis | Aucune |
| IMU MPU-6050 | 0x68 | 3.3V du Qwiic | Aucune |
| LiDAR TF-Luna | 0x10 | **5V** (pas le 3.3V du Qwiic) | Broche **CFG a la masse** + power cycle |

**TF-Luna, alimenter en 5V.** Le capteur demande 3.7-5.2V ; le 3.3V du Qwiic est
sous son minimum. On utilise le Qwiic pour SDA/SCL/GND, mais VCC vient du 5V.

**TF-Luna, CFG a la masse, puis power cycle.** CFG (broche 5) reliee a GND
selectionne le mode I2C. Le mode est **lu au demarrage du capteur** : apres avoir
branche CFG, il faut **couper et remettre l'alimentation**, sinon le capteur reste
en UART et n'apparait jamais sur le bus.

**Sans aucun peripherique sur le Qwiic**, les lignes flottent faute de resistances
de tirage et toutes les transactions I2C partent en timeout.

### Ultrason HC-SR04

| Broche du capteur | Broche UNO Q | Role |
|---|---|---|
| VCC | 5V | Alimentation |
| TRIG | D2 | Declenchement de la salve (impulsion 10 us) |
| ECHO | D3 | Duree de l'echo, en direct (sans pont diviseur) |
| GND | GND | Masse |

**ECHO se branche en direct.** Le montage a d'abord utilise un pont diviseur
22k/47k pour ramener la sortie 5V du capteur a ~3.4V. Il s'est avere inutile : la
liaison directe fonctionne correctement sur notre carte. A noter pour la
reproductibilite : la tolerance 5V des broches D de l'UNO Q n'est **pas garantie**
par Arduino (seuls certains pads STM32U585 sont 5V-tolerants, et jamais en mode
analogique). Le montage fonctionne, mais il sort de la plage documentee.

**TRIG est pilote en 3.3V** alors que le seuil du HC-SR04 se situe vers 3.5V.
Cela fonctionne sur nos exemplaires ; un capteur qui ne declencherait jamais est
la premiere piste a examiner.

### Servo de balayage SG90

| Fil du servo | Broche UNO Q | Role |
|---|---|---|
| Orange / jaune | D5 | Signal PWM (genere en logiciel) |
| Rouge | 5V | Alimentation |
| Brun / noir | GND | Masse, commune avec la carte |

La librairie `Servo` est inutilisable sur ce core (voir plus bas) : l'impulsion est
generee par `servo_lidar.cpp`.

### Bandeaux LED WS2812B

| Broche du bandeau | Broche UNO Q | Role |
|---|---|---|
| DI (bandeau avant) | D6 | Donnees. Sur WS2813, relier BI a la meme broche |
| DI (bandeau arriere) | D7 | Donnees |
| VCC | 5V | Alimentation |
| GND | GND | Masse |

Chaque bandeau porte **7 LEDs en serie sur une seule ligne de donnees**, decoupees
en deux zones par le logiciel :

| Pixels | Zone | Role |
|---|---|---|
| 0 a 4 | Barre haute | Feux de position ou gyrophare |
| 5 | Feu droit | Blanc a l'avant, rouge a l'arriere, orange en clignotant |
| 6 | Feu gauche | Idem |

La logique de l'UNO Q est en 3.3V : le seuil des WS2812B est limite mais fonctionne
en pratique, aucun level-shifter n'a ete necessaire.

### Ecran TFT ILI9341 sur SPI materiel

| Broche de l'ecran | Broche UNO Q | Role |
|---|---|---|
| CS | D10 | Selection du peripherique |
| RESET | D8 | Reinitialisation |
| DC (ou RS) | D9 | Choix donnee / commande |
| SDI (MOSI) | D11 | Donnees vers l'ecran |
| SCK | D13 | Horloge SPI |
| SDO (MISO) | D12 | Donnees depuis l'ecran, non utilise en ecriture |
| VCC | *a confirmer sur le module* | Alimentation |
| LED | *a confirmer sur le module* | Retroeclairage |
| GND | GND | Masse |

Les six lignes de signal sont celles declarees dans `config.h` et le materiel SPI de
la carte. L'alimentation et le retroeclairage dependent du modele de carte de
decouplage et doivent etre releves sur l'exemplaire utilise. Aucun level-shifter
n'est necessaire : l'ILI9341 accepte la logique 3.3V de l'UNO Q.

## Librairies Arduino : versions critiques

Le core Zephyr de l'UNO Q ne supporte pas les librairies qui accedent directement
aux registres (style AVR). **Les versions ci-dessous sont obligatoires** : des versions
plus anciennes compilent parfois sans erreur mais ne fonctionnent pas au runtime.

```yaml
# sketch/sketch.yaml
profiles:
  default:
    fqbn: arduino:zephyr:unoq
    platforms:
      - platform: arduino:zephyr
    libraries:
      - Adafruit NeoPixel (1.15.5)
      - Adafruit GFX Library (1.12.6)
      - Adafruit ILI9341 (1.6.3)
      - Adafruit BusIO (1.17.4)
      - QRCode (0.0.1)
default_profile: default
```

| Librairie | Version | Pourquoi cette version |
|---|---|---|
| Adafruit NeoPixel | **1.15.5** | Les versions anterieures (ex. 1.12.0) **compilent** mais leur timing bit-bang 800 kHz ne produit **aucun signal** sur STM32U585/Zephyr. Symptome : LEDs completement eteintes. |
| Adafruit BusIO | **1.17.4** | Contient le guard `#elif defined(__MBED__) \|\| defined(__ZEPHYR__)` qui desactive `BUSIO_USE_FAST_PINIO`. Les versions < 1.15 **ne compilent pas** (erreur `portOutputRegister` / `digitalPinToPort`). |
| Adafruit GFX / ILI9341 | 1.12.6 / 1.6.3 | Dependances de l'ecran, dernieres versions stables. |
| QRCode | 0.0.1 | Generation des codes QR de connexion. C pur, sans acces registre ni allocation dynamique, le tampon est fourni par l'appelant, ce qui la rend compatible avec le core Zephyr. |

Les autres librairies visibles a la compilation (`Arduino_RouterBridge`, `Arduino_RPClite`,
`MsgPack`, `DebugLog`, `ArxTypeTraits`, `ArxContainer`, `Wire`) sont des dependances
**automatiques** du systeme Bridge et du core. Ne pas les epingler dans `sketch.yaml` :
elles forment un ensemble coherent livre avec le Bridge.

### Librairie a ne pas utiliser

**`Servo` est inutilisable sur ce core.** Des qu'elle coexiste avec NeoPixel, l'edition
de liens echoue sur `undefined reference to noInterrupts()/interrupts()`. Le servo de
balayage est donc pilote par un PWM logiciel dans `sketch/servo_lidar.cpp` : l'impulsion
est mesuree par attente active sur `micros()`, `delayMicroseconds()` etant imprecis sous
le scheduler Zephyr.

## Guide de deploiement sur Arduino UNO Q

### Prerequis

- Arduino App Lab installe (v0.8+)
- Arduino UNO Q connecte par USB au PC
- Webcam USB branchee

### Etape 1 : Configuration de l'Arduino UNO Q en mode point d'acces

Creer un point d'acces (AP), remplacer le SSID et le mot de passe :

```bash
nmcli device wifi hotspot ssid TankETS password tank1234 ifname wlan0
```

Appliquer automatiquement a chaque demarrage :

```bash
nmcli connection modify Hotspot connection.autoconnect yes
```

En mode point d'acces la carte n'a **pas d'acces Internet** : `main.py` ignore alors
l'installation pip (enveloppee dans un `try/except`) et demarre le serveur avec les
dependances deja presentes. Faire au moins un premier demarrage **connecte a Internet**
pour que les dependances Python s'installent.

### Etape 2 : Configuration des ports

Dans `app.yaml` a la racine du projet, s'assurer que le port du serveur web est expose :

```yaml
ports: [7000]
```

Sans cette ligne, le conteneur Docker tourne mais le port n'est pas accessible de l'exterieur.

### Etape 3 : Deployer le code

1. Ouvrir le projet dans Arduino App Lab
2. Cliquer **Run** (bouton vert)
3. App Lab va :
   - Compiler et flasher le sketch sur le MCU (STM32)
   - Creer un conteneur Docker sur le MPU (Qualcomm)
   - Installer automatiquement les dependances Python (premiere fois ~30s)
   - Demarrer le serveur web

### Etape 4 : Acceder a l'interface

1. Trouver l'IP de la carte (affichee en bas d'App Lab, ex: `192.168.137.52`)
2. Ouvrir dans un navigateur : `http://<IP>:7000`

### Depannage

| Probleme | Solution |
|----------|----------|
| Page web ne charge pas | Verifier `ports: [7000]` dans `app.yaml` |
| Camera non disponible | La camera USB n'est pas sur l'index attendu, le code scanne automatiquement `/dev/video0` a `/dev/video9` |
| Terminal Python vide | Les dependances pip s'installent au premier demarrage, patienter ~30s |
| Erreur `ModuleNotFoundError` | Verifier que `requirements.txt` est a jour et relancer Run |
| Le serveur ne demarre pas hors ligne | `main.py` enveloppe l'install pip dans un `try/except` : en mode point d'acces (sans Internet) l'install est ignoree et le serveur demarre quand meme |
| `Error: verify failed in bank at 0x08000000` | Le flash a echoue : le MCU tourne **l'ancien code**. Relancer Run (parfois 2-3 fois) jusqu'a un flash propre. Si une modification semble sans effet, c'est souvent ca. |
| `ld: file truncated` / `invalid string offset` / segfault du linker | Cache de compilation corrompu. Supprimer `.cache/sketch` sur la carte (garder `.cache/.venv` qui contient l'environnement Python) : `rm -rf ~/ArduinoApps/tankets_ele795/.cache/sketch` |
| LEDs completement eteintes | Verifier `Adafruit NeoPixel (1.15.5)` dans `sketch.yaml`, les versions anterieures compilent mais ne pilotent rien |
| Ecran : erreur `portOutputRegister` a la compilation | Verifier `Adafruit BusIO (1.17.4)`, les versions < 1.15 ne compilent pas sur Zephyr |
| LiDAR `capteur present : NON` | CFG (broche 5) a la masse ? Alimentation >= 3.7V ? **Power cycle apres avoir branche CFG** ? SDA/SCL non inverses ? |
| Un capteur absent rend la carte muette | Le Bridge est enregistre **avant** les inits materielles dans `setup()`, donc le controle survit a un capteur defaillant. Si le probleme persiste, verifier qu'aucune init bloquante n'a ete deplacee avant les `provide_safe`. |
| `[Bridge.read_loop] 'utf-8' codec can't decode byte ...` puis tous les RPC qui expirent apres 10 s | La sortie serie du MCU partage son lien avec les RPC du Bridge. Sous une ecriture serie soutenue, le multiplexage perd une frontiere de trame et la boucle de lecture du Bridge meurt : plus aucun appel n'aboutit jusqu'au redemarrage. **Commenter `TEST_CAPTEURS_ACTIF` dans `test_capteurs.h`**, et n'ecrire sur le port serie que ponctuellement. Symptome typique : une sequence de blocs qui casse apres une dizaine de commandes. |

### Diagnostic I2C

Pour verifier ce qui repond sur le bus Qwiic, ajouter temporairement dans `setup()`
apres `Wire1.begin()` :

```cpp
for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    if (Wire1.endTransmission() == 0) {
        Serial.print("[i2c] Trouve a 0x");
        Serial.println(addr, HEX);
    }
}
```

Attendu : `0x34` (moteurs), `0x68` (IMU), `0x10` (LiDAR).

## Conventions de code

- **Python** : snake_case (fonctions, variables, fichiers)
- **JavaScript** : camelCase (fonctions, variables), UPPER_CASE (constantes)
- **HTML** : kebab-case (IDs, classes)
- **C/Arduino** : PascalCase + prefixe module (ex: `CommBridge_Initialiser`)
- **Fichiers .h** : documentation en francais, format bloc majuscule
- **Constantes materielles** : centralisees dans `sketch/config.h`, jamais dans les modules
- **Modules capteurs** : pattern `X_Initialiser()` / `X_MettreAJour()` / `X_Valeur()`,
  avec mise en cache non bloquante rafraichie dans `loop()`

## Contrat RPC Bridge (Python vers MCU)

| Fonction | Parametres | Description |
|----------|-----------|-------------|
| `joy_x`, `joy_y` | float | Pilotage manuel joystick |
| `roues` | int, int (0-100, positifs) | Vitesse par cote, suivi de ligne |
| `avancer_metres` | float (positif) | Deplacement avant asservi |
| `reculer_metres` | float (positif) | Deplacement arriere asservi |
| `tourner_gauche_deg` | float (positif) | Rotation gauche asservie |
| `tourner_droite_deg` | float (positif) | Rotation droite asservie |
| `arreter_mouvement` | aucun | Arret d'urgence |
| `mouvement_actif` | aucun, retourne int (0/1) | Polling fin de mouvement |
| `definir_mode` | int | 0=manuel (aucun veto), 1=autonome (veto sur obstacle) |
| `veto_actif` | aucun, retourne int (0/1) | Le MCU refuse-t-il d'avancer ? |
| `cause_arret` | aucun, retourne int | 0=voie libre, 1=obstacle, 2=feu rouge ou jaune |
| `mode_bandeaux` | int | Barre haute : 0=eteint, 1=position, 2=gyrophare |
| `mode_phares` | int (0/1) | Feux : blanc a l'avant, rouge a l'arriere |
| `lire_ultrason_cm` | aucun, retourne int | Distance frontale HC-SR04 (cm), plafond `ULTRASON_DISTANCE_MAX` = voie degagee |
| `lire_lidar_cm` | aucun, retourne int | Distance frontale TF-Luna (cm), -1 si invalide |
| `servo_angle` | int (0-180) | Oriente le servo de balayage, 90 = droit devant |
| `obstacle_frontal_cm` | aucun, retourne int | Distance fusionnee ultrason + LiDAR (cm) |
| `obstacle_detecte` | aucun, retourne int (0/1) | Obstacle sous `OBSTACLE_SEUIL_CM` |
| `lancer_sondage` | aucun | Sonde les trois secteurs (~1 s, vehicule a l'arret) |
| `cote_degage` | aucun, retourne int | 0 = sondage en cours, 1 = gauche, 2 = droite |

RPC dans l'autre sens (MCU recoit du MPU) : `on_feu(bool, int, int)`. L'ecart de
ligne n'est pas transmis au MCU : le suivi de ligne est calcule sur le MPU, qui
n'envoie que la consigne de vitesse par `roues`.

**Note** : le Bridge ne transmet pas correctement les nombres negatifs. Toujours envoyer
des valeurs positives, le sens est gere par des fonctions dediees cote MCU (c'est
pourquoi `roues` prend deux vitesses positives plutot qu'un differentiel signe).

## Detection des feux de signalisation

L'entree du modele YOLOv8n exporte en ONNX est **figee a 256 pixels de cote**.
Redimensionner l'image entiere pour l'y faire tenir divise la largeur apparente
des objets par plus de deux : un feu de 6 cm vu a 1 m passe d'environ 33 pixels
a 13, en dessous de ce que le modele detecte de facon fiable. C'est pourquoi le
feu n'etait vu qu'au dernier moment.

Le modele recoit donc des **fenetres prelevees a la resolution native** plutot
que l'image comprimee. Deux fenetres, aux bords gauche et droit : le feu borde
la chaussee et n'apparait jamais au centre du champ.

    image 640x360
    +---------------+-------+---------------+
    |   fenetre     |       |    fenetre    |
    |   GAUCHE      |       |    DROITE     |  256x256, resolution native
    |   256x256     |       |    256x256    |  soumises au modele
    |               |       |               |
    +---------------+-------+---------------+
    |   ROI du suivi de ligne (40 % du bas) |  seuillage adaptatif + centroide
    +---------------------------------------+

Le feu conserve sa taille reelle, ce qui **double environ la portee de
detection** sans reexporter le modele. La bande centrale n'est pas analysee,
ce qui est sans consequence pour un feu de bord de voie.

Les deux fenetres impliquent **deux inferences par cycle**. Si la charge du MPU
devient genante, augmenter `PERIODE_INFERENCE` dans `boucle_vision.py` : un feu
est immobile, sa detection ne demande pas une cadence elevee.

| Constante | Fichier | Role |
|---|---|---|
| `TAILLE_INFERENCE` | `vision.py` | Cote de la fenetre. **Doit correspondre a l'`imgsz` d'export du modele** |
| `SEUIL_CONFIANCE` | `vision.py` | Un feu lointain sort avec une confiance plus basse |
| `REBOND_ACTIVATION` | `boucle_vision.py` | Detections consecutives avant de signaler un feu |
| `PERIODE_INFERENCE` | `boucle_vision.py` | Cadence des inferences |

## Suivi de ligne autonome

Le module `python/navigation.py` implemente un correcteur **proportionnel-derive** :
la vision fournit l'ecart lateral (pixels) entre le centre de la ligne et le centre
de l'image, et on ralentit la roue du cote vers lequel tourner. Les deux roues avancent
toujours (valeurs positives, contrainte du Bridge).

Le terme derive est indispensable : la vision tourne a 10 Hz et le Bridge ajoute sa
latence. Avec un correcteur purement proportionnel, la correction arrive alors que
l'ecart a deja change de signe : le vehicule oscille d'un bord a l'autre et finit par
perdre la ligne.

| Constante | Role |
|---|---|
| `VITESSE_BASE` | Vitesse des deux roues en ligne droite (0-100) |
| `KP_LATERAL` | Gain proportionnel : pixels d'ecart -> unites de vitesse |
| `KD_LATERAL` | Gain derive : s'oppose aux variations de l'ecart, amortit le depassement |
| `ZONE_MORTE_PX` | Ecart en deca duquel la ligne est jugee centree |
| `CORRECTION_MAX` | Correction maximale (garde les deux roues en avant) |
| `SENS` | Oriente la correction. **-1** est la valeur validee sur le montage actuel ; a reprendre si la camera est reorientee. |
| `MISS_MAX` | Cycles sans ligne avant l'arret |

### Procedure de reglage

Un seul parametre a la fois, dans cet ordre, sinon il est impossible de savoir
lequel agit.

1. **`KD_LATERAL = 0`**, puis diviser `KP_LATERAL` par deux jusqu'a disparition de
   l'oscillation. Le vehicule sera mou en virage : c'est attendu. Noter la valeur.
2. **Remonter `KD_LATERAL`** par paliers jusqu'a retrouver du mordant en virage sans
   redepasser. Trop haut, il rend le vehicule nerveux sur le bruit du centroide.
3. **`ZONE_MORTE_PX`** : augmenter jusqu'a ce que le vehicule cesse de fretiller en
   ligne droite. Trop haute, il louvoie lentement autour de la ligne.
4. **`VITESSE_BASE`** en dernier. Toute augmentation de vitesse deteriore un reglage
   trouve plus lentement, et impose de reprendre a l'etape 1.

`KP_LATERAL` et `KD_LATERAL` dependent de la **resolution de la camera**, l'ecart etant
exprime en pixels bruts. Un changement de resolution invalide tout le reglage.

## References

### Code adapte

- YOLO Live Object Detection, exemple Arduino App Lab (2025). Base du
  chargement du modele ONNX et du post-traitement des sorties dans
  `python/vision.py`.
- Exemple officiel Hiwonder pour carte moteur a encodeur. Origine des
  adresses de registres (0x14, 0x15, 0x33, 0x3C), de leurs unites et de la
  valeur de vitesse de translation, reprises dans `sketch/config.h` et
  `sketch/moteurs.cpp`.

### Algorithmes

- Seuillage adaptatif gaussien, extraction de la ligne
  OpenCV documentation, Image Thresholding
  https://docs.opencv.org/4.x/d7/d4d/tutorial_py_thresholding.html
- Moments d'image, calcul du centroide de la ligne
  Hu, M.-K. (1962). Visual Pattern Recognition by Moment Invariants.
  IRE Transactions on Information Theory, 8(2), 179-187.
- Ouverture morphologique, nettoyage du masque binaire
  Serra, J. (1982). Image Analysis and Mathematical Morphology.
  Academic Press.
- Non-Maximum Suppression, fusion des detections des deux fenetres
  Neubeck, A., & Van Gool, L. (2006). Efficient Non-Maximum Suppression.
  ICPR 2006.
- Espace colorimetrique HSV, classification de la couleur du feu
  Smith, A. R. (1978). Color Gamut Transform Pairs. SIGGRAPH '78.
- Correcteur proportionnel-derive, suivi de ligne
  Ogata, K. (2010). Modern Control Engineering, 5e edition. Prentice Hall.
- Courbe dite exponentielle appliquee au joystick, pratique etablie sur les
  emetteurs de radiocommande. L'implementation courante est un melange
  cubique de la forme (1 - facteur) * v^3 + facteur * v, transpose dans
  `sketch/config.h` avec le coefficient complementaire EXPO = 1 - facteur.
  EdgeTX User Manual, section Inputs
  https://manual.edgetx.org/bw-radios/model-select/inputs-mixes-and-outputs/inputs

### Bibliotheques

| Bibliotheque | Reference | Usage |
|---|---|---|
| YOLOv8n | Jocher et al. (2023), Ultralytics, https://github.com/ultralytics/ultralytics | Detection des feux |
| ONNX Runtime | Microsoft (2021), https://onnxruntime.ai/ | Inference du modele |
| OpenCV | Bradski, G. (2000). The OpenCV Library. Dr. Dobb's Journal, https://opencv.org/ | Capture et traitement d'image |
| NumPy | Harris et al. (2020). Array programming with NumPy. Nature, 585, 357-362 | Calcul sur les sorties du modele |
| Flask | https://flask.palletsprojects.com/ | Serveur web |
| Flask-SocketIO | https://flask-socketio.readthedocs.io/ | Evenements temps reel |
| Eventlet | https://eventlet.net/ | Boucle d'evenements du serveur |
| Socket.IO (v4) | https://socket.io/ | Client temps reel du navigateur |
| Blockly | Google, https://developers.google.com/blockly | Editeur de sequences par blocs |
| Adafruit NeoPixel | https://github.com/adafruit/Adafruit_NeoPixel | Pilotage des bandeaux WS2812B |
| Adafruit GFX, ILI9341, BusIO | https://github.com/adafruit | Affichage sur l'ecran TFT |
| QRCode (Arduino) | Richard Moore (ricmoo), licence MIT, https://github.com/ricmoo/QRCode | Codes QR de connexion |
| Arduino_RouterBridge | Fourni avec le core `arduino:zephyr` | RPC entre le MPU et le MCU |

### Donnees

- Classes COCO, identifiant 9 pour `traffic light`
  Lin, T.-Y. et al. (2014). Microsoft COCO: Common Objects in Context.
  ECCV 2014. https://cocodataset.org/

### Normes et specifications

- ISO/IEC 18004:2015, QR Code bar code symbology specification
- Format d'adhesion WiFi `WIFI:` : projet ZXing, page Barcode Contents
  https://github.com/zxing/zxing/wiki/Barcode-Contents
- QR-Code-generator : Project Nayuki, licence MIT. Cite par l'auteur de
  QRCode comme determinant dans le developpement de sa librairie
  https://www.nayuki.io/page/qr-code-generator-library

### Documentation materielle

- TF-Luna Product Manual, Benewake (`docs/Datasheet/`)
- Arduino UNO Q, fiche technique (`docs/Datasheet/`)
- Ecran LCD ILI9341, fiche technique (`docs/Datasheet/`)
- MPU-6050 Register Map and Descriptions, InvenSense. Origine des adresses
  de registres et de la sensibilite gyroscopique utilisees dans
  `sketch/imu.cpp` et `sketch/config.h`.
- HC-SR04 Ultrasonic Ranging Module, fiche technique du fabricant. Origine
  de l'impulsion de declenchement de 10 us et du facteur de conversion de
  la duree d'echo en distance.
- WS2812B Intelligent Control LED, Worldsemi. Protocole a fil unique
  800 kHz, exigence a l'origine de la version imposee de NeoPixel.
