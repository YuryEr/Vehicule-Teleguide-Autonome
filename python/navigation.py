"""
Navigation autonome — TankETS (MPU / Qualcomm Linux)
=====================================================
Suivi de ligne proportionnel.

Algorithme :
    A chaque cycle de vision, calcule l'ecart lateral de la
    ligne et applique une correction proportionnelle aux moteurs.
    Reference : PID Line Follower — standard robotics approach.
"""

import comm_bridge


# ======================== Parametres de suivi ========================

VITESSE_AVANT  = 0.4
KP_LATERAL     = 0.003
CORRECTION_MAX = 0.3


# ======================== Etat interne ========================

_actif = False


# ======================== Activation ========================

def activer():
    global _actif
    _actif = True
    print("[nav] Mode autonome active")


def desactiver():
    global _actif
    _actif = False
    comm_bridge.envoyer_joystick(0, 0)
    print("[nav] Mode autonome desactive")


def est_actif():
    return _actif


# ======================== Traitement vision ========================

def traiter_lignes(detecte, ecart):
    if not _actif:
        return

    if not detecte:
        comm_bridge.envoyer_joystick(0, 0)
        return

    correction = -KP_LATERAL * ecart
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))
    comm_bridge.envoyer_joystick(correction, VITESSE_AVANT)