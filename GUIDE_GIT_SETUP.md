# Guide Git — Projet TankEts ELE795

## Vue d'ensemble

Ce guide explique comment configurer Git sur l'Arduino UNO Q et sur votre PC pour travailler en équipe. Le principe est simple : on édite le code sur PC avec VS Code, on push sur GitHub, et on pull sur le board pour tester dans Arduino App Lab.

```
VS Code (PC) → git push → GitHub → git pull → Arduino UNO Q (App Lab)
```

---

## Prérequis

- Un compte GitHub (demander à Yury de vous ajouter comme collaborateur sur le repo)
- Git installé sur votre PC ([git-scm.com](https://git-scm.com/) sur Windows)
- Arduino App Lab installé
- Accès au terminal SSH du Arduino UNO Q (via le terminal intégré dans App Lab)

---

## Partie 1 — Créer votre Personal Access Token GitHub

GitHub n'accepte plus les mots de passe en ligne de commande. Il faut créer un token.

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

> **Important :** Gardez ce token quelque part de sécuritaire (gestionnaire de mots de passe, note privée). Vous en aurez besoin pour l'authentification Git.

---

## Partie 2 — Configuration sur votre PC (VS Code)

### 2.1 Installer Git

Téléchargez et installez Git depuis [git-scm.com](https://git-scm.com/). Pendant l'installation, gardez les options par défaut.

### 2.2 Cloner le repo

Ouvrez un terminal (PowerShell, CMD, ou le terminal VS Code avec `Ctrl+ù`) :

```bash
cd C:\Dev
git clone https://github.com/YuryEr/ELE795-TankETS.git
cd ELE795-TankETS
```

> **⚠️ Ne pas cloner dans un dossier OneDrive.** OneDrive synchronise les fichiers en arrière-plan et peut corrompre le dossier `.git/`. Le code est déjà sauvegardé sur GitHub, vous n'avez pas besoin de OneDrive en plus. Utilisez un dossier comme `C:\Dev\` ou `C:\Projets\`.

Quand Git demande l'authentification :
- **Username :** votre nom d'utilisateur GitHub
- **Password :** votre Personal Access Token (pas votre mot de passe GitHub)

### 2.3 Configurer votre identité Git

```bash
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@ets.ca"
```

### 2.4 Sauvegarder le token pour ne pas le retaper

```bash
git config --global credential.helper store
```

La prochaine fois que vous entrez votre token, il sera sauvegardé. Plus besoin de le retaper après ça.

### 2.5 Ouvrir le projet dans VS Code

```
File → Open Folder → C:\Dev\ELE795-TankETS
```

Vous pouvez maintenant éditer les fichiers directement dans VS Code.

---

## Partie 3 — Configuration sur l'Arduino UNO Q

> **⚠️ Avant de commencer :** Si vous utilisez le Arduino UNO Q qui est déjà monté sur le véhicule, il est très probable que Git soit déjà configuré dessus. **Vérifiez d'abord** en ouvrant le terminal App Lab et en tapant :
> ```bash
> cd /home/arduino/ArduinoApps/tankets_ele795
> git status
> ```
> Si ça affiche quelque chose comme `On branch main, nothing to commit`, tout est déjà en place. Vous pouvez sauter directement à la **Partie 4 — Workflow quotidien**. Les étapes ci-dessous sont uniquement pour configurer un board neuf ou réinitialisé.
>
> **Note :** Même si Git est déjà configuré, le token d'authentification sauvegardé est celui de la personne qui a fait le setup initial. Si vous devez faire des `git push` depuis le board (rare — normalement on push depuis le PC), vous devrez entrer votre propre token.

### 3.1 Ouvrir le terminal

Dans Arduino App Lab, ouvrez le terminal intégré (c'est un terminal SSH vers le board).

### 3.2 Vérifier que Git est installé

```bash
git --version
```

Si ça affiche une version (ex: `git version 2.47.3`), c'est bon. Sinon :

```bash
sudo apt update && sudo apt install git -y
```

### 3.3 Configurer votre identité

```bash
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@ets.ca"
```

### 3.4 Sauvegarder le token

```bash
git config --global credential.helper store
```

### 3.5 Se déplacer dans le dossier du projet

À chaque ouverture du terminal App Lab, vous repartez dans le dossier home (`~`). Il faut se déplacer dans le dossier du projet :

```bash
cd /home/arduino/ArduinoApps/tankets_ele795
```

Pour ne plus avoir à le taper à chaque fois, exécutez cette commande **une seule fois** :

```bash
echo 'cd /home/arduino/ArduinoApps/tankets_ele795' >> ~/.bashrc
```

Désormais, chaque nouveau terminal s'ouvrira directement dans le dossier du projet.

### 3.6 Initialiser Git dans le dossier du projet App Lab

Le projet App Lab est stocké dans `/home/arduino/ArduinoApps/tankets_ele795/`.

Si Git n'est pas encore configuré dans ce dossier :

```bash
cd /home/arduino/ArduinoApps/tankets_ele795/
git init
git remote add origin https://github.com/YuryEr/ELE795-TankETS.git
git branch -M main
git pull origin main
```

---

## Partie 4 — Workflow quotidien

### Sur votre PC (VS Code) — Éditer et envoyer le code

```bash
# 1. Avant de commencer, récupérer les derniers changements
git pull

# 2. Modifier vos fichiers dans VS Code...

# 3. Voir ce qui a changé
git status

# 4. Ajouter vos changements
git add .

# 5. Commiter avec un message descriptif
git commit -m "Description claire de ce que vous avez changé"

# 6. Envoyer sur GitHub
git push
```

### Sur le board (terminal App Lab) — Récupérer et tester

Pour mettre à jour le board avec la dernière version de `main` sur GitHub, **en écrasant toute modification locale** :

```bash
git fetch origin
git reset --hard origin/main
```

Ensuite, cliquez **Run** dans App Lab pour tester.

> **Raccourci :** Vous pouvez créer un alias pour faire les deux commandes en une :
> ```bash
> git config --global alias.sync '!git fetch origin && git reset --hard origin/main'
> ```
> Après ça, tapez simplement `git sync` pour mettre à jour le board.

### Sur le board — Tester la branche d'un coéquipier

Chaque membre de l'équipe a sa propre branche :

| Branche | Membre |
|---|---|
| `Yury` | Yury |
| `Yoan` | Yoan |
| `Ryan` | Ryan |
| `Serby` | Serby |

Pour tester la branche d'un coéquipier sur le board :

```bash
git fetch origin
git checkout Yoan
git reset --hard origin/Yoan
```

Remplacez `Yoan` par le nom de la branche à tester. Cliquez **Run** dans App Lab pour tester.

**Pour revenir sur main après le test :**

```bash
git checkout main
git reset --hard origin/main
```

> **Astuce :** Pensez à toujours revenir sur `main` quand vous avez fini de tester une branche personnelle, pour éviter de pousser des changements au mauvais endroit par accident.

---

## Partie 5 — Travailler en équipe avec les branches

**Règle d'or : personne ne push directement sur `main`.**

Chacun travaille sur sa propre branche et on fusionne via Pull Request sur GitHub.

### Créer une branche pour votre travail

Chaque membre a déjà sa branche personnelle (`Yury`, `Yoan`, `Ryan`, `Serby`). Pour basculer sur votre branche :

```bash
git checkout Yury
```

Remplacez `Yury` par votre propre nom de branche.

### Travailler sur votre branche

```bash
# Modifier vos fichiers...
git add .
git commit -m "Mode manuel: contrôle moteurs via Bluetooth"
git push -u origin Yury
```

Remplacez `Yury` par votre nom de branche.

### Fusionner via Pull Request

1. Aller sur GitHub → votre repo
2. GitHub affiche un bandeau jaune "Compare & pull request" — cliquer dessus
3. Décrire vos changements
4. Un coéquipier review le code
5. Cliquer **Merge pull request**

### Revenir sur main après le merge

```bash
git checkout main
git pull
```

---

## Résumé des commandes essentielles

| Action | Commande |
|---|---|
| Récupérer les changements | `git pull` |
| Voir l'état des fichiers | `git status` |
| Ajouter tous les changements | `git add .` |
| Commiter | `git commit -m "message"` |
| Envoyer sur GitHub | `git push` |
| Écraser le local avec GitHub (main) | `git fetch origin && git reset --hard origin/main` |
| Tester la branche d'un coéquipier | `git fetch origin && git checkout Yoan && git reset --hard origin/Yoan` |
| Revenir sur main | `git checkout main && git reset --hard origin/main` |
| Changer de branche | `git checkout NomDeBranche` |

---

## Dépannage

### "fatal: not a git repository"
Vous n'êtes pas dans le bon dossier. Faites `cd` vers le dossier du projet.

### "error: failed to push some refs"
Quelqu'un d'autre a push avant vous. Faites `git pull` d'abord, puis `git push`.

### "Username/Password" demandé à chaque fois
Exécutez `git config --global credential.helper store` puis entrez votre token une dernière fois.

### Le terminal n'affiche rien quand je colle mon token
C'est normal. Les mots de passe ne s'affichent pas dans un terminal Linux (pas d'étoiles, rien). Collez et appuyez sur Enter.

### Conflit de merge
Si Git dit qu'il y a un conflit, ouvrez les fichiers concernés dans VS Code. Les conflits sont marqués avec `<<<<<<<` et `>>>>>>>`. Choisissez la version à garder, supprimez les marqueurs, puis `git add .` et `git commit`.

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
