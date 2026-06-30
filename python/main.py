# =============================================================
# Serveur principal - Véhicule téléguidé autonome
# PFE ÉTS - Été 2026
#
# Dépendances : flask, flask-socketio, eventlet
# Lancement via App Lab OU directement : python3 main.py
# Interface   : http://<IP_ARDUINO>:7000
# Flux vidéo  : http://<IP_ARDUINO>:8090/video (service systemd séparé)
# =============================================================

import threading
import time
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
# CALLBACKS SOCKET.IO
# =============================================================

@socketio.on('connect')
def on_connect():
    print("Client connecté")

@socketio.on('disconnect')
def on_disconnect():
    print("Client déconnecté")

@socketio.on('joystick')
def on_joystick(data):
    x = float(data.get("x", 0))
    y = float(data.get("y", 0))
    etat["direction"]["x"] = x
    etat["direction"]["y"] = y
    print(f"Joystick reçu → x={x:.2f}, y={y:.2f}")  # ← ajoute ça
    try:
        from arduino.app_utils import Bridge
        Bridge.call("joy_x", x)
        Bridge.call("joy_y", y)
        print(f"Bridge OK → joy_x={x:.2f}, joy_y={y:.2f}")  # ← et ça
    except Exception as e:
        print(f"[Bridge moteurs] {e}")

@socketio.on('changer_mode')
def on_changer_mode(data):
    """Change le mode de conduite : manuel / semi-autonome / autonome"""
    nouveau_mode = data.get("mode", "manuel")
    etat["mode"] = nouveau_mode
    print(f"Mode véhicule → {nouveau_mode}")
    socketio.emit("mode_actuel", {"mode": nouveau_mode})

@socketio.on('toggle_camera')
def on_toggle_camera(data):
    """Allume ou éteint l'affichage de la caméra dans l'interface."""
    etat["camera_active"] = data.get("active", True)
    statut = "ON" if etat["camera_active"] else "OFF"
    print(f"Caméra → {statut}")
    socketio.emit("etat_camera", {"active": etat["camera_active"]})

@socketio.on('onoff_led1')
def on_onoff_led1(data):
    """Allume ou éteint le bandeau LED 1 (pin D3)."""
    actif = data.get("active", False)
    etat["led1_on"] = actif
    if actif:
        mode = etat["mode_led1"] if etat["mode_led1"] > 0 else 1
        etat["mode_led1"] = mode
        _envoyer_mode_led(1, mode)
        print(f"LED 1 → ON ({noms_modes[mode]})")
    else:
        _envoyer_mode_led(1, 0)
        print("LED 1 → OFF")
    socketio.emit("etat_led1", {"active": actif, "mode": etat["mode_led1"]})

@socketio.on('onoff_led2')
def on_onoff_led2(data):
    """Allume ou éteint le bandeau LED 2 (pin D6)."""
    actif = data.get("active", False)
    etat["led2_on"] = actif
    if actif:
        mode = etat["mode_led2"] if etat["mode_led2"] > 0 else 1
        etat["mode_led2"] = mode
        _envoyer_mode_led(2, mode)
        print(f"LED 2 → ON ({noms_modes[mode]})")
    else:
        _envoyer_mode_led(2, 0)
        print("LED 2 → OFF")
    socketio.emit("etat_led2", {"active": actif, "mode": etat["mode_led2"]})

@socketio.on('mode_led1')
def on_mode_led1(data):
    """Change le mode du bandeau LED 1."""
    mode = data.get("mode", 1)
    etat["mode_led1"] = mode
    if etat["led1_on"]:
        _envoyer_mode_led(1, mode)
    print(f"LED 1 mode → {noms_modes[mode]}")
    socketio.emit("etat_led1", {"active": etat["led1_on"], "mode": mode})

@socketio.on('mode_led2')
def on_mode_led2(data):
    """Change le mode du bandeau LED 2."""
    mode = data.get("mode", 1)
    etat["mode_led2"] = mode
    if etat["led2_on"]:
        _envoyer_mode_led(2, mode)
    print(f"LED 2 mode → {noms_modes[mode]}")
    socketio.emit("etat_led2", {"active": etat["led2_on"], "mode": mode})

# =============================================================
# BRIDGE STM32
# =============================================================

def _envoyer_mode_led(num, mode):
    """Envoie le mode LED au STM32 via Bridge."""
    try:
        from arduino.app_utils import Bridge
        Bridge.call(f"mode_led{num}", mode)
    except ImportError:
        print(f"[Bridge] mode_led{num} = {mode} (Bridge non disponible hors App Lab)")
    except Exception as e:
        print(f"[Bridge] mode_led{num} erreur : {e}")

# =============================================================
# LANCEMENT
# =============================================================

if __name__ == '__main__':
    print(f"Serveur démarré sur http://0.0.0.0:{PORT_WEB}")
    print(f"Flux vidéo sur http://<IP_ARDUINO>:8090/video (service systemd)")
    socketio.run(app, host='0.0.0.0', port=PORT_WEB, debug=False)