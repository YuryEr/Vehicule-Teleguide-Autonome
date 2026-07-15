"""
Navigation autonome — TankETS (MPU / Qualcomm Linux)
=====================================================
Suivi de ligne proportionnel.

Principe :
    La vision fournit l'ecart lateral (en pixels) entre le centre
    de la ligne noire et le centre de l'image. Une correction
    proportionnelle ralentit la roue du cote vers lequel tourner,
    ce qui recentre la ligne. Les deux roues avancent toujours
    (valeurs positives) — le Bridge ne transmet pas les negatifs.

Reference : Proportional line follower — approche standard robotique.
"""

import comm_bridge


# ======================== Parametres de suivi ========================

VITESSE_BASE   = 30      # vitesse des deux roues en ligne droite (0-100)
KP_LATERAL     = 0.10    # gain : pixels d'ecart -> unites de vitesse
CORRECTION_MAX = 25      # correction maximale appliquee
MISS_MAX       = 5       # cycles sans ligne avant l'arret


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
    """Recentre la ligne au milieu de l'image (controle proportionnel).

    ecart > 0 : ligne a droite -> ralentir la roue droite (tourner a droite)
    ecart < 0 : ligne a gauche -> ralentir la roue gauche (tourner a gauche)
    """
    global _misses

    if not _actif:
        return

    if not detecte:
        _misses += 1
        if _misses >= MISS_MAX:
            comm_bridge.envoyer_roues(0, 0)
        return

    _misses = 0

    correction = KP_LATERAL * ecart
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))

    gauche = int(max(0, min(100, VITESSE_BASE + correction)))
    droite = int(max(0, min(100, VITESSE_BASE - correction)))

    comm_bridge.envoyer_roues(gauche, droite)