"""
Serveur web — TankETS (MPU / Qualcomm Linux)
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
from boucle_vision import BoucleVision


# ======================== Configuration ========================

PORT_WEB = 7000

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
            ret, _ = cam.read()
            if ret:
                print(f"[camera] Trouvee sur index {i}")
                camera = cam
                index_camera = i
                return camera
            cam.release()

    print("[camera] Aucune camera detectee")
    return None

def _tache_capture():
    global derniere_frame, camera
    while True:
        # Pendant une sequence, on libere le CPU/tpool pour le Bridge :
        # la video se fige quelques secondes, sans consequence.
        if sequence_en_cours:
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
    while True:
        if sequence_en_cours:
            socketio.sleep(0.2)
            continue
        frame = derniere_frame
        if frame is None:
            socketio.sleep(0.1)
            continue
        _, jpeg = eventlet.tpool.execute(
            cv2.imencode, '.jpg', frame,
            [cv2.IMWRITE_JPEG_QUALITY, 60]
        )
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n'
               + jpeg.tobytes() + b'\r\n')
        socketio.sleep(0)


@app.route('/video')
def flux_video():
    return Response(generer_flux(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


def _sur_changement_feu(present, couleur, confiance):
    nom = module_vision.NOMS_COULEURS.get(couleur, "AUCUNE")
    socketio.emit('etat_feu', {
        'present': present,
        'couleur': nom,
        'confiance': confiance,
    })
    # Pendant une sequence Blockly, on ne notifie PAS le MCU : deux sources
    # RPC simultanees (vision + sequence) corrompent la trame du Bridge.
    if not sequence_en_cours:
        comm_bridge.notifier_feu(present, couleur, confiance)


def _sur_lignes_detectees(detecte, ecart):
    socketio.emit('etat_lignes', {
        'detecte': detecte,
        'ecart': ecart,
    })
    if not sequence_en_cours:
        comm_bridge.notifier_lignes(detecte, ecart)
        navigation.traiter_lignes(detecte, ecart)


def _tache_vision():
    try:
        module_vision.initialiser_modele()
    except Exception as e:
        print(f"[vision] Modele non charge : {e}")
        return

    bv = BoucleVision(_sur_changement_feu, _sur_lignes_detectees)
    print("[vision] Pipeline de detection active")

    while True:
        # Pendant une sequence Blockly, on suspend TOTALEMENT la vision :
        # sinon YOLO/OpenCV saturent le CPU et affament le thread de lecture
        # du Bridge -> buffer serie qui deborde -> trame RPC corrompue.
        frame = derniere_frame
        if frame is not None and not sequence_en_cours:
            try:
                eventlet.tpool.execute(bv.traiter, frame)
            except Exception as e:
                print(f"[vision] erreur frame ignoree : {e}")
        socketio.sleep(0.05)

# ======================== Socket — Connexion ========================

@socketio.on('connect')
def on_connect():
    print("[web] Client connecte")


@socketio.on('disconnect')
def on_disconnect():
    print("[web] Client deconnecte")


# ======================== Socket — Pilotage manuel ========================

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


# ======================== Socket — LEDs ========================

@socketio.on('mode_bandeaux')
def on_mode_bandeaux(data):
    """Change le mode de la barre haute des deux bandeaux.

    mode — 0=eteint, 1=feux de position, 2=gyrophare
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


# ======================== Socket — Sequences Blockly ========================

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
            "description": "Sequence vide — ajoutez des blocs",
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
    socketio.start_background_task(_tache_vision)
    socketio.run(app, host='0.0.0.0', port=PORT_WEB, debug=False)