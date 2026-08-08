"""
Pipeline de vision — TankETS
=============================
Detections routieres avec anti-rebond.

Les deux traitements sont exposes separement et n'ont aucune cadence propre :
c'est l'appelant qui les appelle a son rythme. Les partager dans un meme appel
asservirait le suivi de ligne, qui coute quelques millisecondes et pilote le
vehicule, a l'inference des feux, qui coute plusieurs centaines de
millisecondes. La cadence de correction dependrait alors du cout de la vision.

Cette classe s'execute dans un thread du pool, ou toucher aux sockets eventlet
ou au Bridge corromprait leur etat. Elle ne diffuse donc rien elle-meme : elle
retourne ce qu'il y a a emettre, et c'est l'appelant, dans sa greenlet, qui le
dispatche.
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

        self._fps_t0 = time.time()
        self._fps_n  = 0

    @property
    def feu_present(self):
        return self._feu_present

    @property
    def derniere_couleur(self):
        return self._derniere_couleur

    def traiter_lignes(self, frame):
        """Detecte la ligne et retourne (detecte, ecart).

        Cadence pilotee par l'appelant : elle ne doit dependre que de la
        boucle de suivi, jamais du cout de l'inference.
        """
        self._compter_cadence()
        return vision.detecter_lignes(frame)

    def traiter_feux(self, frame):
        """Detecte les feux.

        Retourne (present, couleur, confiance_pourcent) quand l'etat du feu
        change, None sinon. Cadence pilotee par l'appelant.
        """
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

        return self._evaluer_etat_feu(couleur, confiance, time.time())

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

    def _compter_cadence(self):
        """Trace la cadence reelle du suivi de ligne.

        C'est le nombre de corrections que le vehicule applique par seconde :
        la grandeur qui conditionne le reglage du correcteur.
        """
        self._fps_n += 1
        now = time.time()
        if now - self._fps_t0 < 5.0:
            return
        cadence = self._fps_n / (now - self._fps_t0)
        print(
            f"[perf] lignes {cadence:.1f} Hz | "
            f"feu={self._feu_present} "
            f"couleur={vision.NOMS_COULEURS[self._derniere_couleur]}"
        )
        self._fps_t0 = now
        self._fps_n  = 0
