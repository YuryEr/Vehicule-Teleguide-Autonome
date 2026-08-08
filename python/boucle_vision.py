"""
Pipeline de vision — TankETS
=============================
Orchestre les detections a cadences independantes avec anti-rebond.

Cette classe s'execute dans un thread du pool, ou toucher aux sockets
eventlet ou au Bridge corromprait leur etat. Elle ne diffuse donc rien
elle-meme : `traiter()` retourne les notifications a emettre, et c'est
l'appelant, dans sa greenlet, qui les dispatche.
"""

import time
import vision


# ======================== Cadences et seuils ========================

PERIODE_INFERENCE        = 0.30
PERIODE_LIGNES           = 0.10
REBOND_ACTIVATION        = 2
REBOND_DESACTIVATION     = 4
RAFRAICHISSEMENT_S       = 0.5


# ======================== Classe principale ========================

class BoucleVision:
    """Machine a etats pour la detection routiere."""

    def __init__(self):
        self._hits             = 0
        self._misses           = 0
        self._feu_present      = False
        self._derniere_couleur = vision.COULEUR_AUCUNE
        self._dernier_envoi    = 0.0

        self._t_inference = 0.0
        self._t_lignes    = 0.0

        self._fps_t0 = time.time()
        self._fps_n  = 0

    @property
    def feu_present(self):
        return self._feu_present

    @property
    def derniere_couleur(self):
        return self._derniere_couleur

    def traiter(self, frame):
        """Traite une frame et retourne les notifications a diffuser.

        Retourne un dictionnaire pouvant contenir :
            "lignes" — (detecte, ecart)
            "feu"    — (present, couleur, confiance_pourcent)
        Une cle absente signifie qu'il n'y a rien a diffuser pour ce sujet.
        """
        now = time.time()
        notifications = {}

        lignes = self._traiter_lignes(frame, now)
        if lignes is not None:
            notifications["lignes"] = lignes

        feu = self._traiter_feux(frame, now)
        if feu is not None:
            notifications["feu"] = feu

        self._traiter_telemetrie(now)
        return notifications

    def _traiter_lignes(self, frame, now):
        if now - self._t_lignes < PERIODE_LIGNES:
            return None
        self._t_lignes = now

        return vision.detecter_lignes(frame)

    def _traiter_feux(self, frame, now):
        if now - self._t_inference < PERIODE_INFERENCE:
            return None
        self._t_inference = now

        detections = vision.detecter_feux(frame)
        nombre    = len(detections)
        confiance = 0.0
        couleur   = vision.COULEUR_AUCUNE

        if nombre:
            x1, y1, x2, y2, conf = max(
                detections, key=lambda d: d[4]
            )
            confiance = conf
            couleur = vision.classifier_couleur_feu(
                frame[y1:y2, x1:x2]
            )

        if nombre > 0:
            self._hits  += 1
            self._misses = 0
        else:
            self._misses += 1
            self._hits    = 0

        return self._evaluer_etat_feu(couleur, confiance, now)

    def _evaluer_etat_feu(self, couleur, confiance, now):
        """Retourne la notification a diffuser, ou None si rien n'a change."""
        if not self._feu_present and self._hits >= REBOND_ACTIVATION:
            self._feu_present      = True
            self._derniere_couleur = couleur
            self._dernier_envoi    = now
            print(f"[feu] DETECTE "
                  f"{vision.NOMS_COULEURS[couleur]} "
                  f"({confiance:.2f})")
            return (True, couleur, int(confiance * 100))

        if (self._feu_present
                and self._misses >= REBOND_DESACTIVATION):
            self._feu_present      = False
            self._derniere_couleur = vision.COULEUR_AUCUNE
            self._dernier_envoi    = now
            print("[feu] perdu")
            return (False, vision.COULEUR_AUCUNE, 0)

        if (self._feu_present
                and self._hits > 0
                and (couleur != self._derniere_couleur
                     or now - self._dernier_envoi >= RAFRAICHISSEMENT_S)):
            self._derniere_couleur = couleur
            self._dernier_envoi    = now
            return (True, couleur, int(confiance * 100))

        return None

    def _traiter_telemetrie(self, now):
        self._fps_n += 1
        if now - self._fps_t0 < 5.0:
            return
        fps = self._fps_n / (now - self._fps_t0)
        print(
            f"[perf] {fps:.1f} FPS | "
            f"feu={self._feu_present} "
            f"couleur={vision.NOMS_COULEURS[self._derniere_couleur]}"
        )
        self._fps_t0 = now
        self._fps_n  = 0
