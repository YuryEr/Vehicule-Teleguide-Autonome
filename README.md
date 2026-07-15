# 😀 TankEts_ELE795

Vehicule teleguide autonome — PFE ELE795, Ecole de technologie superieure, ete 2026.

## Architecture materielle

- **Arduino UNO Q** : double processeur (Qualcomm QRB2210 MPU + STM32U585 MCU)
- **Hiwonder Tank** : chassis chenille, carte moteur I2C (0x34)
- **MPU-6050** : IMU gyroscope/accelerometre I2C (0x68)
- **Webcam USB** : detection routiere (YOLO, OpenCV)
- **HC-SR04 / TF-Luna** : capteurs de distance (a venir)

## Architecture logicielle

    sketch/              MCU (STM32) — moteurs, IMU, Bridge RPC
    python/              MPU (Qualcomm Linux) — serveur web, vision, Bridge
      main.py              Point d'entree
      serveur_web.py       Flask + SocketIO + streaming video
      comm_bridge.py       Communication Bridge RPC (MPU <-> MCU)
      vision.py            Detection YOLO + lignes (Canny/Hough)
      boucle_vision.py     Pipeline de vision a cadences
      navigation.py        Navigation autonome (a venir)
      requirements.txt     Dependances Python
    assets/              Interface web
      index.html           Page principale
      style.css            Styles
      app.js               Logique JS (joystick, Blockly, SocketIO)
      libs/                Blockly + Socket.IO embarques

## Guide de deploiement — Arduino UNO Q

### Prerequis

- Arduino App Lab installe (v0.8+)
- Arduino UNO Q connecte par USB au PC
- Webcam USB branchee sur le hub USB de la carte

### Etape 1 : Configuration du arduino uno q en mode point d'accès
```yaml
# Créé un point d'accès (AP). Remplacer le SSID/mot de passe.
`nmcli device wifi hotspot ssid TankETS password tank1234 ifname wlan0`

# Appliquer automatiquement à chaque démarrage :
`nmcli connection modify Hotspot connection.autoconnect yes`
```

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

## Conventions de code

- **Python** : snake_case (fonctions, variables, fichiers)
- **JavaScript** : camelCase (fonctions, variables), UPPER_CASE (constantes)
- **HTML** : kebab-case (IDs, classes)
- **C/Arduino** : PascalCase + prefixe module (ex: `CommBridge_Initialiser`)
- **Fichiers .h** : documentation en francais, format bloc majuscule

## Contrat RPC Bridge (Python vers MCU)

| Fonction | Parametres | Description |
|----------|-----------|-------------|
| `joy_x`, `joy_y` | float | Pilotage manuel joystick |
| `avancer_metres` | float (positif) | Deplacement avant asservi |
| `reculer_metres` | float (positif) | Deplacement arriere asservi |
| `tourner_gauche_deg` | float (positif) | Rotation gauche asservie |
| `tourner_droite_deg` | float (positif) | Rotation droite asservie |
| `arreter_mouvement` | aucun | Arret d'urgence |
| `mouvement_actif` | aucun, retourne int (0/1) | Polling fin de mouvement |
| `mode_led1`, `mode_led2` | int | 0=off, 1=gyro, 2=cligno, 3=phares |

**Note** : le Bridge ne transmet pas correctement les nombres negatifs. Toujours envoyer des valeurs positives.

## References

- YOLOv8n — Jocher et al. (2023), Ultralytics
- OpenCV — Bradski, G. (2000), The OpenCV Library
- ONNX Runtime — Microsoft (2021)
- Canny Edge Detection — Canny, J. (1986), IEEE Trans. PAMI
- Hough Transform probabiliste — Matas et al. (2000), CVIU
