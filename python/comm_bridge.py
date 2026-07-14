"""
Communication Bridge — TankETS (MPU <-> MCU)
==============================================
Couche unique de communication entre le MPU (Python/Linux)
et le MCU (STM32/Zephyr) via Arduino Bridge RPC.

Contrat RPC (Python -> MCU) :
    Pilotage manuel :
        joy_x(float), joy_y(float)
    Deplacements asservis (valeurs toujours positives) :
        avancer_metres(float), reculer_metres(float)
        tourner_gauche_deg(float), tourner_droite_deg(float)
        arreter_mouvement()
        mouvement_actif() -> int (0/1)
    LEDs :
        mode_led1(int), mode_led2(int)  [0=off, 1=gyro, 2=cligno, 3=phares]
    Vision :
        on_feu(bool, int, int), on_lignes(bool, int)

NOTE : le Bridge ne transmet pas correctement les nombres negatifs.
Toutes les valeurs envoyees au MCU sont positives — le sens est
gere par des fonctions dediees cote MCU.
"""


# ======================== Appels generiques ========================

def _appeler(nom, *args):
    """Appel Bridge securise. Retourne True si succes, False sinon."""
    try:
        from arduino.app_utils import Bridge
        Bridge.call(nom, *args)
        return True
    except Exception as e:
        print(f"[bridge] {nom} echec : {e}")
        return False


def _appeler_avec_retour(nom, *args):
    """Appel Bridge qui retourne la valeur du MCU, ou None si echec."""
    try:
        from arduino.app_utils import Bridge
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
    """Retourne True si un deplacement asservi est en cours sur le MCU."""
    resultat = _appeler_avec_retour("mouvement_actif")
    return bool(resultat) if resultat is not None else False


# ======================== LEDs ========================

def definir_mode_led(bandeau, mode):
    """Change le mode d'un bandeau LED.

    bandeau — numero du bandeau (1 ou 2)
    mode    — 0=eteint, 1=gyrophare, 2=clignotant, 3=phares
    """
    _appeler(f"mode_led{int(bandeau)}", int(mode))


# ======================== Capteurs de distance ========================

def lire_sonar_cm():
    """Distance frontale mesuree par le sonar HC-SR04.

    Retour :
        int  — distance en cm (SONAR_DISTANCE_MAX ~= voie degagee)
        None — Bridge indisponible
    """
    return _appeler_avec_retour("lire_sonar_cm")


def lire_lidar_cm():
    """Distance droit devant mesuree par le TF-Luna (LiDAR).

    Retour :
        int  — distance en cm ; -1 si la lecture MCU est invalide
        None — Bridge indisponible
    """
    return _appeler_avec_retour("lire_lidar_cm")


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