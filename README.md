# 😀 TankEts_ELE795

Vehicule teleguide autonome — PFE ELE795, Ecole de technologie superieure, ete 2026.

## Architecture materielle

- **Arduino UNO Q** : double processeur (Qualcomm QRB2210 MPU + STM32U585 MCU)
- **Hiwonder Tank** : chassis chenille, carte moteur I2C (0x34)
- **MPU-6050** : IMU gyroscope/accelerometre I2C (0x68)
- **Webcam USB** : detection routiere (YOLO, OpenCV)
- **Bandeaux LED WS2812B** : adressables, modes gyrophare / clignotant / phares
- **Ecran TFT ILI9341** : 320x240, SPI materiel
- **HC-SR04** : capteur ultrason, distance frontale
- **TF-Luna** : LiDAR I2C (0x10), distance frontale

## Architecture logicielle

    sketch/              MCU (STM32) — temps reel, capteurs, actionneurs
      sketch.ino           setup/loop, enregistrement des RPC Bridge
      config.h             Constantes materielles centralisees (broches, adresses, seuils)
      comm_bridge.h/cpp    Bridge RPC, reception des donnees de vision
      moteurs.h/cpp        Carte Hiwonder, encodeurs, I2C bas niveau
      imu.h/cpp            MPU-6050, calibration gyroscope
      deplacement.h/cpp    Machine a etats, joystick, asservissement encodeurs + gyro
      leds.h/cpp           Bandeaux WS2812B (NeoPixel)
      ecran.h/cpp          Ecran ILI9341, affichage
      ultrason.h/cpp       HC-SR04, distance par pulseIn
      lidar.h/cpp          TF-Luna, distance par I2C

    python/              MPU (Qualcomm Linux) — serveur web, vision, Bridge
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

| Peripherique | Broches UNO Q | Notes |
|---|---|---|
| Carte moteur Hiwonder | I2C Wire1 (Qwiic), 0x34 | |
| IMU MPU-6050 | I2C Wire1 (Qwiic), 0x68 | |
| LiDAR TF-Luna | I2C Wire1 (Qwiic), 0x10 | VCC sur **5V**, broche CFG a la **masse** |
| Bandeau LED 1 | D6 (data) | DI (et BI si WS2813) sur la meme broche |
| Bandeau LED 2 | D7 (data) | non connecte pour l'instant |
| Ultrason HC-SR04 | TRIG = D2, ECHO = D3 | ECHO via **pont diviseur 22k/47k** |
| Ecran ILI9341 | CS = D10, DC = D9, RST = D8 | SPI materiel : MOSI D11, SCK D13, MISO D12 |

### Pieges de cablage (verifies sur le terrain)

- **HC-SR04 — pont diviseur obligatoire sur ECHO.** Le capteur sort du 5V ; la
  tolerance 5V des broches D de l'UNO Q n'est **pas garantie** par Arduino
  (seuls certains pads STM32U585 sont 5V-tolerants, et jamais en mode analogique).
  Utiliser 22k (ECHO -> broche) + 47k (broche -> GND), soit ~3.4V.
- **TF-Luna — alimenter en 5V, pas via le 3.3V du Qwiic.** Le capteur demande
  **3.7-5.2V** ; le 3.3V du Qwiic est sous son minimum. On utilise le Qwiic pour
  SDA/SCL/GND, mais VCC vient du 5V.
- **TF-Luna — broche CFG a la masse + power cycle.** CFG (broche 5) reliee a GND
  selectionne le mode I2C. Le mode est **lu au demarrage du capteur** : apres avoir
  branche CFG, il faut **couper et remettre l'alimentation**, sinon il reste en UART
  et n'apparait jamais sur le bus I2C.
- **LEDs et ecran** : logique 3.3V de l'UNO Q, aucun level-shifter necessaire pour
  l'ILI9341. Pour les WS2812B le seuil est limite mais fonctionne en pratique.

## Librairies Arduino — versions critiques

Le core Zephyr de l'UNO Q ne supporte pas les librairies qui accedent directement
aux registres (style AVR). **Les versions ci-dessous sont obligatoires** — des versions
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
default_profile: default
```

| Librairie | Version | Pourquoi cette version |
|---|---|---|
| Adafruit NeoPixel | **1.15.5** | Les versions anterieures (ex. 1.12.0) **compilent** mais leur timing bit-bang 800 kHz ne produit **aucun signal** sur STM32U585/Zephyr. Symptome : LEDs completement eteintes. |
| Adafruit BusIO | **1.17.4** | Contient le guard `#elif defined(__MBED__) \|\| defined(__ZEPHYR__)` qui desactive `BUSIO_USE_FAST_PINIO`. Les versions < 1.15 **ne compilent pas** (erreur `portOutputRegister` / `digitalPinToPort`). |
| Adafruit GFX / ILI9341 | 1.12.6 / 1.6.3 | Dependances de l'ecran, dernieres versions stables. |

Les autres librairies visibles a la compilation (`Arduino_RouterBridge`, `Arduino_RPClite`,
`MsgPack`, `DebugLog`, `ArxTypeTraits`, `ArxContainer`, `Wire`) sont des dependances
**automatiques** du systeme Bridge et du core. Ne pas les epingler dans `sketch.yaml` :
elles forment un ensemble coherent livre avec le Bridge.

## Guide de deploiement — Arduino UNO Q

### Prerequis

- Arduino App Lab installe (v0.8+)
- Arduino UNO Q connecte par USB au PC
- Webcam USB branchee

### Etape 1 : Configuration de l'Arduino UNO Q en mode point d'acces

Creer un point d'acces (AP) — remplacer le SSID et le mot de passe :

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
| Camera non disponible | La camera USB n'est pas sur l'index attendu — le code scanne automatiquement `/dev/video0` a `/dev/video9` |
| Terminal Python vide | Les dependances pip s'installent au premier demarrage, patienter ~30s |
| Erreur `ModuleNotFoundError` | Verifier que `requirements.txt` est a jour et relancer Run |
| Le serveur ne demarre pas hors ligne | `main.py` enveloppe l'install pip dans un `try/except` : en mode point d'acces (sans Internet) l'install est ignoree et le serveur demarre quand meme |
| `Error: verify failed in bank at 0x08000000` | Le flash a echoue : le MCU tourne **l'ancien code**. Relancer Run (parfois 2-3 fois) jusqu'a un flash propre. Si une modification semble sans effet, c'est souvent ca. |
| `ld: file truncated` / `invalid string offset` / segfault du linker | Cache de compilation corrompu. Supprimer `.cache/sketch` sur la carte (garder `.cache/.venv` qui contient l'environnement Python) : `rm -rf ~/ArduinoApps/tankets_ele795/.cache/sketch` |
| LEDs completement eteintes | Verifier `Adafruit NeoPixel (1.15.5)` dans `sketch.yaml` — les versions anterieures compilent mais ne pilotent rien |
| Ecran : erreur `portOutputRegister` a la compilation | Verifier `Adafruit BusIO (1.17.4)` — les versions < 1.15 ne compilent pas sur Zephyr |
| LiDAR `capteur present : NON` | CFG (broche 5) a la masse ? Alimentation >= 3.7V ? **Power cycle apres avoir branche CFG** ? SDA/SCL non inverses ? |
| Un capteur absent rend la carte muette | Le Bridge est enregistre **avant** les inits materielles dans `setup()`, donc le controle survit a un capteur defaillant. Si le probleme persiste, verifier qu'aucune init bloquante n'a ete deplacee avant les `provide_safe`. |

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
| `mode_led1`, `mode_led2` | int | 0=off, 1=gyro, 2=cligno, 3=phares |
| `lire_ultrason_cm` | aucun, retourne int | Distance frontale HC-SR04 (cm) |
| `lire_lidar_cm` | aucun, retourne int | Distance frontale TF-Luna (cm), -1 si invalide |

RPC dans l'autre sens (MCU recoit du MPU) : `on_feu(bool, int, int)`, `on_lignes(bool, int)`.

**Note** : le Bridge ne transmet pas correctement les nombres negatifs. Toujours envoyer
des valeurs positives — le sens est gere par des fonctions dediees cote MCU (c'est
pourquoi `roues` prend deux vitesses positives plutot qu'un differentiel signe).

## Suivi de ligne autonome

Le module `python/navigation.py` implemente un suivi proportionnel :
la vision fournit l'ecart lateral (pixels) entre le centre de la ligne et le centre
de l'image, et on ralentit la roue du cote vers lequel tourner. Les deux roues avancent
toujours (valeurs positives, contrainte du Bridge).

Parametres a ajuster sur le terrain (`navigation.py`) :

| Constante | Role |
|---|---|
| `VITESSE_BASE` | Vitesse des deux roues en ligne droite (0-100) |
| `KP_LATERAL` | Gain : pixels d'ecart -> unites de vitesse |
| `CORRECTION_MAX` | Correction maximale (garde les deux roues en avant) |
| `SENS` | **-1 actuellement** (camera inversee). A remettre a **1** quand la coque 3D sera montee. |
| `MISS_MAX` | Cycles sans ligne avant l'arret |

## References

- YOLOv8n — Jocher et al. (2023), Ultralytics
- OpenCV — Bradski, G. (2000), The OpenCV Library
- ONNX Runtime — Microsoft (2021)
- Canny Edge Detection — Canny, J. (1986), IEEE Trans. PAMI
- Hough Transform probabiliste — Matas et al. (2000), CVIU
- Adaptive Thresholding — OpenCV documentation
- Courbe de Bezier quadratique — tracee par echantillonnage (`ecran.cpp`)
- TF-Luna Product Manual — Benewake (`docs/Datasheet/`)
