import subprocess, sys
subprocess.check_call([sys.executable, "-m", "pip", "install", "-q", "--break-system-packages", "flask", "flask-socketio", "eventlet"])

# =============================================================
# Serveur principal - Véhicule téléguidé autonome
# PFE ÉTS - Été 2026
#
# Dépendances : flask, flask-socketio, eventlet
# Interface   : http://<IP_ARDUINO>:7000
# Flux vidéo  : http://<IP_ARDUINO>:8090/video (service systemd séparé)
# =============================================================

from flask import Flask, render_template
from flask_socketio import SocketIO

# --- Configuration ---
PORT_WEB = 7000

app      = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet")

# --- État global du véhicule ---
etat = {
    "mode":          "manuel",
    "camera_active": True,
    "direction":     {"x": 0.0, "y": 0.0},
    "mode_led1":     0,
    "mode_led2":     0,
    "led1_on":       False,
    "led2_on":       False,
}

noms_modes = ["éteint", "gyrophare", "clignotant", "phares"]

# =============================================================
# MODE BLOCS — asservissement délégué au MCU (STM32)
# =============================================================
# Les déplacements précis (encodeurs + gyro) sont exécutés en boucle
# fermée sur le MCU. Python démarre le mouvement via Bridge puis attend
# sa fin en interrogeant "mouvement_actif". Voir sketch/sketch.ino.
# Les commandes Bridge reçoivent TOUJOURS une valeur positive (le Bridge
# ne transmet pas correctement les nombres négatifs) : le sens est géré
# par des fonctions dédiées côté MCU.

# --- État d'exécution des séquences Blockly ---
sequence_en_cours = False
sequence_stop     = False

# =============================================================
# UTILITAIRES BRIDGE
# =============================================================

def _bridge_call(nom, *args):
    """Appel Bridge sécurisé (silencieux hors App Lab)."""
    try:
        from arduino.app_utils import Bridge
        Bridge.call(nom, *args)
        return True
    except Exception as e:
        print(f"[Bridge] {nom} erreur : {e}")
        return False

def _bridge_call_valeur(nom, *args):
    """Comme _bridge_call mais retourne la valeur renvoyée par le MCU
    (ou None si le Bridge est indisponible)."""
    try:
        from arduino.app_utils import Bridge
        return Bridge.call(nom, *args)
    except Exception as e:
        print(f"[Bridge] {nom} erreur : {e}")
        return None

def _moteurs(x, y):
    """Envoie une consigne de déplacement au STM32."""
    _bridge_call("joy_x", float(x))
    _bridge_call("joy_y", float(y))

# =============================================================
# EXÉCUTION DES SÉQUENCES BLOCKLY
# =============================================================

def _sleep_interruptible(duree):
    """
    Attend `duree` secondes en vérifiant le flag stop toutes les 50 ms.
    Retourne False si la séquence a été interrompue.
    """
    global sequence_stop
    ecoule = 0.0
    while ecoule < duree:
        if sequence_stop:
            return False
        socketio.sleep(0.05)
        ecoule += 0.05
    return True

def _mouvement_bloquant(nom_commande, valeur):
    """
    Démarre un déplacement asservi sur le MCU (non bloquant côté MCU),
    puis attend sa fin en interrogeant l'état toutes les 50 ms.
    Interruptible via le flag `sequence_stop`.

    RETOUR : False si le mouvement a été interrompu, True sinon.
    """
    _bridge_call(nom_commande, abs(float(valeur)))
    while True:
        if sequence_stop:
            _bridge_call("arreter_mouvement")
            return False
        if not _bridge_call_valeur("mouvement_actif"):
            return True
        socketio.sleep(0.05)

def _description_commande(commande):
    """Texte affiché dans l'interface pendant l'exécution."""
    cmd = commande.get("cmd", "?")
    v   = commande.get("valeur", "")
    descriptions = {
        "avancer":        f"Avancer de {v} m",
        "reculer":        f"Reculer de {v} m",
        "tourner_gauche": f"Tourner à gauche de {v}°",
        "tourner_droite": f"Tourner à droite de {v}°",
        "attendre":       f"Attendre {v} s",
        "led":            f"Allumer bandeau {commande.get('bandeau')} "
                          f"({noms_modes[commande.get('mode', 1)]})",
        "led_off":        f"Éteindre bandeau {commande.get('bandeau')}",
    }
    return descriptions.get(cmd, cmd)

def executer_sequence(sequence):
    """
    Exécute une séquence de commandes issue de Blockly.
    Tourne dans une tâche de fond flask-socketio (interruptible).
    """
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
            print(f"[Séquence] {i+1}/{total} : {_description_commande(commande)}")

            if cmd == "avancer":
                if not _mouvement_bloquant("avancer_metres", commande.get("valeur", 0)):
                    interrompue = True

            elif cmd == "reculer":
                if not _mouvement_bloquant("reculer_metres", commande.get("valeur", 0)):
                    interrompue = True

            elif cmd == "tourner_gauche":
                if not _mouvement_bloquant("tourner_gauche_deg", commande.get("valeur", 90)):
                    interrompue = True

            elif cmd == "tourner_droite":
                if not _mouvement_bloquant("tourner_droite_deg", commande.get("valeur", 90)):
                    interrompue = True

            elif cmd == "attendre":
                if not _sleep_interruptible(float(commande.get("valeur", 1))):
                    interrompue = True

            elif cmd == "led":
                bandeau = int(commande.get("bandeau", 1))
                mode    = int(commande.get("mode", 1))
                etat[f"mode_led{bandeau}"] = mode
                etat[f"led{bandeau}_on"]   = True
                _bridge_call(f"mode_led{bandeau}", mode)
                socketio.emit(f"etat_led{bandeau}", {"active": True, "mode": mode})

            elif cmd == "led_off":
                bandeau = int(commande.get("bandeau", 1))
                etat[f"led{bandeau}_on"] = False
                _bridge_call(f"mode_led{bandeau}", 0)
                socketio.emit(f"etat_led{bandeau}",
                              {"active": False, "mode": etat[f"mode_led{bandeau}"]})

            if interrompue:
                break

    finally:
        # Sécurité : toujours arrêter les moteurs en fin de séquence
        _bridge_call("arreter_mouvement")
        _moteurs(0, 0)
        sequence_en_cours = False
        socketio.emit("sequence_status", {
            "etat":  "arretee" if interrompue else "terminee",
            "index": 0,
            "total": total,
            "description": "Séquence interrompue" if interrompue else "Séquence terminée",
        })
        print(f"[Séquence] {'Interrompue' if interrompue else 'Terminée'}")

# =============================================================
# ROUTES WEB
# =============================================================

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/scan_i2c')
def route_scan_i2c():
    try:
        from arduino.app_utils import Bridge
        resultat = Bridge.call("scan_i2c")
        print(f"=== Résultat scan I2C : {resultat} ===")
        return f"Résultat : {resultat}"
    except Exception as e:
        return str(e)

# =============================================================
# CALLBACKS SOCKET.IO — Pilotage manuel
# =============================================================

@socketio.on('connect')
def on_connect():
    print("Client connecté")

@socketio.on('disconnect')
def on_disconnect():
    print("Client déconnecté")

@socketio.on('joystick')
def on_joystick(data):
    """Pilotage manuel — ignoré pendant l'exécution d'une séquence."""
    if sequence_en_cours:
        return
    x = float(data.get("x", 0))
    y = float(data.get("y", 0))
    etat["direction"]["x"] = x
    etat["direction"]["y"] = y
    _moteurs(x, y)

@socketio.on('changer_mode')
def on_changer_mode(data):
    nouveau_mode = data.get("mode", "manuel")
    etat["mode"] = nouveau_mode
    print(f"Mode véhicule → {nouveau_mode}")
    socketio.emit("mode_actuel", {"mode": nouveau_mode})

@socketio.on('toggle_camera')
def on_toggle_camera(data):
    etat["camera_active"] = data.get("active", True)
    print(f"Caméra → {'ON' if etat['camera_active'] else 'OFF'}")
    socketio.emit("etat_camera", {"active": etat["camera_active"]})

@socketio.on('onoff_led1')
def on_onoff_led1(data):
    actif = data.get("active", False)
    etat["led1_on"] = actif
    if actif:
        mode = etat["mode_led1"] if etat["mode_led1"] > 0 else 1
        etat["mode_led1"] = mode
        _bridge_call("mode_led1", mode)
        print(f"LED 1 → ON ({noms_modes[mode]})")
    else:
        _bridge_call("mode_led1", 0)
        print("LED 1 → OFF")
    socketio.emit("etat_led1", {"active": actif, "mode": etat["mode_led1"]})

@socketio.on('onoff_led2')
def on_onoff_led2(data):
    actif = data.get("active", False)
    etat["led2_on"] = actif
    if actif:
        mode = etat["mode_led2"] if etat["mode_led2"] > 0 else 1
        etat["mode_led2"] = mode
        _bridge_call("mode_led2", mode)
        print(f"LED 2 → ON ({noms_modes[mode]})")
    else:
        _bridge_call("mode_led2", 0)
        print("LED 2 → OFF")
    socketio.emit("etat_led2", {"active": actif, "mode": etat["mode_led2"]})

@socketio.on('mode_led1')
def on_mode_led1(data):
    mode = data.get("mode", 1)
    etat["mode_led1"] = mode
    if etat["led1_on"]:
        _bridge_call("mode_led1", mode)
    print(f"LED 1 mode → {noms_modes[mode]}")
    socketio.emit("etat_led1", {"active": etat["led1_on"], "mode": mode})

@socketio.on('mode_led2')
def on_mode_led2(data):
    mode = data.get("mode", 1)
    etat["mode_led2"] = mode
    if etat["led2_on"]:
        _bridge_call("mode_led2", mode)
    print(f"LED 2 mode → {noms_modes[mode]}")
    socketio.emit("etat_led2", {"active": etat["led2_on"], "mode": mode})

# =============================================================
# CALLBACKS SOCKET.IO — Programmation par blocs
# =============================================================

@socketio.on('executer_sequence')
def on_executer_sequence(data):
    """
    Reçoit la séquence générée par Blockly.
    data = {"sequence": [{"cmd": "avancer", "valeur": 1.0}, ...]}
    """
    global sequence_en_cours
    if sequence_en_cours:
        socketio.emit("sequence_status", {
            "etat": "erreur",
            "description": "Une séquence est déjà en cours",
        })
        return

    sequence = data.get("sequence", [])
    if not sequence:
        socketio.emit("sequence_status", {
            "etat": "erreur",
            "description": "Séquence vide — ajoutez des blocs",
        })
        return

    print(f"[Séquence] Reçue : {len(sequence)} commandes")
    socketio.start_background_task(executer_sequence, sequence)

@socketio.on('stop_sequence')
def on_stop_sequence():
    """Interrompt la séquence en cours et stoppe les moteurs."""
    global sequence_stop
    sequence_stop = True
    _bridge_call("arreter_mouvement")
    _moteurs(0, 0)
    print("[Séquence] STOP demandé")

# =============================================================
# LANCEMENT
# =============================================================

if __name__ == '__main__':
    print(f"Serveur démarré sur http://0.0.0.0:{PORT_WEB}")
    print("Flux vidéo sur http://<IP_ARDUINO>:8090/video (service systemd)")
    socketio.run(app, host='0.0.0.0', port=PORT_WEB, debug=False)