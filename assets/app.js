/*
 * app.js — Logique interface web TankETS
 * PFE ELE795 - ETS - Ete 2026
 *
 * Sources :
 *   - Socket.IO (v4) : https://socket.io/
 *   - Blockly (Google) : https://developers.google.com/blockly
 */

// ================================================================
// SOCKET.IO
// ================================================================
const socket = io();
const errorContainer = document.getElementById('error-container');

socket.on('connect', () => {
    if (errorContainer) errorContainer.style.display = 'none';
});

socket.on('disconnect', () => {
    if (errorContainer) {
        errorContainer.textContent =
            'Connexion perdue. Verifiez la connexion au vehicule.';
        errorContainer.style.display = 'block';
    }
});

socket.on('mode_actuel', (data) => mettreAJourAffichageMode(data.mode));
socket.on('etat_camera', (data) => mettreAJourAffichageCamera(data.active));
socket.on('etat_led1',   (data) => mettreAJourAffichageLed(1, data.active, data.mode));
socket.on('etat_led2',   (data) => mettreAJourAffichageLed(2, data.active, data.mode));


// ================================================================
// GESTION DES ONGLETS
// ================================================================
let workspaceInitialise = false;

function afficherOnglet(nom) {
    const pilotage = document.getElementById('onglet-pilotage');
    const blocs    = document.getElementById('onglet-blocs');
    const btnPilot = document.getElementById('btn-onglet-pilotage');
    const btnBlocs = document.getElementById('btn-onglet-blocs');

    if (nom === 'pilotage') {
        pilotage.style.display = 'flex';
        blocs.style.display    = 'none';
        btnPilot.classList.add('onglet-actif');
        btnBlocs.classList.remove('onglet-actif');
    } else {
        pilotage.style.display = 'none';
        blocs.style.display    = 'flex';
        btnBlocs.classList.add('onglet-actif');
        btnPilot.classList.remove('onglet-actif');
        if (workspaceInitialise) {
            Blockly.svgResize(workspace);
        }
    }
}


// ================================================================
// BLOCKLY — Definition des blocs personnalises
// ================================================================
let workspace = null;

if (typeof Blockly !== 'undefined') {

    Blockly.Blocks['avancer'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("avancer de")
                .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "VALEUR")
                .appendField("m");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(180);
            this.setTooltip("Le vehicule avance de la distance indiquee");
        }
    };

    Blockly.Blocks['reculer'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("reculer de")
                .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "VALEUR")
                .appendField("m");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(180);
            this.setTooltip("Le vehicule recule de la distance indiquee");
        }
    };

    Blockly.Blocks['tourner_gauche'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("tourner a gauche de")
                .appendField(new Blockly.FieldNumber(90, 45, 360, 45), "VALEUR")
                .appendField("\u00B0");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(200);
            this.setTooltip("Le vehicule pivote a gauche de l'angle indique");
        }
    };

    Blockly.Blocks['tourner_droite'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("tourner a droite de")
                .appendField(new Blockly.FieldNumber(90, 45, 360, 45), "VALEUR")
                .appendField("\u00B0");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(200);
            this.setTooltip("Le vehicule pivote a droite de l'angle indique");
        }
    };

    Blockly.Blocks['led_allumer'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("allumer bandeau")
                .appendField(new Blockly.FieldDropdown([
                    ["1", "1"], ["2", "2"]
                ]), "BANDEAU")
                .appendField("en mode")
                .appendField(new Blockly.FieldDropdown([
                    ["gyrophare",  "1"],
                    ["clignotant", "2"],
                    ["phares",     "3"]
                ]), "MODE");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(280);
            this.setTooltip("Allume le bandeau LED dans le mode choisi");
        }
    };

    Blockly.Blocks['led_eteindre'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("eteindre bandeau")
                .appendField(new Blockly.FieldDropdown([
                    ["1", "1"], ["2", "2"]
                ]), "BANDEAU");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(280);
            this.setTooltip("Eteint le bandeau LED");
        }
    };

    Blockly.Blocks['attendre'] = {
        init: function() {
            this.appendDummyInput()
                .appendField("attendre")
                .appendField(new Blockly.FieldNumber(1, 0.1, 60, 0.1), "VALEUR")
                .appendField("s");
            this.setPreviousStatement(true);
            this.setNextStatement(true);
            this.setColour(40);
            this.setTooltip("Pause avant la commande suivante");
        }
    };

    workspace = Blockly.inject('blockly-div', {
        toolbox: document.getElementById('toolbox'),
        scrollbars: true,
        trashcan: true,
        zoom: { controls: true, wheel: false, startScale: 1.0 }
    });
    workspaceInitialise = true;

} else {
    document.getElementById('blockly-div').innerHTML =
        '<p style="padding:20px;color:#b00020;">Blockly non charge — ' +
        'verifiez que les fichiers sont dans libs/blockly/</p>';
}


// ================================================================
// BLOCKLY — Extraction et execution de la sequence
// ================================================================
function extraireSequence() {
    const sequence = [];
    if (!workspace) return sequence;

    const topBlocks = workspace.getTopBlocks(true);
    if (topBlocks.length === 0) return sequence;

    let bloc = topBlocks[0];
    while (bloc) {
        switch (bloc.type) {
            case 'avancer':
                sequence.push({ cmd: 'avancer',
                                valeur: parseFloat(bloc.getFieldValue('VALEUR')) });
                break;
            case 'reculer':
                sequence.push({ cmd: 'reculer',
                                valeur: parseFloat(bloc.getFieldValue('VALEUR')) });
                break;
            case 'tourner_gauche':
                sequence.push({ cmd: 'tourner_gauche',
                                valeur: parseFloat(bloc.getFieldValue('VALEUR')) });
                break;
            case 'tourner_droite':
                sequence.push({ cmd: 'tourner_droite',
                                valeur: parseFloat(bloc.getFieldValue('VALEUR')) });
                break;
            case 'led_allumer':
                sequence.push({ cmd: 'led',
                                bandeau: parseInt(bloc.getFieldValue('BANDEAU')),
                                mode:    parseInt(bloc.getFieldValue('MODE')) });
                break;
            case 'led_eteindre':
                sequence.push({ cmd: 'led_off',
                                bandeau: parseInt(bloc.getFieldValue('BANDEAU')) });
                break;
            case 'attendre':
                sequence.push({ cmd: 'attendre',
                                valeur: parseFloat(bloc.getFieldValue('VALEUR')) });
                break;
        }
        bloc = bloc.getNextBlock();
    }
    return sequence;
}

function executerBlocs() {
    const sequence = extraireSequence();
    if (sequence.length === 0) {
        document.getElementById('sequence-statut').textContent =
            'Aucun bloc — glissez des blocs depuis la barre laterale';
        return;
    }
    socket.emit('executer_sequence', { sequence: sequence });
}

function stopSequence() {
    socket.emit('stop_sequence');
}

function effacerBlocs() {
    if (workspace) workspace.clear();
    document.getElementById('sequence-statut').textContent = 'Pret';
}

socket.on('sequence_status', (data) => {
    const statut      = document.getElementById('sequence-statut');
    const btnExecuter = document.getElementById('btn-executer');
    const btnStop     = document.getElementById('btn-stop');

    if (data.etat === 'en_cours') {
        statut.textContent = `${data.index}/${data.total} — ${data.description}`;
        btnExecuter.disabled = true;
        btnStop.disabled     = false;
    } else if (data.etat === 'terminee') {
        statut.textContent = '\u2713 Sequence terminee';
        btnExecuter.disabled = false;
        btnStop.disabled     = true;
    } else if (data.etat === 'arretee') {
        statut.textContent = '\u25A0 Sequence interrompue';
        btnExecuter.disabled = false;
        btnStop.disabled     = true;
    } else if (data.etat === 'erreur') {
        statut.textContent = '\u26A0 ' + data.description;
        btnExecuter.disabled = false;
        btnStop.disabled     = true;
    }
});


// ================================================================
// STREAMING VIDEO
// ================================================================
const videoStream = document.getElementById('video-stream');
const placeholder = document.getElementById('video-placeholder');
const streamUrl   = `http://${window.location.hostname}:7000/video`;

function chargerVideo() {
    videoStream.src = streamUrl;
    videoStream.style.display = 'block';
    placeholder.style.display = 'none';
}

videoStream.onerror = () => {
    videoStream.style.display = 'none';
    placeholder.style.display = 'flex';
    placeholder.querySelector('.feedback-text').textContent =
        'Camera non disponible';
    setTimeout(chargerVideo, 2000);
};

chargerVideo();


// ================================================================
// MODE DE CONDUITE
// ================================================================
const MODES = ["manuel", "semi-autonome", "autonome"];
let indexMode = 0;

function changerMode() {
    indexMode = (indexMode + 1) % MODES.length;
    const nouveauMode = MODES[indexMode];
    socket.emit('changer_mode', { mode: nouveauMode });
    mettreAJourAffichageMode(nouveauMode);
}

function mettreAJourAffichageMode(mode) {
    const badge = document.getElementById('mode-badge');
    badge.textContent = mode.toUpperCase().replace('-', '\u2011');
    badge.className   = 'mode-actuel-badge mode-' + mode.replace('-', '');
}


// ================================================================
// CAMERA
// ================================================================
let cameraActive = true;

function toggleCamera() {
    cameraActive = !cameraActive;
    socket.emit('toggle_camera', { active: cameraActive });
    mettreAJourAffichageCamera(cameraActive);
    if (cameraActive) {
        chargerVideo();
    } else {
        videoStream.src = '';
        videoStream.style.display = 'none';
        placeholder.style.display = 'flex';
        placeholder.querySelector('.feedback-text').textContent =
            'Camera eteinte';
    }
}

function mettreAJourAffichageCamera(active) {
    const indicateur = document.getElementById('indicateur-camera');
    const texte      = document.getElementById('texte-camera');
    const btn        = document.getElementById('btn-camera');
    if (active) {
        indicateur.className = 'indicateur-on';
        texte.textContent    = 'Active';
        btn.textContent      = 'Eteindre la camera';
        btn.className        = 'btn-camera btn-camera-off';
    } else {
        indicateur.className = 'indicateur-off';
        texte.textContent    = 'Eteinte';
        btn.textContent      = 'Allumer la camera';
        btn.className        = 'btn-camera btn-camera-on';
    }
}


// ================================================================
// BANDEAUX LED
// ================================================================
const MODES_LED   = ["ETEINT", "GYROPHARE", "CLIGNOTANT", "PHARES"];
const CLASSES_LED = [
    "mode-led-eteint", "mode-led-gyrophare",
    "mode-led-clignotant", "mode-led-phares"
];
const ledOn   = { 1: false, 2: false };
const ledMode = { 1: 1, 2: 1 };

function toggleOnoffLed(num) {
    ledOn[num] = !ledOn[num];
    socket.emit(`onoff_led${num}`, { active: ledOn[num] });
    mettreAJourAffichageLed(num, ledOn[num], ledMode[num]);
}

function changerModeLed(num) {
    ledMode[num] = (ledMode[num] % 3) + 1;
    socket.emit(`mode_led${num}`, { mode: ledMode[num] });
    mettreAJourAffichageLed(num, ledOn[num], ledMode[num]);
}

function mettreAJourAffichageLed(num, active, mode) {
    const indicateur = document.getElementById(`indicateur-led${num}`);
    const texte      = document.getElementById(`texte-led${num}`);
    const btnOnoff   = document.getElementById(`btn-onoff-led${num}`);
    const badge      = document.getElementById(`badge-led${num}`);

    if (active) {
        indicateur.className = 'indicateur-on';
        texte.textContent    = 'Allume';
        btnOnoff.textContent = 'Eteindre';
        btnOnoff.className   = 'btn-camera btn-camera-off';
    } else {
        indicateur.className = 'indicateur-off';
        texte.textContent    = 'Eteint';
        btnOnoff.textContent = 'Allumer';
        btnOnoff.className   = 'btn-camera btn-camera-on';
    }

    const modeAff     = mode > 0 ? mode : ledMode[num];
    badge.textContent = MODES_LED[modeAff];
    badge.className   = `mode-actuel-badge ${CLASSES_LED[modeAff]}`;
    ledOn[num]        = active;
    if (mode > 0) ledMode[num] = mode;
}


// ================================================================
// JOYSTICK VIRTUEL
// ================================================================
const zone             = document.getElementById('joystick-zone');
const poignee          = document.getElementById('joystick-poignee');
const valeursAffichage = document.getElementById('joystick-valeurs');

let joystickActif = false;
let centreX = 0, centreY = 0;
const RAYON_MAX     = 60;
const INTERVALLE_MS = 50;
let intervalJoystick = null;
let dernierX = 0, dernierY = 0;

function obtenirCentre() {
    const rect = zone.querySelector('.joystick-base').getBoundingClientRect();
    centreX = rect.left + rect.width  / 2;
    centreY = rect.top  + rect.height / 2;
}

function calculerPosition(clientX, clientY) {
    let dx = clientX - centreX;
    let dy = clientY - centreY;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance > RAYON_MAX) {
        dx = (dx / distance) * RAYON_MAX;
        dy = (dy / distance) * RAYON_MAX;
    }
    return { dx, dy };
}

function mettreAJourPoignee(dx, dy) {
    poignee.style.transform = `translate(${dx}px, ${dy}px)`;
    dernierX = parseFloat((dx / RAYON_MAX).toFixed(2));
    dernierY = parseFloat((-dy / RAYON_MAX).toFixed(2));
    valeursAffichage.textContent =
        `x: ${dernierX.toFixed(2)} | y: ${dernierY.toFixed(2)}`;
}

function demarrerJoystick(clientX, clientY) {
    joystickActif = true;
    obtenirCentre();
    intervalJoystick = setInterval(() => {
        socket.emit('joystick', { x: dernierX, y: dernierY });
    }, INTERVALLE_MS);
}

function deplacerJoystick(clientX, clientY) {
    if (!joystickActif) return;
    const { dx, dy } = calculerPosition(clientX, clientY);
    mettreAJourPoignee(dx, dy);
}

function relacherJoystick() {
    if (!joystickActif) return;
    joystickActif = false;
    mettreAJourPoignee(0, 0);
    dernierX = 0;
    dernierY = 0;
    clearInterval(intervalJoystick);
    socket.emit('joystick', { x: 0, y: 0 });
}

// Evenements souris
zone.addEventListener('mousedown', (e) => {
    e.preventDefault();
    demarrerJoystick(e.clientX, e.clientY);
});
document.addEventListener('mousemove', (e) => {
    deplacerJoystick(e.clientX, e.clientY);
});
document.addEventListener('mouseup', relacherJoystick);

// Evenements tactiles
zone.addEventListener('touchstart', (e) => {
    e.preventDefault();
    demarrerJoystick(e.touches[0].clientX, e.touches[0].clientY);
}, { passive: false });

document.addEventListener('touchmove', (e) => {
    if (!joystickActif) return;
    e.preventDefault();
    deplacerJoystick(e.touches[0].clientX, e.touches[0].clientY);
}, { passive: false });

document.addEventListener('touchend', relacherJoystick);

// ======================== Detection (vision) ========================

const detectionTexte = document.getElementById('detection-texte');
const detectionsActives = {};

function mettreAJourDetectionTexte() {
    const parties = Object.values(detectionsActives).filter(Boolean);
    detectionTexte.textContent = parties.length ? parties.join(' | ') : '';
}

socket.on('etat_feu', (data) => {
    detectionsActives.feu = data.present ? 'Feu ' + data.couleur : null;
    mettreAJourDetectionTexte();
});

socket.on('etat_lignes', (data) => {
    detectionsActives.lignes = data.detecte ? 'Lignes detectees' : null;
    mettreAJourDetectionTexte();
});