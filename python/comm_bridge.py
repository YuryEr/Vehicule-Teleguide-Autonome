"""
Communication Bridge — TankETS (MPU -> MCU)
=============================================
Envoie les resultats de vision au STM32 via Bridge RPC.

Contrat RPC (doit correspondre aux noms dans sketch/comm_bridge.cpp) :
    on_feu(bool present, int couleur, int confiance)
    on_lignes(bool detecte, int ecart)
"""

from arduino.app_utils import Bridge


def notifier_feu(present, couleur, confiance):
    """Envoie l'etat du feu au MCU.

    present   — True si un feu est detecte
    couleur   — COULEUR_* (voir vision.py)
    confiance — pourcentage 0-100
    """
    try:
        Bridge.call("on_feu", present, int(couleur), int(confiance))
    except Exception as e:
        print(f"[bridge] on_feu echec : {e}")


def notifier_lignes(detecte, ecart):
    """Envoie l'etat des lignes au MCU.

    detecte — True si des lignes sont detectees
    ecart   — deviation laterale en pixels (signe)
    """
    try:
        Bridge.call("on_lignes", detecte, int(ecart))
    except Exception as e:
        print(f"[bridge] on_lignes echec : {e}")