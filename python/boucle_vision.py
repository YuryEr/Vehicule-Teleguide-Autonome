"""
Pipeline de vision — TankETS
=============================
Orchestre les detections a cadences independantes avec anti-rebond.
Decouple : recoit des callbacks, ne connait pas Bridge.
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
    """Machine a etats pour la detection routiere.

    sur_changement_feu(present, couleur, confiance) — appele quand
        l'etat du feu change (apparition, disparition, changement couleur).
    sur_lignes_detectees(detecte, ecart) — appele a chaque cycle lignes.
    """

    def __init__(self, sur_changement_feu, sur_lignes_detectees):
        self._sur_feu    = sur_changement_feu
        self._sur_lignes = sur_lignes_detectees

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
        """Traite une frame. Appeler a chaque iteration de la boucle."""
        now = time.time()

        self._traiter_lignes(frame, now)
        self._traiter_feux(frame, now)
        self._traiter_telemetrie(now)

    def _traiter_lignes(self, frame, now):
        if now - self._t_lignes < PERIODE_LIGNES:
            return
        self._t_lignes = now

        detecte, ecart = vision.detecter_lignes(frame)
        self._sur_lignes(detecte, ecart)

    def _traiter_feux(self, frame, now):
        if now - self._t_inference < PERIODE_INFERENCE:
            return
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

        self._evaluer_etat_feu(couleur, confiance, now)

    def _evaluer_etat_feu(self, couleur, confiance, now):
        if not self._feu_present and self._hits >= REBOND_ACTIVATION:
            self._feu_present      = True
            self._derniere_couleur = couleur
            self._sur_feu(True, couleur, int(confiance * 100))
            self._dernier_envoi = now
            print(f"[feu] DETECTE "
                  f"{vision.NOMS_COULEURS[couleur]} "
                  f"({confiance:.2f})")

        elif (self._feu_present
              and self._misses >= REBOND_DESACTIVATION):
            self._feu_present      = False
            self._derniere_couleur = vision.COULEUR_AUCUNE
            self._sur_feu(False, vision.COULEUR_AUCUNE, 0)
            self._dernier_envoi = now
            print("[feu] perdu")

        elif (self._feu_present
              and self._hits > 0
              and (couleur != self._derniere_couleur
                   or now - self._dernier_envoi >= RAFRAICHISSEMENT_S)):
            self._derniere_couleur = couleur
            self._sur_feu(True, couleur, int(confiance * 100))
            self._dernier_envoi = now

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