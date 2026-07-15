"""
Navigation autonome — TankETS (MPU / Qualcomm Linux)
=====================================================
Suivi de ligne proportionnel.

Principe :
    La vision fournit l'ecart lateral (pixels) entre le centre de
    la ligne noire et le centre de l'image. On ralentit la roue du
    cote vers lequel tourner pour recentrer la ligne. Les deux roues
    avancent toujours (valeurs positives).

Reference : Proportional line follower — approche standard robotique.
"""

import comm_bridge


# ======================== Parametres de suivi ========================

VITESSE_BASE   = 15      # vitesse des deux roues en ligne droite (0-100)
KP_LATERAL     = 0.04    # gain : pixels d'ecart -> unites de vitesse
CORRECTION_MAX = 10      # correction maximale (garde les deux roues en avant)
SENS           = -1       # mettre -1 si le robot braque du MAUVAIS cote
MISS_MAX       = 8       # cycles sans ligne avant l'arret


# ======================== Etat interne ========================

_actif = False
_misses = 0


# ======================== Activation ========================

def activer():
    global _actif, _misses
    _actif = True
    _misses = 0
    print("[nav] Mode autonome active")


def desactiver():
    global _actif
    _actif = False
    comm_bridge.envoyer_roues(0, 0)
    print("[nav] Mode autonome desactive")


def est_actif():
    return _actif


# ======================== Traitement vision ========================

def traiter_lignes(detecte, ecart):
    """Recentre la ligne au milieu de l'image (controle proportionnel)."""
    global _misses

    if not _actif:
        return

    if not detecte:
        _misses += 1
        if _misses >= MISS_MAX:
            comm_bridge.envoyer_roues(0, 0)
        else:
            comm_bridge.envoyer_roues(VITESSE_BASE, VITESSE_BASE)  # tout droit
        return

    _misses = 0

    correction = SENS * KP_LATERAL * ecart
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))

    gauche = int(max(0, min(100, VITESSE_BASE + correction)))
    droite = int(max(0, min(100, VITESSE_BASE - correction)))

    comm_bridge.envoyer_roues(gauche, droite)