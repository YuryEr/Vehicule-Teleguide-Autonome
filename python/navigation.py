"""
Navigation autonome — TankETS (MPU / Qualcomm Linux)
=====================================================
Suivi de ligne et reactions aux detections visuelles.

Lorsque le mode autonome est actif, ce module recoit les
donnees de la pipeline de vision et envoie les commandes
de direction au MCU via le Bridge RPC.

Algorithme :
    Correction proportionnelle sur l'ecart lateral des lignes.
    ecart > 0 = decale a droite -> correction vers la gauche.
"""

import comm_bridge


# ======================== Parametres de suivi ========================

VITESSE_AVANT  = 0.3
KP_LATERAL     = 0.002
CORRECTION_MAX = 0.5


# ======================== Etat interne ========================

_actif = False


# ======================== Activation ========================

def activer():
    """Active le suivi de ligne autonome."""
    global _actif
    _actif = True
    print("[nav] Mode autonome active")


def desactiver():
    """Desactive la navigation et arrete les moteurs."""
    global _actif
    _actif = False
    comm_bridge.envoyer_joystick(0, 0)
    print("[nav] Mode autonome desactive")


def est_actif():
    return _actif


# ======================== Traitement vision ========================

def traiter_lignes(detecte, ecart):
    """Reagit aux lignes detectees par la vision.

    detecte — True si des lignes sont detectees
    ecart   — deviation laterale en pixels (positif = droite)
    """
    if not _actif:
        return

    if not detecte:
        comm_bridge.envoyer_joystick(0, 0)
        return

    correction = -KP_LATERAL * ecart
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))
    comm_bridge.envoyer_joystick(correction, VITESSE_AVANT)