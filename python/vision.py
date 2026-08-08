"""
Module de vision, TankETS (MPU / Qualcomm Linux)
==================================================
Capture camera et fonctions de detection routiere.

Bibliotheques :
    - YOLOv8n       Jocher et al. (2023), Ultralytics
                    https://github.com/ultralytics/ultralytics
    - ONNX Runtime  Microsoft (2021)
                    https://onnxruntime.ai/
    - OpenCV        Bradski, G. (2000), The OpenCV Library
                    https://opencv.org/
    - NumPy         Harris et al. (2020), Array programming with NumPy
                    https://numpy.org/

Algorithmes :
    - Canny Edge Detection
      Canny, J. (1986). A Computational Approach to Edge Detection.
      IEEE Trans. PAMI, 8(6), 679-698.
    - Hough Transform probabiliste
      Matas, J., Galambos, C., & Kittler, J. (2000).
      Robust Detection of Lines Using the Progressive Probabilistic
      Hough Transform. CVIU, 78(1), 119-137.
    - Non-Maximum Suppression
      Neubeck, A., & Van Gool, L. (2006). Efficient Non-Maximum
      Suppression. ICPR 2006.
    - Espace colorimetrique HSV
      Smith, A.R. (1978). Color Gamut Transform Pairs. SIGGRAPH '78.

Donnees :
    - Classes COCO (id 9 = traffic light)
      Lin, T.-Y. et al. (2014). Microsoft COCO: Common Objects
      in Context. ECCV 2014. https://cocodataset.org/

Code adapte de : YOLO Live Object Detection (Arduino App Lab, 2025)

API publique :
    initialiser_modele()            Charge YOLOv8n ONNX
    detecter_feux(frame)            -> [(x1, y1, x2, y2, confiance)]
                                       deux fenetres a resolution native,
                                       voir _fenetres_inference
    classifier_couleur_feu(crop)    -> COULEUR_*
    detecter_lignes(frame)          -> (detecte, ecart_px)
"""

import os

import cv2
import numpy as np
import onnxruntime as ort


# ======================== Constantes ========================

CHEMIN_MODELE = os.getenv(
    "CHEMIN_MODELE",
    os.path.join(os.path.dirname(__file__), "yolov8n.onnx")
)
TAILLE_INFERENCE = int(os.getenv("TAILLE_INFERENCE", "256"))
SEUIL_CONFIANCE  = 0.40
SEUIL_NMS        = 0.45
THREADS_ORT      = int(os.getenv("THREADS_ORT", "2"))

_ID_CLASSE_FEU = 9
_LIGNE_FEU     = 4 + _ID_CLASSE_FEU

COULEUR_AUCUNE = 0
COULEUR_ROUGE  = 1
COULEUR_JAUNE  = 2
COULEUR_VERT   = 3

NOMS_COULEURS = {
    COULEUR_AUCUNE: "AUCUNE",
    COULEUR_ROUGE:  "ROUGE",
    COULEUR_JAUNE:  "JAUNE",
    COULEUR_VERT:   "VERT",
}

_PIXELS_COULEUR_MIN = 25
_PLAGES_HSV = {
    COULEUR_ROUGE: [((0, 90, 90),   (10, 255, 255)),
                    ((170, 90, 90), (180, 255, 255))],
    COULEUR_JAUNE: [((18, 80, 90),  (35, 255, 255))],
    COULEUR_VERT:  [((40, 60, 60),  (90, 255, 255))],
}

ROI_HAUT_LIGNES    = 0.60
PIXELS_MIN_LIGNE   = 800


# ======================== Etat interne ========================

_session    = None
_nom_entree = None


# ======================== Initialisation ========================

def initialiser_modele():
    """Charge le modele YOLOv8n ONNX. Appeler une fois au demarrage."""
    global _session, _nom_entree

    if not os.path.exists(CHEMIN_MODELE):
        raise FileNotFoundError(
            f"Modele introuvable : {CHEMIN_MODELE}\n"
            "Exporter sur un PC :\n"
            "  pip install ultralytics\n"
            "  yolo export model=yolov8n.pt format=onnx imgsz=256 opset=12\n"
            "puis copier yolov8n.onnx dans python/."
        )

    options = ort.SessionOptions()
    options.intra_op_num_threads    = THREADS_ORT
    options.inter_op_num_threads    = 1
    options.execution_mode          = ort.ExecutionMode.ORT_SEQUENTIAL
    options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    options.enable_cpu_mem_arena    = False
    options.enable_mem_pattern      = True

    print(f"[vision] ONNX ({TAILLE_INFERENCE}px, {THREADS_ORT} threads)")
    _session = ort.InferenceSession(
        CHEMIN_MODELE, sess_options=options,
        providers=["CPUExecutionProvider"]
    )
    _nom_entree = _session.get_inputs()[0].name


# ======================== Detection feux (YOLOv8n) ========================

def _nms(boites, scores, seuil_iou):
    x1, y1, x2, y2 = boites[:, 0], boites[:, 1], boites[:, 2], boites[:, 3]
    aires = (x2 - x1) * (y2 - y1)
    ordre = scores.argsort()[::-1]
    gardes = []
    while ordre.size > 0:
        i = ordre[0]
        gardes.append(i)
        xx1 = np.maximum(x1[i], x1[ordre[1:]])
        yy1 = np.maximum(y1[i], y1[ordre[1:]])
        xx2 = np.minimum(x2[i], x2[ordre[1:]])
        yy2 = np.minimum(y2[i], y2[ordre[1:]])
        w = np.maximum(0.0, xx2 - xx1)
        h = np.maximum(0.0, yy2 - yy1)
        inter = w * h
        iou = inter / (aires[i] + aires[ordre[1:]] - inter + 1e-9)
        ordre = ordre[np.where(iou <= seuil_iou)[0] + 1]
    return gardes


def _fenetres_inference(frame):
    """Prepare les fenetres soumises au modele et leur position dans l'image.

    L'entree du modele ONNX est figee a TAILLE_INFERENCE pixels de cote.
    Redimensionner l'image entiere pour l'y faire tenir divise la largeur
    apparente des objets par plus de deux, ce qui rend les feux lointains
    indetectables : un feu de six centimetres vu a un metre passe d'environ
    trente-trois pixels a treize.

    On preleve donc des fenetres a la resolution native, dans le haut de
    l'image ou se trouve un feu de signalisation. Deux fenetres, aux bords
    gauche et droit, car le feu borde la chaussee et n'apparait jamais au
    centre du champ. Le redimensionnement ne sert plus que de repli, quand
    l'image est plus petite que la fenetre.

    Retourne une liste de (fenetre, x0, y0, sx, sy), telle que la coordonnee
    dans l'image d'origine vaut coordonnee_modele * s + origine.
    """
    h, w = frame.shape[:2]

    if w < TAILLE_INFERENCE or h < TAILLE_INFERENCE:
        return [(cv2.resize(frame, (TAILLE_INFERENCE, TAILLE_INFERENCE)),
                 0, 0, w / TAILLE_INFERENCE, h / TAILLE_INFERENCE)]

    return [
        (frame[0:TAILLE_INFERENCE, 0:TAILLE_INFERENCE],
         0, 0, 1.0, 1.0),
        (frame[0:TAILLE_INFERENCE, w - TAILLE_INFERENCE:w],
         w - TAILLE_INFERENCE, 0, 1.0, 1.0),
    ]


def detecter_feux(frame):
    """Retourne [(x1, y1, x2, y2, confiance)] pour chaque feu detecte."""
    if _session is None:
        raise RuntimeError("Appeler initialiser_modele() d'abord.")

    h, w = frame.shape[:2]
    boites = []
    scores = []

    for fenetre, x0, y0, sx, sy in _fenetres_inference(frame):
        image = cv2.cvtColor(fenetre, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
        blob  = np.transpose(image, (2, 0, 1))[None]

        sortie = np.squeeze(
            _session.run(None, {_nom_entree: blob})[0]
        )

        confiances = sortie[_LIGNE_FEU, :]
        masque = confiances >= SEUIL_CONFIANCE
        if not np.any(masque):
            continue

        brutes = sortie[:4, masque]
        cx, cy, bw, bh = brutes[0], brutes[1], brutes[2], brutes[3]
        boites.append(np.stack([
            (cx - bw / 2) * sx + x0, (cy - bh / 2) * sy + y0,
            (cx + bw / 2) * sx + x0, (cy + bh / 2) * sy + y0
        ], axis=1))
        scores.append(confiances[masque])

    if not boites:
        return []

    # Les deux fenetres se chevauchent parfois : la suppression des maxima
    # locaux est appliquee sur leur union, pas fenetre par fenetre.
    boites = np.concatenate(boites, axis=0)
    scores = np.concatenate(scores, axis=0)

    detections = []
    for i in _nms(boites, scores, SEUIL_NMS):
        x1, y1, x2, y2 = boites[i]
        detections.append((
            max(int(x1), 0),  max(int(y1), 0),
            min(int(x2), w),  min(int(y2), h),
            float(scores[i])
        ))
    return detections


# ======================== Couleur du feu (HSV) ========================

def classifier_couleur_feu(image_recadree):
    """Retourne COULEUR_ROUGE, COULEUR_JAUNE, COULEUR_VERT
    ou COULEUR_AUCUNE."""
    if image_recadree.size == 0:
        return COULEUR_AUCUNE

    hsv = cv2.cvtColor(image_recadree, cv2.COLOR_BGR2HSV)
    meilleure, meilleur_n = COULEUR_AUCUNE, 0

    for couleur, plages in _PLAGES_HSV.items():
        n = sum(int(cv2.countNonZero(cv2.inRange(hsv, bas, haut)))
                for bas, haut in plages)
        if n > meilleur_n:
            meilleure, meilleur_n = couleur, n

    return meilleure if meilleur_n >= _PIXELS_COULEUR_MIN else COULEUR_AUCUNE


# ======================== Lignes de route (seuillage + centroide) ========================

def detecter_lignes(frame):
    """Retourne (detecte, ecart_px).
    ecart_px positif = decale a droite, negatif = a gauche.

    Algorithme : seuillage adaptatif sur la ligne noire,
    calcul du centroide dans la region d'interet.
    Reference : Adaptive Thresholding, OpenCV documentation
    https://docs.opencv.org/4.x/d7/d4d/tutorial_py_thresholding.html
    """
    h, w = frame.shape[:2]
    roi = frame[int(h * ROI_HAUT_LIGNES):, :]
    gris = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
    gris = cv2.GaussianBlur(gris, (5, 5), 0)

    masque = cv2.adaptiveThreshold(
        gris, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY_INV, 51, 15
    )

    noyau = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
    masque = cv2.morphologyEx(masque, cv2.MORPH_OPEN, noyau)

    nb_pixels = cv2.countNonZero(masque)
    if nb_pixels < PIXELS_MIN_LIGNE:
        return False, 0

    moments = cv2.moments(masque)
    cx = int(moments["m10"] / moments["m00"])
    ecart = cx - (masque.shape[1] // 2)
    return True, ecart
