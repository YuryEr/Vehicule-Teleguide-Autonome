"""
Detection du reseau, VTA (MPU / Qualcomm Linux)
==================================================
Determine l'adresse IPv4 par laquelle un client joindra le serveur web.
Cette adresse alimente le code QR affiche a l'ecran du MCU.

Plusieurs methodes sont tentees dans l'ordre : selon le mode reseau du
conteneur, toutes ne repondent pas, et certaines rapportent l'adresse
interne du conteneur plutot que celle de l'interface sans fil.
"""

import os
import socket
import subprocess


# Surcharge manuelle, prioritaire sur toute detection. Sert quand aucune
# methode ne rapporte l'adresse attendue.
IP_FORCEE = os.getenv("VTA_IP", "")


def _sortie(commande):
    """Execute une commande systeme et retourne sa sortie, vide si echec."""
    try:
        return subprocess.run(commande, capture_output=True,
                              text=True, timeout=2).stdout.strip()
    except Exception:
        return ""


def _ip_par_interface():
    """Adresse portee par l'interface sans fil, lue avec l'outil ip."""
    for commande in (["ip", "-4", "-o", "addr", "show", "wlan0"],
                     ["ip", "-4", "-o", "addr", "show", "scope", "global"]):
        for champ in _sortie(commande).split():
            if "/" in champ and champ.count(".") == 3:
                return champ.split("/")[0]
    return ""


def _ip_par_hostname():
    """Premiere adresse non locale rapportee par hostname."""
    for adresse in _sortie(["hostname", "-I"]).split():
        if adresse.count(".") == 3 and not adresse.startswith("127."):
            return adresse
    return ""


def _ip_par_socket():
    """Adresse de l'interface qui porterait une route sortante.

    Aucun paquet n'est emis : connecter un socket UDP ne fait que fixer la
    route, ce qui fonctionne meme sans acces Internet, donc en mode point
    d'acces.
    """
    prise = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        prise.connect(("10.255.255.255", 1))
        return prise.getsockname()[0]
    except Exception:
        return ""
    finally:
        prise.close()


_METHODES = (
    ("interface", _ip_par_interface),
    ("hostname",  _ip_par_hostname),
    ("socket",    _ip_par_socket),
)


def adresse_ip():
    """Adresse IPv4 du serveur, chaine vide si aucune methode n'aboutit."""
    if IP_FORCEE:
        return IP_FORCEE
    for _, methode in _METHODES:
        adresse = methode()
        if adresse and not adresse.startswith("127."):
            return adresse
    return ""


def diagnostiquer():
    """Affiche ce que rapporte chaque methode.

    Le mode reseau du conteneur determine laquelle est exploitable : avec une
    redirection de ports, les methodes internes decrivent le conteneur et non
    l'hote, et l'adresse annoncee serait alors injoignable depuis un telephone.
    """
    for nom, methode in _METHODES:
        print(f"[reseau] {nom:10s} -> {methode() or 'rien'}")
    print(f"[reseau] retenue    -> {adresse_ip() or 'aucune'}")
