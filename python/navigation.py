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

VITESSE_AVANT  = 0.4
KP_LATERAL     = 0.001
CORRECTION_MAX = 0.3


# ======================== Etat interne ========================

_actif = False
_derniere_correction = None
_dernier_detecte = None


# ======================== Activation ========================

def activer():
    """Active le suivi de ligne autonome."""
    global _actif, _derniere_correction, _dernier_detecte
    _actif = True
    _derniere_correction = None
    _dernier_detecte = None
    print("[nav] Mode autonome active")


def desactiver():
    """Desactive la navigation et arrete les moteurs."""
    global _actif, _derniere_correction, _dernier_detecte
    _actif = False
    _derniere_correction = None
    _dernier_detecte = None
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
    global _derniere_correction, _dernier_detecte

    if not _actif:
        return

    if not detecte:
        if _dernier_detecte is not False:
            comm_bridge.envoyer_joystick(0, 0)
            _dernier_detecte = False
            _derniere_correction = None
        return

    correction = -KP_LATERAL * ecart
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))
    correction = round(correction, 2)

    if correction != _derniere_correction:
        comm_bridge.envoyer_joystick(correction, VITESSE_AVANT)
        _derniere_correction = correction
        _dernier_detecte = True