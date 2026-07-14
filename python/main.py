"""
TankETS — Point d'entree (MPU / Qualcomm Linux)
"""

import socket
import subprocess
import sys
from pathlib import Path


def _connexion_internet(hote="8.8.8.8", port=53, timeout=2):
    """Retourne True si une connexion Internet sortante est disponible.

    En mode point d'acces (AP), le vehicule diffuse son propre Wi-Fi et
    n'a pas d'acces Internet : on saute alors l'installation pip pour ne
    pas bloquer le demarrage. Les dependances doivent avoir ete installees
    une premiere fois en etant connecte (voir README).
    """
    try:
        socket.setdefaulttimeout(timeout)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((hote, port))
        return True
    except OSError:
        return False


requirements = Path(__file__).parent / "requirements.txt"
if requirements.exists():
    if _connexion_internet():
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install",
             "-r", str(requirements), "--quiet"],
            stdout=subprocess.DEVNULL
        )
    else:
        print("[main] Hors ligne (mode point d'acces ?) — "
              "installation pip ignoree.")

from serveur_web import demarrer_serveur

demarrer_serveur()
