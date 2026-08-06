"""
Navigation autonome — TankETS (MPU / Qualcomm Linux)
=====================================================
Suivi de ligne par correcteur proportionnel-derive.

Principe :
    La vision fournit l'ecart lateral (pixels) entre le centre de
    la ligne noire et le centre de l'image. On ralentit la roue du
    cote vers lequel tourner pour recentrer la ligne. Les deux roues
    avancent toujours (valeurs positives).

    Le terme proportionnel repond a l'ecart, le terme derive s'oppose
    a ses variations. Sans ce dernier, le retard de la boucle — vision
    a 10 Hz, plus la latence du Bridge et des moteurs — fait arriver la
    correction alors que l'ecart a deja change de signe : le vehicule
    depasse d'un bord a l'autre et finit par perdre la ligne.

Reference : correcteur proportionnel-derive, approche standard en
    robotique mobile. Astrom, K. J., & Murray, R. M. (2008). Feedback
    Systems: An Introduction for Scientists and Engineers. Princeton
    University Press.
"""

import comm_bridge


# ======================== Parametres de suivi ========================

VITESSE_BASE   = 15      # vitesse des deux roues en ligne droite (0-100)
KP_LATERAL     = 0.015    # gain proportionnel : pixels d'ecart -> vitesse
KD_LATERAL     = 0.035    # gain derive : amortit le depassement
ZONE_MORTE_PX  = 35      # ecart en deca duquel la ligne est jugee centree
CORRECTION_MAX = 8       # correction maximale (garde les deux roues en avant)
SENS           = -1      # oriente la correction ; -1 valide sur le montage
                         # actuel, a reprendre si la camera est reorientee
MISS_MAX       = 8       # cycles sans ligne avant l'arret


# ======================== Etat interne ========================

_actif  = False
_misses = 0
_ecart_precedent = 0


# ======================== Activation ========================

def activer():
    global _actif, _misses, _ecart_precedent
    _actif = True
    _misses = 0
    _ecart_precedent = 0
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
    """Recentre la ligne au milieu de l'image (correcteur proportionnel-derive)."""
    global _misses, _ecart_precedent

    if not _actif:
        return

    if not detecte:
        _misses += 1
        if _misses >= MISS_MAX:
            comm_bridge.envoyer_roues(0, 0)
        else:
            comm_bridge.envoyer_roues(VITESSE_BASE, VITESSE_BASE)  # tout droit
        _ecart_precedent = 0
        return

    _misses = 0

    # Zone morte : sous ce seuil la ligne est consideree centree. Sans elle,
    # le bruit du centroide entretient une correction permanente et le
    # vehicule serpente meme en ligne droite.
    ecart_utile = ecart if abs(ecart) >= ZONE_MORTE_PX else 0

    # Terme derive : il s'oppose aux variations rapides de l'ecart. C'est lui
    # qui amortit le depassement, le terme proportionnel ne pouvant que
    # reagir a un ecart deja constitue.
    variation = ecart_utile - _ecart_precedent
    _ecart_precedent = ecart_utile

    correction = SENS * (KP_LATERAL * ecart_utile + KD_LATERAL * variation)
    correction = max(-CORRECTION_MAX, min(CORRECTION_MAX, correction))

    gauche = int(max(0, min(100, VITESSE_BASE + correction)))
    droite = int(max(0, min(100, VITESSE_BASE - correction)))

    comm_bridge.envoyer_roues(gauche, droite)
