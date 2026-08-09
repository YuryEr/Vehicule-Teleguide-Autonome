"""
Serveur web, VTA (MPU / Qualcomm Linux)
==============================================
Serveur Flask + SocketIO pour l'interface de controle.

Responsabilites :
    - Servir l'interface web statique (assets/)
    - Pilotage manuel via joystick WebSocket
    - Execution des sequences Blockly (mode programme)
    - Controle des LEDs (barre haute + feux avant/arriere)
    - Gestion des modes de conduite

Sources :
    - Flask : https://flask.palletsprojects.com/
    - Flask-SocketIO : https://flask-socketio.readthedocs.io/
    - Eventlet : https://eventlet.net/
"""

import os
import time
import cv2
import eventlet
import eventlet.tpool
from flask import Flask, Response
from flask_socketio import SocketIO

import comm_bridge
import navigation
import vision as module_vision
from boucle_vision import BoucleVision, PERIODE_LIGNES, PERIODE_INFERENCE


# ======================== Configuration ========================

PORT_WEB = 7000

# Capture. Sans consigne explicite, le pilote impose son mode par defaut,
# souvent un recadrage en 4/3 qui ampute le champ de vision et se lit comme un
# zoom. Le capteur plafonne a 1280x720, mais n'y tient que 10 images par
# seconde en YUYV : autant que la cadence du suivi de ligne, donc sans aucune
# marge. Le mode 640x360 offre le meme champ, etant lui aussi en 16/9, pour
# trente images par seconde et quatre fois moins de pixels a traiter.
LARGEUR_CAPTURE = 640
HAUTEUR_CAPTURE = 360
CADENCE_CAPTURE = 30

# Cadence maximale du flux vers le navigateur, alignee sur la camera : encoder
# plus vite ne produirait que des doublons.
PERIODE_FLUX = 1.0 / CADENCE_CAPTURE

CHEMIN_ASSETS = os.path.join(
    os.path.dirname(os.path.dirname(__file__)), 'assets'
)

app = Flask(__name__,
            static_folder=CHEMIN_ASSETS,
            static_url_path='')
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet",
                    ping_timeout=60, ping_interval=25)

# ======================== Etat global ========================

NOMS_MODES_BANDEAUX = ["eteint", "position", "gyrophare"]

etat = {
    "mode":          "manuel",
    "camera_active": True,
    "direction":     {"x": 0.0, "y": 0.0},
    "mode_bandeaux": 0,
    "phares_on":     False,
}

sequence_en_cours = False
sequence_stop     = False


# ======================== Route ========================

@app.route('/')
def index():
    return app.send_static_file('index.html')

# ======================== Flux video + vision ========================

derniere_frame = None

camera = None
index_camera = None

def obtenir_camera():
    global camera, index_camera
    if camera is not None and camera.isOpened():
        return camera

    for i in range(10):
        cam = cv2.VideoCapture(i)
        if cam.isOpened():
            cam.set(cv2.CAP_PROP_FRAME_WIDTH,  LARGEUR_CAPTURE)
            cam.set(cv2.CAP_PROP_FRAME_HEIGHT, HAUTEUR_CAPTURE)
            cam.set(cv2.CAP_PROP_FPS,          CADENCE_CAPTURE)

            ret, _ = cam.read()
            if ret:
                # Reglages relus : une camera substitue silencieusement le mode
                # le plus proche quand celui demande n'existe pas, et la cadence
                # depend de la resolution retenue.
                largeur = int(cam.get(cv2.CAP_PROP_FRAME_WIDTH))
                hauteur = int(cam.get(cv2.CAP_PROP_FRAME_HEIGHT))
                cadence = int(cam.get(cv2.CAP_PROP_FPS))
                print(f"[camera] Trouvee sur index {i} "
                      f"en {largeur}x{hauteur} a {cadence} fps")
                camera = cam
                index_camera = i
                return camera
            cam.release()

    print("[camera] Aucune camera detectee")
    return None

def _capture_utile():
    """Indique s'il vaut la peine de capturer et de traiter des images.

    Une sequence Blockly monopolise le Bridge, et une camera coupee n'a aucun
    consommateur : dans les deux cas, capturer reviendrait a decoder et encoder
    des images que personne ne regarde.
    """
    return (not sequence_en_cours) and etat["camera_active"]


def _tache_capture():
    global derniere_frame, camera
    while True:
        if not _capture_utile():
            socketio.sleep(0.1)
            continue
        cam = obtenir_camera()
        if cam is None:
            socketio.sleep(2)
            continue
        ret, frame = eventlet.tpool.execute(cam.read)
        if not ret:
            camera = None
            socketio.sleep(0.5)
            continue
        derniere_frame = frame
        socketio.sleep(0)


def generer_flux():
    derniere_envoyee = None
    while True:
        if not _capture_utile():
            socketio.sleep(0.2)
            continue

        frame = derniere_frame
        # Ne re-encoder que sur image nouvelle, et jamais plus vite que la
        # camera ne produit. Sans ces deux gardes, la boucle re-encode la meme
        # image aussi vite que le processeur le permet, ce qui sature le lien
        # WiFi et monopolise les threads du pool au detriment de la vision.
        if frame is None or frame is derniere_envoyee:
            socketio.sleep(PERIODE_FLUX)
            continue
        derniere_envoyee = frame

        _, jpeg = eventlet.tpool.execute(
            cv2.imencode, '.jpg', frame,
            [cv2.IMWRITE_JPEG_QUALITY, 60]
        )
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n'
               + jpeg.tobytes() + b'\r\n')
        socketio.sleep(PERIODE_FLUX)


@app.route('/video')
def flux_video():
    return Response(generer_flux(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


def _sur_changement_feu(present, couleur, confiance):
    """Diffuse un changement d'etat du feu. Appele depuis la greenlet."""
    nom = module_vision.NOMS_COULEURS.get(couleur, "AUCUNE")
    socketio.emit('etat_feu', {
        'present': present,
        'couleur': nom,
        'confiance': confiance,
    })
    if not sequence_en_cours:
        eventlet.tpool.execute(comm_bridge.notifier_feu,
                               present, couleur, confiance)


def _sur_lignes_detectees(detecte, ecart):
    """Diffuse un cycle de detection de ligne. Appele depuis la greenlet.

    L'emission se fait ici meme, la ou vit la boucle d'evenements. La commande
    moteur part en revanche dans le pool : elle bloque le temps d'un aller-retour
    RPC, ce qui figerait la boucle. Le verrou de comm_bridge serialise les
    appelants concurrents.
    """
    socketio.emit('etat_lignes', {
        'detecte': detecte,
        'ecart': ecart,
    })
    if not sequence_en_cours:
        eventlet.tpool.execute(navigation.traiter_lignes, detecte, ecart)


_boucle_vision = None


def _tache_feux():
    """Charge le modele, puis fait tourner l'inference a sa propre cadence."""
    global _boucle_vision
    try:
        module_vision.initialiser_modele()
    except Exception as e:
        print(f"[vision] Modele non charge : {e}")
        return

    _boucle_vision = BoucleVision()
    print("[vision] Pipeline de detection active")

    while True:
        frame = derniere_frame
        if frame is None or not _capture_utile():
            socketio.sleep(PERIODE_INFERENCE)
            continue

        try:
            # Seul le calcul part dans le pool. La notification revient ici,
            # dans la greenlet, avant d'etre diffusee : emettre sur une socket
            # eventlet ou appeler le Bridge depuis un thread natif corrompt
            # leur etat, ce qui deconnecte le navigateur et brouille les
            # trames RPC.
            feu = eventlet.tpool.execute(_boucle_vision.traiter_feux, frame)
        except Exception as e:
            print(f"[vision] feu ignore : {e}")
            feu = None

        if feu is not None:
            _sur_changement_feu(*feu)

        socketio.sleep(PERIODE_INFERENCE)


def _tache_lignes():
    """Detection de ligne, a cadence propre.

    Elle pilote le suivi et ne doit donc jamais attendre l'inference des feux,
    cent fois plus lente. Partager une meme boucle asservirait la cadence de
    correction du vehicule au cout de la vision : ajouter une fenetre
    d'inference suffirait a degrader sa trajectoire.
    """
    while _boucle_vision is None:
        socketio.sleep(0.2)

    prochaine = time.time()
    while True:
        frame = derniere_frame
        if frame is None or not _capture_utile():
            socketio.sleep(PERIODE_LIGNES)
            prochaine = time.time()
            continue

        try:
            lignes = eventlet.tpool.execute(_boucle_vision.traiter_lignes,
                                            frame)
        except Exception as e:
            print(f"[vision] ligne ignoree : {e}")
            lignes = None

        if lignes is not None:
            _sur_lignes_detectees(*lignes)

        # Attente jusqu'a la prochaine echeance plutot que d'une duree fixe :
        # dormir apres le travail ajouterait sa duree a la periode, et la
        # cadence dependrait alors du temps de traitement. Si un cycle deborde,
        # on repart de maintenant sans chercher a rattraper le retard.
        prochaine += PERIODE_LIGNES
        reste = prochaine - time.time()
        if reste > 0:
            socketio.sleep(reste)
        else:
            prochaine = time.time()

# ======================== Socket : Connexion ========================

@socketio.on('connect')
def on_connect():
    print("[web] Client connecte")


@socketio.on('disconnect')
def on_disconnect():
    print("[web] Client deconnecte")


# ======================== Socket : Pilotage manuel ========================

@socketio.on('joystick')
def on_joystick(data):
    if sequence_en_cours or navigation.est_actif():
        return
    x = float(data.get("x", 0))
    y = float(data.get("y", 0))
    etat["direction"]["x"] = x
    etat["direction"]["y"] = y
    comm_bridge.envoyer_joystick(x, y)


@socketio.on('changer_mode')
def on_changer_mode(data):
    nouveau_mode = data.get("mode", "manuel")
    etat["mode"] = nouveau_mode
    print(f"[web] Mode vehicule -> {nouveau_mode}")
    socketio.emit("mode_actuel", {"mode": nouveau_mode})

    # Le veto du MCU ne s'applique qu'en autonome : le mode doit lui etre
    # transmis, il ne le deduit pas des commandes recues.
    if nouveau_mode == "autonome":
        comm_bridge.definir_mode(comm_bridge.MODE_AUTONOME)
        navigation.activer()
    else:
        comm_bridge.definir_mode(comm_bridge.MODE_MANUEL)
        navigation.desactiver()


@socketio.on('toggle_camera')
def on_toggle_camera(data):
    etat["camera_active"] = data.get("active", True)
    print(f"[web] Camera -> {'ON' if etat['camera_active'] else 'OFF'}")
    socketio.emit("etat_camera", {"active": etat["camera_active"]})


# ======================== Socket : LEDs ========================

@socketio.on('mode_bandeaux')
def on_mode_bandeaux(data):
    """Change le mode de la barre haute des deux bandeaux.

    mode : 0=eteint, 1=feux de position, 2=gyrophare
    """
    mode = int(data.get("mode", 0))
    etat["mode_bandeaux"] = mode
    comm_bridge.definir_mode_bandeaux(mode)
    print(f"[web] Bandeaux -> {NOMS_MODES_BANDEAUX[mode]}")
    socketio.emit("etat_bandeaux", {"mode": mode})


@socketio.on('toggle_phares')
def on_toggle_phares(data):
    """Allume ou eteint les feux avant et arriere."""
    actif = data.get("active", False)
    etat["phares_on"] = actif
    comm_bridge.definir_phares(actif)
    print(f"[web] Phares -> {'ON' if actif else 'OFF'}")
    socketio.emit("etat_phares", {"active": actif})


# ======================== Socket : Sequences Blockly ========================

@socketio.on('executer_sequence')
def on_executer_sequence(data):
    global sequence_en_cours
    if sequence_en_cours:
        socketio.emit("sequence_status", {
            "etat": "erreur",
            "description": "Une sequence est deja en cours",
        })
        return

    sequence = data.get("sequence", [])
    if not sequence:
        socketio.emit("sequence_status", {
            "etat": "erreur",
            "description": "Sequence vide, ajoutez des blocs",
        })
        return

    print(f"[blocs] Sequence recue : {len(sequence)} commandes")
    socketio.start_background_task(_executer_sequence, sequence)


@socketio.on('stop_sequence')
def on_stop_sequence():
    global sequence_stop
    sequence_stop = True
    comm_bridge.arreter_mouvement()
    comm_bridge.envoyer_joystick(0, 0)
    print("[blocs] STOP demande")


def _executer_sequence(sequence):
    global sequence_en_cours, sequence_stop
    sequence_en_cours = True
    sequence_stop     = False
    total = len(sequence)
    interrompue = False

    try:
        for i, commande in enumerate(sequence):
            if sequence_stop:
                interrompue = True
                break

            cmd = commande.get("cmd")
            socketio.emit("sequence_status", {
                "etat":        "en_cours",
                "index":       i + 1,
                "total":       total,
                "description": _description_commande(commande),
            })
            print(f"[blocs] {i+1}/{total} : "
                  f"{_description_commande(commande)}")

            if cmd == "avancer":
                if not _mouvement_bloquant(
                        comm_bridge.avancer_metres,
                        commande.get("valeur", 0)):
                    interrompue = True

            elif cmd == "reculer":
                if not _mouvement_bloquant(
                        comm_bridge.reculer_metres,
                        commande.get("valeur", 0)):
                    interrompue = True

            elif cmd == "tourner_gauche":
                if not _mouvement_bloquant(
                        comm_bridge.tourner_gauche_deg,
                        commande.get("valeur", 90)):
                    interrompue = True

            elif cmd == "tourner_droite":
                if not _mouvement_bloquant(
                        comm_bridge.tourner_droite_deg,
                        commande.get("valeur", 90)):
                    interrompue = True

            elif cmd == "attendre":
                if not _sleep_interruptible(
                        float(commande.get("valeur", 1))):
                    interrompue = True

            elif cmd == "bandeaux":
                mode = int(commande.get("mode", 0))
                etat["mode_bandeaux"] = mode
                comm_bridge.definir_mode_bandeaux(mode)
                socketio.emit("etat_bandeaux", {"mode": mode})

            elif cmd == "phares":
                actif = bool(commande.get("actif", True))
                etat["phares_on"] = actif
                comm_bridge.definir_phares(actif)
                socketio.emit("etat_phares", {"active": actif})

            if interrompue:
                break

    finally:
        comm_bridge.arreter_mouvement()
        comm_bridge.envoyer_joystick(0, 0)
        sequence_en_cours = False
        socketio.emit("sequence_status", {
            "etat": "arretee" if interrompue else "terminee",
            "index": 0,
            "total": total,
            "description": ("Sequence interrompue"
                            if interrompue else "Sequence terminee"),
        })
        print(f"[blocs] {'Interrompue' if interrompue else 'Terminee'}")


def _mouvement_bloquant(fonction_bridge, valeur):
    eventlet.tpool.execute(fonction_bridge, abs(float(valeur)))

    # 1) Confirmer le demarrage
    demarre = False
    debut = time.time()
    while time.time() - debut < 1.5:
        if sequence_stop:
            eventlet.tpool.execute(comm_bridge.arreter_mouvement)
            return False
        if eventlet.tpool.execute(comm_bridge.mouvement_actif) == 1:
            demarre = True
            break
        socketio.sleep(0.05)

    # Mouvement jamais demarre = Bridge sature/corrompu : on interrompt la
    # sequence au lieu d'enchainer des timeouts de 10 s sur une liaison morte.
    if not demarre:
        eventlet.tpool.execute(comm_bridge.arreter_mouvement)
        return False

    # 2) Attendre la fin
    debut = time.time()
    while True:
        if sequence_stop:
            eventlet.tpool.execute(comm_bridge.arreter_mouvement)
            return False
        if time.time() - debut > 10:
            eventlet.tpool.execute(comm_bridge.arreter_mouvement)
            socketio.sleep(0.3)
            return True
        if eventlet.tpool.execute(comm_bridge.mouvement_actif) == 0:
            socketio.sleep(0.3)
            return True
        socketio.sleep(0.2)


def _sleep_interruptible(duree):
    ecoule = 0.0
    while ecoule < duree:
        if sequence_stop:
            return False
        socketio.sleep(0.05)
        ecoule += 0.05
    return True


def _description_commande(commande):
    cmd = commande.get("cmd", "?")
    v   = commande.get("valeur", "")
    descriptions = {
        "avancer":        f"Avancer de {v} m",
        "reculer":        f"Reculer de {v} m",
        "tourner_gauche": f"Tourner a gauche de {v} deg",
        "tourner_droite": f"Tourner a droite de {v} deg",
        "attendre":       f"Attendre {v} s",
        "bandeaux":       f"Bandeaux : "
                          f"{NOMS_MODES_BANDEAUX[commande.get('mode', 0)]}",
        "phares":         ("Allumer les phares"
                           if commande.get("actif", True)
                           else "Eteindre les phares"),
    }
    return descriptions.get(cmd, cmd)


# ======================== Lancement ========================

def demarrer_serveur():
    """Lance le serveur web. Bloquant."""
    print(f"[web] Serveur demarre sur http://0.0.0.0:{PORT_WEB}")
    socketio.start_background_task(_tache_capture)
    socketio.start_background_task(_tache_feux)
    socketio.start_background_task(_tache_lignes)
    socketio.run(app, host='0.0.0.0', port=PORT_WEB, debug=False)
