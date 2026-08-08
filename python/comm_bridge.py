"""
Communication Bridge — TankETS (MPU <-> MCU)
==============================================
Couche unique de communication entre le MPU (Python/Linux)
et le MCU (STM32/Zephyr) via Arduino Bridge RPC.

Contrat RPC (Python -> MCU) :
    Mode de conduite :
        definir_mode(int)  [0=manuel, 1=autonome]
        veto_actif() -> int (0/1)
    Pilotage manuel :
        joy_x(float), joy_y(float)
    Deplacements asservis (valeurs toujours positives) :
        avancer_metres(float), reculer_metres(float)
        tourner_gauche_deg(float), tourner_droite_deg(float)
        arreter_mouvement()
        mouvement_actif() -> 1, 0, ou None si la lecture echoue
    LEDs :
        mode_bandeaux(int)  [0=eteint, 1=position, 2=gyrophare]
        mode_phares(int)    [0=eteint, 1=allume]
    Vision :
        on_feu(bool, int, int), on_lignes(bool, int)

NOTE : le Bridge ne transmet pas correctement les nombres negatifs.
Toutes les valeurs envoyees au MCU sont positives — le sens est
gere par des fonctions dediees cote MCU.
"""

import threading


# ======================== Appels generiques ========================

# Le transport RPC n'admet qu'un echange a la fois. Or les appelants ne sont
# pas tous dans la meme greenlet : la vision, le sequenceur de blocs et les
# gestionnaires d'evenements peuvent solliciter le Bridge simultanement, et
# deux trames qui se chevauchent arrivent corrompues au MCU.
_verrou = threading.Lock()


def _appeler(nom, *args):
    """Appel Bridge securise. Retourne True si succes, False sinon."""
    try:
        from arduino.app_utils import Bridge
        with _verrou:
            Bridge.call(nom, *args)
        return True
    except Exception as e:
        print(f"[bridge] {nom} echec : {e}")
        return False


def _appeler_avec_retour(nom, *args):
    """Appel Bridge qui retourne la valeur du MCU, ou None si echec."""
    try:
        from arduino.app_utils import Bridge
        with _verrou:
            return Bridge.call(nom, *args)
    except Exception as e:
        print(f"[bridge] {nom} echec : {e}")
        return None


# ======================== Pilotage manuel ========================

def envoyer_joystick(x, y):
    """Envoie la position du joystick au MCU.

    x — axe horizontal (-1.0 a 1.0, gauche/droite)
    y — axe vertical (-1.0 a 1.0, arriere/avant)
    """
    _appeler("joy_x", float(x))
    _appeler("joy_y", float(y))

def envoyer_roues(gauche, droite):
    """Envoie une vitesse positive a chaque cote (suivi de ligne).

    gauche — vitesse cote gauche (0 a 100, positif)
    droite — vitesse cote droit (0 a 100, positif)
    """
    _appeler("roues", int(gauche), int(droite))

# ======================== Mode de conduite ========================

MODE_MANUEL   = 0
MODE_AUTONOME = 1


def definir_mode(mode):
    """Informe le MCU du regime de conduite.

    Le veto de securite n'est applique qu'en mode autonome : en manuel le
    pilote garde le controle complet du vehicule.

    mode — MODE_MANUEL ou MODE_AUTONOME
    """
    _appeler("definir_mode", int(mode))


def lire_cause_arret():
    """Raison pour laquelle le MCU refuse de faire avancer le vehicule.

    Retourne : "obstacle", "feu", ou None si la voie est libre ou si la
               lecture Bridge echoue.
    """
    valeur = _appeler_avec_retour("cause_arret")
    if valeur == 1:
        return "obstacle"
    if valeur == 2:
        return "feu"
    return None


def veto_actif():
    """Retourne True si le MCU refuse actuellement de faire avancer le
    vehicule a cause d'un obstacle."""
    return bool(_appeler_avec_retour("veto_actif"))


# ======================== Deplacements asservis ========================

def avancer_metres(distance):
    """Demarre un deplacement vers l'avant (asservi par encodeurs).

    distance — distance en metres (toujours positive)
    """
    _appeler("avancer_metres", abs(float(distance)))


def reculer_metres(distance):
    """Demarre un deplacement vers l'arriere (asservi par encodeurs).

    distance — distance en metres (toujours positive)
    """
    _appeler("reculer_metres", abs(float(distance)))


def tourner_gauche_deg(angle):
    """Demarre une rotation a gauche (asservie par gyroscope).

    angle — angle en degres (toujours positif)
    """
    _appeler("tourner_gauche_deg", abs(float(angle)))


def tourner_droite_deg(angle):
    """Demarre une rotation a droite (asservie par gyroscope).

    angle — angle en degres (toujours positif)
    """
    _appeler("tourner_droite_deg", abs(float(angle)))


def arreter_mouvement():
    """Arret d'urgence — stoppe les moteurs et libere la machine a etats."""
    _appeler("arreter_mouvement")


def mouvement_actif():
    """Retourne 1 (en cours), 0 (termine) ou None (lecture Bridge ratee).

    La distinction entre 0 et None est indispensable au sequencage des blocs :
    confondre une lecture ratee avec un mouvement termine fait enchainer la
    commande suivante par-dessus celle en cours, qui est alors ecrasee.
    """
    resultat = _appeler_avec_retour("mouvement_actif")
    if resultat is None:
        return None
    return 1 if resultat else 0


# ======================== LEDs ========================

def definir_mode_bandeaux(mode):
    """Change le mode de la barre haute des deux bandeaux.

    mode — 0=eteint, 1=feux de position, 2=gyrophare
    """
    _appeler("mode_bandeaux", int(mode))


def definir_phares(actif):
    """Allume ou eteint les feux : blanc a l'avant, rouge a l'arriere.

    actif — True pour allumer, False pour eteindre
    """
    _appeler("mode_phares", 1 if actif else 0)


# ======================== Vision ========================

def notifier_feu(present, couleur, confiance):
    """Envoie l'etat du feu de signalisation au MCU.

    present   — True si un feu est detecte
    couleur   — COULEUR_* (voir vision.py)
    confiance — pourcentage 0-100
    """
    _appeler("on_feu", present, int(couleur), int(confiance))


def notifier_lignes(detecte, ecart):
    """Envoie l'etat des lignes de route au MCU.

    detecte — True si des lignes sont detectees
    ecart   — deviation laterale en pixels (signe)
    """
    _appeler("on_lignes", detecte, int(ecart))

# ======================== Capteurs de distance ========================

def lire_ultrason_cm():
    """Distance frontale mesuree par le capteur ultrason HC-SR04.

    Retourne : int — distance en cm (ULTRASON_DISTANCE_MAX = voie degagee),
               None si la lecture Bridge echoue.
    """
    return _appeler_avec_retour("lire_ultrason_cm")

def lire_lidar_cm():
    """Distance frontale mesuree par le LiDAR TF-Luna.

    Retourne : int — distance en cm, -1 si lecture invalide,
               None si la lecture Bridge echoue.
    """
    return _appeler_avec_retour("lire_lidar_cm")

# ======================== ServoMoteur ========================

def definir_angle_servo(angle):
    """Oriente le servo de balayage du LiDAR.

    angle — degres (0 a 180, 90 = droit devant)
    """
    _appeler("servo_angle", int(max(0, min(180, angle))))

# ======================== Detection d'obstacles ========================

def lire_obstacle_frontal_cm():
    """Distance de l'obstacle le plus proche devant le vehicule.

    Fusion de l'ultrason et du LiDAR cote MCU : la plus petite des mesures
    valides. La valeur plafond signifie voie degagee.

    Retourne : int — distance en cm, None si la lecture Bridge echoue.
    """
    return _appeler_avec_retour("obstacle_frontal_cm")


def obstacle_detecte():
    """Retourne True si un obstacle est signale sous le seuil du MCU."""
    return bool(_appeler_avec_retour("obstacle_detecte"))


def lancer_sondage():
    """Demarre un sondage des trois secteurs. Dure environ une seconde.

    Le vehicule doit etre a l'arret : en mouvement, les trois mesures
    decriraient trois positions differentes.
    """
    _appeler("lancer_sondage")


def lire_cote_degage():
    """Cote le plus degage releve par le dernier sondage.

    Retourne : "gauche", "droite", ou None si le sondage est encore en
               cours ou si la lecture Bridge echoue.
    """
    valeur = _appeler_avec_retour("cote_degage")
    if valeur == 1:
        return "gauche"
    if valeur == 2:
        return "droite"
    return None