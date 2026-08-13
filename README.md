# Vehicule teleguide autonome (VTA)

PFE ELE795, Ecole de technologie superieure, ete 2026.  
Par : Yury Ereshchenko, Yoan Sapet, Vlad Alexandru Ilie, Ryan Leung et Serby Brian Barthelemy

Le sigle **VTA** sert partout ou la place manque : nom du point d'acces WiFi,
titre a l'ecran embarque, en-tetes des modules.

## Architecture materielle

- **Arduino UNO Q** : double processeur (Qualcomm QRB2210 MPU + STM32U585 MCU)
- **Hiwonder Tank** : chassis chenille, carte moteur I2C (0x34)
- **MPU-6050** : IMU gyroscope/accelerometre I2C (0x68)
- **Webcam USB** : detection routiere (YOLO, OpenCV). Cable modifie, alimentee
  par le convertisseur et non par le port USB-C
- **Bandeaux LED WS2812B** : 7 LEDs par bandeau : barre haute (gyrophare, feux de
  position), feux avant/arriere et clignotants de virage
- **Ecran TFT ILI9341** : 320x240, SPI materiel
- **HC-SR04** : capteur ultrason, detection de presence frontale (cone large)
- **TF-Luna** : LiDAR I2C (0x10), telemetre frontal (FOV 2 deg)
- **Servo SG90** : support orientable du LiDAR, sondage par secteurs
- **Convertisseur abaisseur (buck)** : rail 5V du montage, alimente depuis la
  batterie du chassis et non depuis l'UNO Q

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
      sketch.yaml          Profil de compilation, versions de librairies imposees

    python/              MPU (Qualcomm Linux) : serveur web, vision, Bridge
      main.py              Point d'entree, installation pip puis demarrage
      serveur_web.py       Flask + SocketIO, flux video, sequenceur de blocs
      comm_bridge.py       Communication Bridge RPC (MPU <-> MCU)
      vision.py            Feux (YOLO sur deux fenetres) et lignes
                           (seuillage adaptatif + centroide)
      boucle_vision.py     Anti-rebond des feux, cadences des deux boucles
      navigation.py        Suivi de ligne, correcteur proportionnel-derive
      requirements.txt     Dependances Python
      yolov8n.onnx         Modele de detection, entree figee a 256x256

    assets/              Interface web
      index.html           Page principale
      style.css            Styles
      app.js               Logique JS (joystick, Blockly, SocketIO)
      libs/blockly/        Editeur de blocs, embarque (pas de CDN)
      libs/socket.io.min.js  Client temps reel, embarque

Les deux librairies de `libs/` sont **embarquees volontairement** : en mode point
d'acces la carte n'a pas d'acces Internet, et un chargement par CDN laisserait
l'interface vide.

## Brochage

Toutes les broches numeriques sont declarees dans `sketch/config.h` : c'est la
seule source de verite du code. Les tableaux ci-dessous doivent lui correspondre.

### Alimentation

Le montage compte **trois sources distinctes**, et aucune ne provient du rail 5V
de l'UNO Q.

    Batterie du chassis
      |
      +---> Carte moteur Hiwonder (alimentation directe)
      |
      +---> Convertisseur abaisseur (buck) ---> Rail 5V du montage
      |                                           |
      |                                           +--> LiDAR TF-Luna
      |                                           +--> Ultrason HC-SR04
      |                                           +--> Servo SG90
      |                                           +--> Bandeaux LED WS2812B
      |                                           +--> Ecran ILI9341 (VCC et LED)
      |                                           +--> Webcam USB (VBUS seul)
      |
      +---> Masse commune <---------------------> GND de l'UNO Q

    L'UNO Q ne fournit que le 3.3V du connecteur Qwiic (IMU MPU-6050).
    Le port USB-C ne porte que les donnees de la camera, pas son alimentation.

| Composant | Tension | Source | Remarque |
|---|---|---|---|
| Carte moteur Hiwonder | Batterie | Batterie du chassis, directement | Jamais depuis l'UNO Q |
| IMU MPU-6050 | 3.3V | Connecteur Qwiic de l'UNO Q | Seul peripherique alimente par la carte |
| LiDAR TF-Luna | **5V** | Convertisseur abaisseur | **Pas** le 3.3V du Qwiic, voir plus bas |
| Ultrason HC-SR04 | 5V | Convertisseur abaisseur | Logique TRIG pilotee en 3.3V par l'UNO Q |
| Servo SG90 | 5V | Convertisseur abaisseur | Appels de courant au demarrage du mouvement |
| Bandeaux LED WS2812B | 5V | Convertisseur abaisseur | Voir le bilan de courant ci-dessous |
| Ecran ILI9341, `VCC` | **5V** | Convertisseur abaisseur | Le regulateur du module descend en 3.3V |
| Ecran ILI9341, `LED` | **5V** | Convertisseur abaisseur | Retroeclairage. Sous 3.3V l'ecran reste noir alors que le SPI repond |
| Webcam USB | 5V | Convertisseur abaisseur | **Cable modifie**, alimentation separee des donnees, voir plus bas |

**Le 5V vient d'un convertisseur abaisseur, pas de l'UNO Q.** Un module buck
descend la tension de la batterie du chassis a 5V et alimente tous les
peripheriques de puissance. Deux raisons : le rail 5V de la carte n'a pas la
reserve de courant necessaire aux bandeaux et au servo, et faire transiter ces
appels de courant par la carte de commande y ferait chuter la tension au moment
precis ou elle pilote.

**La masse doit rester commune, sinon rien ne fonctionne.** C'est la contrepartie
obligatoire d'une alimentation separee : le GND du convertisseur, celui de la
batterie et celui de l'UNO Q doivent etre relies. Les signaux de commande sortent
de l'UNO Q et sont lus par des composants alimentes ailleurs ; sans reference de
tension partagee, un niveau haut n'a plus de sens pour eux et rien ne repond,
alors que chaque bloc pris isolement semble correctement alimente.

**L'ecran a besoin de 5V sur ses deux entrees d'alimentation.** `VCC` traverse le
regulateur du module de decouplage, et `LED` alimente le retroeclairage. Une erreur
classique consiste a n'alimenter que `VCC` : le controleur repond correctement sur
le SPI, le code s'execute sans erreur, mais la dalle reste noire faute de
retroeclairage.

**Bilan de courant des bandeaux.** Un WS2812B consomme environ 60 mA au blanc
plein (20 mA par canal). Le montage porte 14 LEDs, soit **environ 840 mA** dans le
pire cas, `LUMINOSITE_BANDEAU` etant a 255. Le cas ne se presente pas en usage
normal, les feux etant blancs ou rouges et la barre rarement toute allumee, mais
c'est ce chiffre qui dimensionne le convertisseur : il doit tenir cette pointe en
plus du servo et des deux capteurs. Si les LEDs scintillent, verifier le calibre du
convertisseur avant de soupconner le code, et au besoin baisser
`LUMINOSITE_BANDEAU`.

### Webcam USB, cable modifie

Le cable de la camera est **ouvert et rebranche sur deux sources** : les quatre
conducteurs ne vont plus tous au meme endroit.

| Conducteur | Destination | Role |
|---|---|---|
| VBUS (rouge, 5V) | Convertisseur abaisseur | Alimentation de la camera |
| D+ (vert) | Port USB-C de l'UNO Q | Donnee |
| D- (blanc) | Port USB-C de l'UNO Q | Donnee |
| GND (noir) | Masse commune | Reference, partagee avec le convertisseur et l'UNO Q |

Deux raisons imposent ce montage, constatees sur cette carte :

1. **L'UNO Q n'alimente pas la camera par son port USB-C.** Le courant disponible
   sur ce port ne suffit pas a la faire demarrer.
2. **Brancher quoi que ce soit sur l'USB-C fait perdre le 5V de la carte.** Le
   rail 5V de l'UNO Q s'effondre des qu'un peripherique occupe ce port, ce qui
   priverait au passage tout ce qui en dependrait encore.

Prendre le VBUS sur le convertisseur resout les deux d'un coup : la camera a le
courant qu'il lui faut, et le port USB-C ne sert plus qu'a transporter les
donnees. **La masse doit rester commune** entre le convertisseur et l'UNO Q, sinon
la paire differentielle D+/D- n'a plus de reference et l'hote ne voit aucun
peripherique.

C'est aussi la raison pour laquelle l'ensemble du montage est alimente par le
convertisseur : le 5V de la carte n'est pas disponible de facon fiable des que la
camera est branchee.

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
| LiDAR TF-Luna | 0x10 | **5V** du convertisseur | Broche **CFG a la masse** + power cycle |

**TF-Luna, alimenter en 5V.** Le capteur demande 3.7-5.2V ; le 3.3V du Qwiic est
sous son minimum. On utilise le Qwiic pour SDA/SCL/GND, mais VCC vient du rail
5V du convertisseur abaisseur.

**TF-Luna, CFG a la masse, puis power cycle.** CFG (broche 5) reliee a GND
selectionne le mode I2C. Le mode est **lu au demarrage du capteur** : apres avoir
branche CFG, il faut **couper et remettre l'alimentation**, sinon le capteur reste
en UART et n'apparait jamais sur le bus.

**Sans aucun peripherique sur le Qwiic**, les lignes flottent faute de resistances
de tirage et toutes les transactions I2C partent en timeout.

### Ultrason HC-SR04

| Broche du capteur | Broche UNO Q | Role |
|---|---|---|
| VCC | 5V du convertisseur | Alimentation |
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
| Rouge | 5V du convertisseur | Alimentation |
| Brun / noir | GND | Masse, commune avec la carte et le convertisseur |

La librairie `Servo` est inutilisable sur ce core (voir plus bas) : l'impulsion est
generee par `servo_lidar.cpp`.

### Bandeaux LED WS2812B

| Broche du bandeau | Broche UNO Q | Role |
|---|---|---|
| DI (bandeau avant) | D6 | Donnees. Sur WS2813, relier BI a la meme broche |
| DI (bandeau arriere) | D7 | Donnees |
| VCC | 5V du convertisseur | Alimentation, voir le bilan de courant |
| GND | GND | Masse, commune avec la carte |

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
| VCC | **5V** du convertisseur | Alimentation du module |
| LED | **5V** du convertisseur | Retroeclairage, indispensable pour voir quoi que ce soit |
| GND | GND | Masse |

Les six lignes de signal sont celles declarees dans `config.h` et le materiel SPI de
la carte. Aucun level-shifter n'est necessaire : l'ILI9341 accepte la logique 3.3V
de l'UNO Q sur ses entrees de signal, mais son alimentation et son retroeclairage
demandent tous deux **5V**. Un ecran noir avec un SPI qui repond signale presque
toujours un `LED` non alimente.

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
nmcli device wifi hotspot ssid VTA password admin123 ifname wlan0
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
| Camera non disponible | La camera USB n'est pas sur l'index attendu, le code scanne automatiquement `/dev/video0` a `/dev/video9`. Si aucun index ne repond, verifier le cable modifie : VBUS sur le convertisseur, D+ et D- sur l'USB-C, **masse commune**. Sans masse partagee la paire differentielle n'a pas de reference et l'hote ne detecte aucun peripherique |
| Terminal Python vide | Les dependances pip s'installent au premier demarrage, patienter ~30s |
| Erreur `ModuleNotFoundError` | Verifier que `requirements.txt` est a jour et relancer Run |
| Le serveur ne demarre pas hors ligne | `main.py` enveloppe l'install pip dans un `try/except` : en mode point d'acces (sans Internet) l'install est ignoree et le serveur demarre quand meme |
| `Error: verify failed in bank at 0x08000000` | Le flash a echoue : le MCU tourne **l'ancien code**. Relancer Run (parfois 2-3 fois) jusqu'a un flash propre. Si une modification semble sans effet, c'est souvent ca. |
| `ld: file truncated` / `invalid string offset` / segfault du linker | Cache de compilation corrompu. Supprimer `.cache/sketch` sur la carte (garder `.cache/.venv` qui contient l'environnement Python) : `rm -rf ~/ArduinoApps/tankets_ele795/.cache/sketch` |
| LEDs completement eteintes | Verifier `Adafruit NeoPixel (1.15.5)` dans `sketch.yaml`, les versions anterieures compilent mais ne pilotent rien |
| Ecran : erreur `portOutputRegister` a la compilation | Verifier `Adafruit BusIO (1.17.4)`, les versions < 1.15 ne compilent pas sur Zephyr |
| Ecran noir alors que le code s'execute | La broche `LED` du module n'est pas alimentee. Le controleur repond sur le SPI et le programme se deroule normalement, mais sans retroeclairage la dalle reste noire. `VCC` **et** `LED` demandent 5V |
| LEDs qui scintillent | Calibre du convertisseur abaisseur. 14 WS2812B au blanc plein demandent environ 840 mA, en plus du servo et des capteurs. Verifier le convertisseur avant le code, au besoin baisser `LUMINOSITE_BANDEAU` |
| Un peripherique alimente ne repond a rien | Masse commune. Le convertisseur, la batterie et l'UNO Q doivent partager leur GND, sans quoi les signaux de commande n'ont plus de reference et chaque bloc semble pourtant correctement alimente |
| Servo qui tremble sur place sans se deplacer | Comportement attendu si `SERVO_MAINTIEN_MS` est augmente : chaque impulsion regeneree porte le jitter de l'attente active. Le train d'impulsions doit s'arreter une fois la cible atteinte |
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

## Modes de conduite

Le MCU connait deux regimes, fixes par la RPC `definir_mode`. L'interface web
ajoute un troisieme usage, le mode blocs, qui n'est pas un mode du MCU mais une
sequence de commandes envoyees depuis le MPU.

| Mode | Veto de securite | Origine des commandes |
|---|---|---|
| Manuel | Aucun | Joystick de l'interface web |
| Autonome | Actif | Suivi de ligne calcule sur le MPU |
| Blocs | Aucun | Sequenceur de `serveur_web.py`, un bloc a la fois |

**Le veto ne s'applique qu'en mode autonome.** En manuel le pilote voit le
vehicule et garde le controle complet, y compris pour l'approcher volontairement
d'un obstacle. Le laisser actif rendrait le pilotage incomprehensible, la carte
refusant des commandes sans que rien ne l'explique a l'ecran.

Le veto ne bloque que les **avances**, c'est-a-dire les commandes ou les deux
chenilles poussent vers l'avant. Les rotations et la marche arriere restent
autorisees : ce sont precisement les manoeuvres qui degagent le vehicule.

Deux causes distinctes le declenchent, et elles n'ont pas la meme suite :

| Cause | Consequence |
|---|---|
| Obstacle sous `OBSTACLE_SEUIL_CM` | Arret, puis lancement du contournement |
| Feu rouge ou jaune detecte | Arret seul, reprise des que le feu passe au vert |

Le jaune est traite comme le rouge : devant un feu sur le point de changer,
s'arreter est le comportement attendu. Une detection de feu expire au bout de
`FEU_AGE_MAX_MS`, faute de quoi une perte de la liaison de vision immobiliserait
le vehicule indefiniment.

## Detection d'obstacles et contournement

### Fusion des deux capteurs

La distance frontale retenue est la **plus petite des mesures valides** entre
l'ultrason et le LiDAR. Les deux capteurs ne mesurent pas la meme chose et c'est
la raison de les garder tous les deux :

| Capteur | Champ | Role |
|---|---|---|
| HC-SR04 | Cone large | Presence d'un obstacle quelque part devant |
| TF-Luna | 2 degres | Telemetre precis, mais aveugle a ce qui n'est pas dans l'axe |

Un LiDAR seul manque un obstacle decale de quelques degres ; un ultrason seul
donne une distance grossiere. Chaque mesure est ramenee au pare-choc avant par
`ULTRASON_RECUL_CM` et `LIDAR_RECUL_CM`, sans quoi la fusion comparerait deux
distances prises depuis deux origines differentes.

### Machine a etats du contournement

Le servo n'existe que pour le sondage : il n'y a pas de balayage permanent, le
champ du LiDAR etant trop etroit pour cartographier quoi que ce soit en un temps
raisonnable.

Le trajet forme un **trapeze** : une diagonale qui ecarte le vehicule de la
ligne, un segment parallele qui depasse l'obstacle, puis une diagonale de retour
vers la ligne.

                       EVITEMENT_LONGEMENT_M
                      +---------------+           segment parallele a la ligne
                     /                 \      angles 1 et 2 : ANGLE_DEG
        ____________/    [obstacle]     \___  angle 3 : ANGLE_RETOUR_DEG
             ligne suivie

    REPOS
      | obstacle detecte en mode autonome
      v
    PAUSE --> SONDAGE             trois secteurs, +/- OBSTACLE_ECART_SONDAGE_DEG
      |                           retient le cote le plus degage
      v
    PAUSE --> ROTATION_ALLER      pivote de EVITEMENT_ANGLE_DEG vers ce cote
      |
      v
    PAUSE --> AVANCE_DIAGONALE    avance de EVITEMENT_DISTANCE_M
      |
      v
    PAUSE --> ROTATION_PARALLELE  rend l'angle d'ecartement, cap parallele
      |                           a la ligne
      v
    PAUSE --> AVANCE_PARALLELE    avance de EVITEMENT_LONGEMENT_M
      |
      v
    PAUSE --> ROTATION_RETOUR     EVITEMENT_ANGLE_RETOUR_DEG vers la ligne
      |
      v
    REPOS                         le suivi de ligne reprend la main

**Chaque etape est precedee d'une immobilisation de `EVITEMENT_PAUSE_MS`.** La
carte moteur maintient sa derniere consigne tant qu'on ne lui en donne pas
d'autre : sans arret explicite, les etapes s'enchainent en roulant et se
superposent.

**Les trois rotations ne sont pas dans le meme sens.** La premiere ecarte de la
ligne, du cote retenu par le sondage. Les deux suivantes vont a l'oppose : la
deuxieme rend exactement l'angle d'ecartement et remet le cap d'origine, la
troisieme dirige le vehicule vers la ligne, qu'il recroise. Sans cette troisieme
rotation, il roulerait parallelement a la ligne sans jamais la revoir et le
suivi s'arreterait apres `MISS_MAX` cycles.

**La rotation de retour est plus faible que celle d'ecartement**, et pour une
raison qui n'a rien de geometrique : a 45 degres la ligne sort du champ de la
camera avant d'etre recroisee, et le suivi s'arrete sans jamais la retrouver.
A 20 degres elle reste en vue, au prix d'une approche plus longue.

| Angle de retour | Distance parcourue avant de recroiser la ligne |
|---|---|
| 45 degres | 45 cm, mais ligne hors du champ |
| 30 degres | 64 cm |
| **20 degres** | **93 cm, valeur retenue** |
| 15 degres | 123 cm |

**La deuxieme rotation rend l'angle cumule, pas une valeur fixe.** Si la voie
reste bloquee apres la premiere rotation, un cran supplementaire est ajoute,
jusqu'a `EVITEMENT_ESSAIS_MAX` fois. La remise parallele doit alors annuler le
total, sans quoi le segment cense etre parallele partirait de travers.

**Dimensionnement.** L'ecart lateral obtenu vaut `EVITEMENT_DISTANCE_M` multiplie
par le sinus de l'angle d'ecartement, soit 32 cm pour 45 cm a 45 degres. Noter
que la constante decrit la distance **parcourue** sur la diagonale, pas l'ecart
obtenu. Il doit depasser le
demi-encombrement de l'obstacle, sans quoi le segment parallele le percute. Le
segment parallele doit lui-meme depasser la profondeur de l'obstacle avant que le
vehicule ne se reoriente vers la ligne.

## Ecran de connexion

L'ecran affiche une page unique portant **deux codes QR cote a cote**, de facon
qu'un visiteur puisse rejoindre le vehicule sans que personne ne lui dicte un mot
de passe.

| Code QR | Contenu | Effet au scan |
|---|---|---|
| Gauche | `WIFI:T:WPA;S:<ssid>;P:<mdp>;;` | Propose de rejoindre le point d'acces |
| Droite | `http://<ip>:7000` | Ouvre l'interface dans le navigateur |

Le format d'adhesion WiFi `WIFI:` est un schema de fait defini par le projet
ZXing, reconnu nativement par iOS 11 et par Android 10 et versions ulterieures.

| Constante | Valeur | Role |
|---|---|---|
| `QR_VERSION` | 3 | 29x29 modules, 53 octets en correction basse |
| `QR_TAILLE_MODULE` | 4 | Cote d'un module, en pixels |
| `QR_ZONE_SILENCE` | 4 | Marge blanche en modules, exigee par la norme |

La zone de silence n'est pas decorative : sans elle, un lecteur ne delimite pas
le symbole et le scan echoue sur la plupart des telephones.

### Origine des trois informations

Les trois valeurs sont des constantes de `sketch/config.h`, et **doivent
correspondre a la commande `nmcli` du guide de deploiement** :

| Constante | Valeur | Origine |
|---|---|---|
| `RESEAU_SSID` | `VTA` | Choisi dans la commande `nmcli` |
| `RESEAU_MOT_DE_PASSE` | `admin123` | Idem |
| `RESEAU_IP` | `10.42.0.1` | Consequence de la commande, voir ci-dessous |

L'adresse n'est pas arbitraire : en mode partage, NetworkManager attribue
**10.42.0.1** a l'interface et distribue le reste du sous-reseau 10.42.0.0/24
aux clients. Creer le point d'acces fixe donc l'adresse au meme titre que le
SSID, et rien n'a besoin de la decouvrir a l'execution.

**Cela ne tient que si la carte cree son reseau.** Si elle en rejoint un, son
adresse vient d'un DHCP : il faut alors la relever avec `ip addr show wlan0` et
la reporter dans `config.h`. C'est la contrepartie assumee de la simplicite,
acceptable parce que le mode point d'acces est celui du deploiement.

## Signalisation lumineuse

Chaque bandeau porte 7 LEDs sur une seule ligne de donnees, decoupees en deux
zones par le logiciel. Les deux zones partagent la meme luminosite materielle :
le contraste entre la barre et les feux se fait par les valeurs RVB.

| Zone | Pixels | Comportement |
|---|---|---|
| Barre haute | 0 a 4 | Eteinte, feux de position, ou gyrophare (`PERIODE_GYROPHARE_MS`) |
| Feux | 5 et 6 | Blancs a l'avant, rouges a l'arriere |
| Clignotants | 5 ou 6 | Orange sur le cote du virage (`PERIODE_CLIGNOTANT_MS`) |

Le cote de chaque pixel est fixe par `PIXEL_COTE_DROIT` et `PIXEL_COTE_GAUCHE` :
les bandeaux avant et arriere se faisant face, l'ordre physique des pixels
s'inverse de l'un a l'autre.

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

    Image capturee par la webcam : 640 x 360

    x=0          x=256   x=384        x=640
    +--------------+-------+--------------+   y=0
    |##############|       |##############|
    |## FENETRE  ##| bande |## FENETRE  ##|
    |##  GAUCHE  ##|  non  |##  DROITE  ##|
    |##256 x 256 ##|analyse|##256 x 256 ##|
    |##############| 128px |##############|
    |XXXXXXXXXXXXXX|=======|XXXXXXXXXXXXXX|   y=216
    +--------------+=======+--------------+   y=256
    |=====================================|
    |== ROI SUIVI DE LIGNE : 640 x 144 ===|
    +-------------------------------------+   y=360

    ###  fenetres soumises au modele, prelevees a la resolution native
    ===  region du seuillage adaptatif et du centroide de la ligne
    XXX  recouvrement des deux traitements, 40 pixels de haut

Les deux traitements lisent la meme image mais ne s'ignorent pas
geometriquement : les fenetres descendent jusqu'a `y = 256` et le ROI de ligne
commence a `y = 216`, soit 40 pixels communs. Sans consequence, les deux chaines
etant independantes, mais a savoir avant de deplacer l'une ou l'autre.

Une bande centrale de 128 pixels de large n'est jamais soumise au modele. C'est
sans consequence pour un feu de bord de voie, mais un feu place au centre du
champ ne serait pas detecte.

Le feu conserve ainsi sa taille reelle, ce qui **double environ la portee de
detection** sans avoir a reexporter le modele.

Les deux fenetres impliquent **deux inferences par cycle**. Si la charge du MPU
devient genante, augmenter `PERIODE_INFERENCE` dans `boucle_vision.py` : un feu
est immobile, sa detection ne demande pas une cadence elevee.

La couleur du feu n'est pas donnee par le modele, qui ne connait que la classe
`traffic light` de COCO. Elle est determinee ensuite, par comptage de pixels dans
trois plages HSV appliquees a la boite detectee. Le rouge occupe **deux** plages,
la teinte passant par zero en HSV.

Les constantes de reglage sont regroupees plus bas, dans **Constantes de reglage
du MPU**.

## Suivi de ligne autonome

Le module `python/navigation.py` implemente un correcteur **proportionnel-derive** :
la vision fournit l'ecart lateral (pixels) entre le centre de la ligne et le centre
de l'image, et on ralentit la roue du cote vers lequel tourner. Les deux roues avancent
toujours (valeurs positives, contrainte du Bridge).

Le terme derive est indispensable : la vision tourne a 10 Hz et le Bridge ajoute sa
latence. Avec un correcteur purement proportionnel, la correction arrive alors que
l'ecart a deja change de signe : le vehicule oscille d'un bord a l'autre et finit par
perdre la ligne.

| Constante | Valeur | Role |
|---|---|---|
| `VITESSE_BASE` | 15 | Vitesse des deux roues en ligne droite (0-100) |
| `KP_LATERAL` | 0.015 | Gain proportionnel : pixels d'ecart vers unites de vitesse |
| `KD_LATERAL` | 0.035 | Gain derive : s'oppose aux variations de l'ecart, amortit le depassement |
| `ZONE_MORTE_PX` | 35 | Ecart en deca duquel la ligne est jugee centree |
| `CORRECTION_MAX` | 8 | Correction maximale, garde les deux roues en avant |
| `SENS` | -1 | Oriente la correction. **-1** est la valeur validee sur le montage actuel, a reprendre si la camera est reorientee |
| `MISS_MAX` | 8 | Cycles sans ligne avant l'arret |

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

## Constantes de reglage du MCU

Toutes dans `sketch/config.h`. Les valeurs indiquees sont celles du montage
actuel ; celles marquees d'une etoile ont ete relevees par la mesure, les autres
sont des valeurs d'origine ou reprises d'une documentation.

### Calibration mecanique

| Constante | Valeur | Role |
|---|---|---|
| `DIAMETRE_ROUE_MM` | 65.0 | Diametre du barbotin |
| `RATIO_REDUCTEUR` | 50.0 | **Jamais mesure**, voir l'avertissement ci-dessous |
| `IMPULSIONS_PAR_TOUR` | 44.0 | Impulsions par tour d'aimant du JGB37-520 |
| `SENSIBILITE_GYRO` | 131.0 | LSB par deg/s a pleine echelle +/-250 |

`DIAMETRE_ROUE_MM` et `RATIO_REDUCTEUR` n'interviennent que par leur **produit**
dans `Moteurs_PulsesEnMetres`, et le glissement de la chenille s'y cache : ce sont
des constantes ajustees par la mesure, pas des grandeurs relevables sur une fiche
produit. Le fabricant annonce 90 pour le JGB37-520, valeur qui donne le double de
la distance reelle sur ce chassis. Reglage : commander 2 m, mesurer, puis
`RATIO_nouveau = RATIO_actuel * (commandee / mesuree)`.

### Deplacements asservis

| Constante | Valeur | Role |
|---|---|---|
| `VITESSE_DEPLACEMENT` | 12 | Translation, en impulsions par 10 ms |
| `VITESSE_ROTATION` | 11 | Rotation, volontairement lente pour la reproductibilite |
| `ROT_MARGE_ARRET_DEG` | 10.0 * | Compensation de l'inertie de fin de rotation |
| `ROT_MARGE_LENTE_DEG` | 20.0 | Debut de l'approche lente, a garder au-dessus de la marge d'arret |
| `AVANCE_TRIM_NUM` / `_DEN` | 5 / 16 * | Compensation de derive, fraction d'unite de consigne |
| `REEMISSION_MOTEUR_MS` | 100 | Repetition de la consigne pendant un mouvement asservi |
| `TIMEOUT_AVANCE_MS` | 12000 | Abandon d'une avance qui n'aboutit pas |
| `TIMEOUT_ROTATION_MS` | 8000 | Abandon d'une rotation qui n'aboutit pas |

En boucle fermee, l'unite de vitesse est le **nombre d'impulsions par 10 ms** et
la plage utile va jusqu'a environ 50 selon la charge et la tension.

Toute modification de `VITESSE_ROTATION` impose de reprendre les deux marges :
commander 90 puis 180 degres et mesurer. Un ecart **constant** designe l'inertie,
donc `ROT_MARGE_ARRET_DEG` ; un ecart **proportionnel** designe `SENSIBILITE_GYRO`.

Le trim de derive est regle **pour 1 m**. Sur 2 m le vehicule decrit une legere
courbe en S : un transitoire de demarrage s'ajoute a la derive etablie, et une
constante ne peut annuler leur somme qu'a une seule distance.

### Pilotage manuel

| Constante | Valeur | Role |
|---|---|---|
| `VITESSE_JOYSTICK` | 80 | Sensibilite unique, appliquee lineairement aux deux axes |

La consigne envoyee vaut `(y -/+ x) * VITESSE_JOYSTICK` par cote. L'interface
bornant la poignee a un cercle, la diagonale a 45 degres produit une consigne de
113 que `Moteurs_DefinirVitesse` ecrete a 100 : un seul cote etant tronque, le
rayon de virage se resserre legerement a pleins gaz.

### Detection d'obstacles et contournement

| Constante | Valeur | Role |
|---|---|---|
| `OBSTACLE_SEUIL_CM` | 25 * | En deca, obstacle signale. **A garder nettement au-dessus de `LIDAR_DISTANCE_MIN`** |
| `OBSTACLE_PERIODE_MS` | 50 | Reevaluation de la distance frontale |
| `OBSTACLE_ECART_SONDAGE_DEG` | 45 | Ecart des secteurs lateraux sondes |
| `OBSTACLE_SENS_SERVO` | -1 | -1 si gauche et droite sont inverses mecaniquement |
| `OBSTACLE_STABILISATION_MS` | 40 | Attente apres l'arrivee du servo, avant la mesure |
| `EVITEMENT_ANGLE_DEG` | 45 | Rotations d'ecartement et de remise parallele |
| `EVITEMENT_ANGLE_RETOUR_DEG` | 20 * | Rotation finale vers la ligne. Plus faible pour la garder dans le champ de la camera |
| `EVITEMENT_DISTANCE_M` | 0.45 | Diagonale d'ecartement, donne 32 cm d'ecart lateral |
| `EVITEMENT_LONGEMENT_M` | 0.20 | Segment parallele a la ligne, doit depasser l'obstacle |
| `EVITEMENT_PAUSE_MS` | 400 | Immobilisation entre deux etapes |
| `EVITEMENT_ESSAIS_MAX` | 3 | Rotations d'ecartement avant d'abandonner |
| `ULTRASON_RECUL_CM` | 0 | Recul du capteur par rapport au pare-choc, **a mesurer sur la coque finale** |
| `LIDAR_RECUL_CM` | 0 | Idem |

### Capteurs

| Constante | Valeur | Role |
|---|---|---|
| `ULTRASON_DISTANCE_MAX` | 100 | Portee utile (cm), valeur plafond = voie degagee |
| `ULTRASON_PERIODE_MS` | 100 | Rafraichissement, environ 10 Hz |
| `ULTRASON_TIMEOUT_US` | 6000 | 100 cm aller-retour font environ 5.8 ms |
| `ULTRASON_TIMEOUT_PRESENCE_US` | 60000 | Au-dela de 38 ms : echo emis sans obstacle |
| `LIDAR_FORCE_MIN` | 100 | En dessous, signal juge trop faible |
| `LIDAR_DISTANCE_MIN` | 20 | Zone morte declaree par la fiche technique |
| `LIDAR_DISTANCE_MAX` | 800 | Portee fiable (cm) |
| `LIDAR_PERIODE_MS` | 100 | Rafraichissement, environ 10 Hz |
| `SERVO_ANGLE_MIN` / `_MAX` / `_CENTRE` | 0 / 180 / 90 | 90 degres pointe droit devant |
| `SERVO_MS_PAR_DEGRE` | 5 | Environ 200 deg/s, le SG90 plafonnant vers 300 |
| `SERVO_PULSE_MIN_US` / `_MAX_US` | 600 / 2400 | Largeurs d'impulsion a 0 et 180 degres |
| `SERVO_MAINTIEN_MS` | 400 | Duree du train d'impulsions apres l'arrivee a la cible |
| `DLPF_20HZ` | 4 | Bande passante du gyroscope ramenee a 20 Hz |
| `IMU_CAL_ECHANTILLONS` | 200 | Calibration longue au demarrage |
| `IMU_CAL_ECHANTILLONS_RAPIDE` | 40 | Rezero avant chaque mouvement asservi |
| `IMU_CAL_DISPERSION_MAX` | 3.0 | Au-dela, en deg/s, le vehicule bougeait : calibration rejetee |
| `FEU_AGE_MAX_MS` | 3000 | Peremption d'une detection de feu |
| `LUMINOSITE_BANDEAU` | 255 | Gain global des bandeaux, voir le bilan de courant |
| `RESEAU_SSID` | `"VTA"` | Doit correspondre a la commande `nmcli` du deploiement |
| `RESEAU_MOT_DE_PASSE` | `"admin123"` | Idem |
| `RESEAU_IP` | `"10.42.0.1"` | Defaut de NetworkManager en mode partage |
| `NOM_COURT` | `"VTA"` | Titre affiche a l'ecran, le nom complet n'y tient pas |

Le train d'impulsions du servo **s'arrete** une fois la position tenue. A l'arret,
chaque impulsion regeneree porte le jitter de l'attente active et redemande donc
une position legerement differente : le servo vibrerait sur place sans se deplacer.
Sans signal, le reducteur maintient l'angle, la charge du support etant faible.

## Constantes de reglage du MPU

| Constante | Fichier | Valeur | Role |
|---|---|---|---|
| `LARGEUR_CAPTURE` x `HAUTEUR_CAPTURE` | `serveur_web.py` | 640 x 360 | Resolution de capture, 16/9 comme le capteur |
| `CADENCE_CAPTURE` | `serveur_web.py` | 30 | Images par seconde |
| `PERIODE_LIGNES` | `boucle_vision.py` | 0.10 | Cadence du suivi de ligne (10 Hz) |
| `PERIODE_INFERENCE` | `boucle_vision.py` | 0.30 | Cadence des inferences YOLO |
| `REBOND_ACTIVATION` | `boucle_vision.py` | 2 | Detections consecutives avant de signaler un feu |
| `REBOND_DESACTIVATION` | `boucle_vision.py` | 4 | Cycles sans feu avant de le declarer absent |
| `TAILLE_INFERENCE` | `vision.py` | 256 | Cote de la fenetre. **Doit correspondre a l'`imgsz` d'export du modele** |
| `SEUIL_CONFIANCE` | `vision.py` | 0.40 | Un feu lointain sort avec une confiance plus basse |
| `SEUIL_NMS` | `vision.py` | 0.45 | Recouvrement au-dela duquel deux boites fusionnent |
| `THREADS_ORT` | `vision.py` | 2 | Threads laisses a ONNX Runtime |
| `ROI_HAUT_LIGNES` | `vision.py` | 0.60 | Haut du ROI de ligne, donc les 40 % du bas |
| `PIXELS_MIN_LIGNE` | `vision.py` | 800 | En deca, aucune ligne n'est declaree |

`CHEMIN_MODELE`, `TAILLE_INFERENCE` et `THREADS_ORT` sont surchargeables par
variable d'environnement, ce qui permet d'essayer un autre export du modele sans
toucher au code.

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
- Fichiers 3D du chassis et du PCB (`hardware/`)
