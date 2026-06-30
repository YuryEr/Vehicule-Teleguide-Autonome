"""
TankETS — Point d'entree (MPU / Qualcomm Linux)
"""

import time

from arduino.app_utils import App

import vision
import comm_bridge
from boucle_vision import BoucleVision


vision.initialiser_modele()
vision.initialiser_camera()

pipeline = BoucleVision(
    sur_changement_feu=comm_bridge.notifier_feu,
    sur_lignes_detectees=comm_bridge.notifier_lignes
)


def loop():
    frame = vision.derniere_image()
    if frame is None:
        time.sleep(0.02)
        return
    pipeline.traiter(frame)


App.run(user_loop=loop)