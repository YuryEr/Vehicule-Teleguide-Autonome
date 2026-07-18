"""
TankETS — Point d'entree (MPU / Qualcomm Linux)
"""

import subprocess
import sys
from pathlib import Path

requirements = Path(__file__).parent / "requirements.txt"
if requirements.exists():
    subprocess.check_call(
        [sys.executable, "-m", "pip", "install",
         "-r", str(requirements), "--quiet"],
        stdout=subprocess.DEVNULL
    )

from serveur_web import demarrer_serveur

demarrer_serveur()