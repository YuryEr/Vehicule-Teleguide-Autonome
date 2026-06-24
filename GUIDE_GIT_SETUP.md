# Guide Git — Projet TankEts ELE795

## Vue d'ensemble

Ce guide explique comment configurer Git sur l'Arduino UNO Q et sur votre PC pour travailler en équipe. Le principe est simple : on édite le code sur PC avec VS Code, on push sur GitHub via GitHub Desktop, et on pull sur le board pour tester dans Arduino App Lab.

```
VS Code (PC) → GitHub Desktop → GitHub → git pull → Arduino UNO Q (App Lab)
```

Ce guide est divisé en deux parties indépendantes :

- **Partie A** — Configuration de l'Arduino UNO Q (pour tester sur le véhicule)
- **Partie B** — Configuration du PC (pour éditer le code)

Vous pouvez suivre l'une sans avoir lu l'autre.

---

## Prérequis communs

- Un compte GitHub (demander à Yury de vous ajouter comme collaborateur sur le repo)
- Arduino App Lab installé (pour le board)

### Créer votre Personal Access Token GitHub

GitHub n'accepte plus les mots de passe en ligne de commande. Il faut créer un token. **Ce token est nécessaire uniquement pour l'Arduino UNO Q.** GitHub Desktop gère l'authentification automatiquement sur PC.

1. Aller sur [github.com](https://github.com)
2. Cliquer sur votre **photo de profil** (en haut à droite) → **Settings**
3. Descendre tout en bas du menu de gauche → **Developer settings**
4. **Personal access tokens** → **Tokens (classic)**
5. **Generate new token** → **Generate new token (classic)**
6. Donner un nom au token (exemple : `arduino-uno-q`)
7. Expiration : **90 days** (ou plus selon votre préférence)
8. Cocher uniquement la case **repo** (ça coche automatiquement les sous-cases)
9. Cliquer **Generate token** en bas de la page
10. **COPIER LE TOKEN IMMÉDIATEMENT** — il commence par `ghp_...` et GitHub ne le montrera plus jamais après avoir quitté la page

> **Important :** Gardez ce token quelque part de sécuritaire (gestionnaire de mots de passe, note privée). Vous en aurez besoin pour l'authentification sur l'Arduino.

---

# PARTIE A — Arduino UNO Q (board)

> **⚠️ Avant de commencer :** Si vous utilisez le Arduino UNO Q qui est déjà monté sur le véhicule, il est très probable que Git soit déjà configuré dessus. **Vérifiez d'abord** en ouvrant le terminal App Lab et en tapant :
> ```bash
> cd /home/arduino/ArduinoApps/tankets_ele795
> git status
> ```
> Si ça affiche quelque chose comme `On branch main, nothing to commit`, tout est déjà en place. Vous pouvez sauter directement à la section **Workflow sur le board**.
>
> **Note :** Même si Git est déjà configuré, le token d'authentification sauvegardé est celui de la personne qui a fait le setup initial. Si vous devez faire des `git push` depuis le board (rare — normalement on push depuis le PC), vous devrez entrer votre propre token.

### A.1 Ouvrir le terminal

Dans Arduino App Lab, ouvrez le terminal intégré (c'est un terminal SSH vers le board).

### A.2 Vérifier que Git est installé

```bash
git --version
```

Si ça affiche une version (ex: `git version 2.47.3`), c'est bon. Sinon :

```bash
sudo apt update && sudo apt install git -y
```

### A.3 Configurer votre identité

```bash
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@ets.ca"
```

### A.4 Sauvegarder le token pour ne pas le retaper

```bash
git config --global credential.helper store
```

### A.5 Configurer le terminal pour ouvrir directement dans le projet

À chaque ouverture du terminal App Lab, vous repartez dans le dossier home (`~`). Exécutez cette commande **une seule fois** pour régler ça :

```bash
echo 'cd /home/arduino/ArduinoApps/tankets_ele795' >> ~/.bashrc
```

Désormais, chaque nouveau terminal s'ouvrira directement dans le dossier du projet.

### A.6 Initialiser Git dans le dossier du projet App Lab

```bash
cd /home/arduino/ArduinoApps/tankets_ele795
git init
git remote add origin https://github.com/YuryEr/ELE795-TankETS.git
git branch -M main
git pull origin main
```

Quand Git demande l'authentification :
- **Username :** votre nom d'utilisateur GitHub
- **Password :** votre Personal Access Token `ghp_...` (pas votre mot de passe GitHub)

> **Note :** Quand vous collez le token dans le terminal, **rien ne s'affiche** (pas d'étoiles, rien). C'est normal sous Linux. Collez et appuyez sur Enter.

### A.7 Créer le raccourci de synchronisation

Exécutez cette commande **une seule fois** :

```bash
git config --global alias.sync '!git fetch origin && git reset --hard origin/main'
```

Désormais, pour mettre à jour le board avec la dernière version sur GitHub, tapez simplement :

```bash
git sync
```

---

### Workflow sur le board

**Mettre à jour avec la branche principale (main) :**

```bash
git sync
```

Puis cliquez **Run** dans App Lab pour tester.

**Tester la branche d'un coéquipier :**

Chaque membre de l'équipe a sa propre branche :

| Branche | Membre |
|---|---|
| `Yury` | Yury |
| `Yoan` | Yoan |
| `Ryan` | Ryan |
| `Serby` | Serby |

Pour tester la branche d'un coéquipier :

```bash
git fetch origin
git checkout Yoan
git reset --hard origin/Yoan
```

Remplacez `Yoan` par le nom de la branche à tester. Cliquez **Run** dans App Lab.

**Revenir sur main après le test :**

```bash
git checkout main
git reset --hard origin/main
```

> **Astuce :** Pensez à toujours revenir sur `main` quand vous avez fini de tester une branche personnelle, pour éviter de pousser des changements au mauvais endroit par accident.

---

# PARTIE B — PC (GitHub Desktop + VS Code)

### B.1 Installer GitHub Desktop

1. Télécharger depuis [desktop.github.com](https://desktop.github.com/)
2. Installer et ouvrir l'application
3. Cliquer **Sign in to GitHub.com**
4. Se connecter avec votre compte GitHub (l'authentification se fait automatiquement, pas besoin de token)

### B.2 Cloner le repo

1. Dans GitHub Desktop : **File → Clone Repository**
2. Chercher `YuryEr/ELE795-TankETS` dans la liste (ou aller dans l'onglet **URL** et coller `https://github.com/YuryEr/ELE795-TankETS.git`)
3. **Local Path :** choisir un dossier hors OneDrive, par exemple `C:\Dev\ELE795-TankETS`
4. Cliquer **Clone**

> **⚠️ Ne pas cloner dans un dossier OneDrive.** OneDrive synchronise les fichiers en arrière-plan et peut corrompre le dossier `.git/`. Le code est déjà sauvegardé sur GitHub, vous n'avez pas besoin de OneDrive en plus.

### B.3 Ouvrir le projet dans VS Code

Dans GitHub Desktop : **Repository → Open in Visual Studio Code**

Vous pouvez maintenant éditer les fichiers dans VS Code.

---

### Workflow sur PC

**Avant de commencer à travailler :**

Dans GitHub Desktop, cliquez **Fetch origin** (en haut) pour récupérer les derniers changements, puis **Pull origin** si des changements sont disponibles.

**Après avoir modifié vos fichiers :**

1. GitHub Desktop affiche automatiquement la liste des fichiers modifiés dans le panneau de gauche
2. En bas à gauche, écrire un **résumé** du changement (ex: "Mode manuel: contrôle moteurs via Bluetooth")
3. Cliquer **Commit to [votre branche]**
4. Cliquer **Push origin** en haut pour envoyer sur GitHub

**Changer de branche :**

En haut de GitHub Desktop, cliquer sur **Current Branch** → sélectionner votre branche personnelle (`Yury`, `Yoan`, `Ryan`, `Serby`).

**Fusionner via Pull Request :**

1. Après avoir push votre branche, GitHub Desktop affiche un bouton **Create Pull Request** — cliquer dessus
2. Ça ouvre GitHub dans le navigateur
3. Décrire vos changements
4. Un coéquipier review le code
5. Cliquer **Merge pull request**

---

## Résumé des commandes (board uniquement)

| Action | Commande |
|---|---|
| Mettre à jour avec main | `git sync` |
| Tester une branche | `git fetch origin && git checkout Yoan && git reset --hard origin/Yoan` |
| Revenir sur main | `git checkout main && git reset --hard origin/main` |
| Voir l'état des fichiers | `git status` |
| Voir la branche active | `git branch` |

> **Note :** Sur PC, toutes les commandes se font via l'interface GitHub Desktop. Pas besoin de terminal.

---

## Dépannage

### Board : "fatal: not a git repository"
Vous n'êtes pas dans le bon dossier. Tapez `cd /home/arduino/ArduinoApps/tankets_ele795`.

### Board : "Username/Password" demandé à chaque fois
Exécutez `git config --global credential.helper store` puis entrez votre token une dernière fois.

### Board : le terminal s'ouvre dans `~` au lieu du projet
Exécutez `echo 'cd /home/arduino/ArduinoApps/tankets_ele795' >> ~/.bashrc` une fois.

### PC : "error: failed to push some refs"
Quelqu'un d'autre a push avant vous. Dans GitHub Desktop, cliquez **Fetch origin** puis **Pull origin** d'abord.

### PC : conflit de merge
GitHub Desktop vous montrera les fichiers en conflit. Ouvrez-les dans VS Code — les conflits sont marqués avec `<<<<<<<` et `>>>>>>>`. Choisissez la version à garder, supprimez les marqueurs, puis revenez dans GitHub Desktop pour commiter.

---

## Structure du projet

```
tankets_ele795/
├── python/          ← Code Python (Linux - Qualcomm)
│   └── main.py
├── sketch/          ← Sketch Arduino (MCU - STM32)
│   ├── sketch.ino
│   └── sketch.yaml
├── app.yaml         ← Configuration App Lab
├── .gitignore
└── README.md
```

---

*Dernière mise à jour : Juin 2026*