"""
Module de vision — TankETS (MPU / Qualcomm Linux)
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
    initialiser_camera(index)       Demarre la capture
    initialiser_modele()            Charge YOLOv8n ONNX
    arreter_camera()                Arrete la capture
    derniere_image()                -> np.ndarray ou None
    detecter_feux(frame)            -> [(x1, y1, x2, y2, confiance)]
    classifier_couleur_feu(crop)    -> COULEUR_*
    detecter_lignes(frame)          -> (detecte, ecart_px)
"""

import os
import time
import threading

import cv2
import numpy as np
import onnxruntime as ort


# ======================== Constantes ========================

INDEX_CAMERA_DEFAUT = int(os.getenv("INDEX_CAMERA", "2"))
LARGEUR_CAMERA      = 640
HAUTEUR_CAMERA      = 480

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

ROI_HAUT_LIGNES  = 0.55
REDUCTION_LIGNES = 0.5


# ======================== Etat interne ========================

_camera     = None
_session    = None
_nom_entree = None


# ======================== Capture camera ========================

class CaptureCamera:
    """Thread dedie qui garde uniquement la derniere frame
    pour eviter le lag du buffer V4L2."""

    def __init__(self, index):
        self._cap = cv2.VideoCapture(index, cv2.CAP_V4L2)
        self._cap.set(cv2.CAP_PROP_FOURCC,
                      cv2.VideoWriter_fourcc(*"MJPG"))
        self._cap.set(cv2.CAP_PROP_FRAME_WIDTH,  LARGEUR_CAMERA)
        self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HAUTEUR_CAMERA)
        self._cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        if not self._cap.isOpened():
            raise RuntimeError(
                f"Impossible d'ouvrir /dev/video{index}. "
                "Verifier v4l2-ctl --list-devices et INDEX_CAMERA."
            )
        self._frame = None
        self._lock  = threading.Lock()
        self._actif = True
        threading.Thread(target=self._boucle, daemon=True).start()
        print(f"[vision] Camera index {index} "
              f"({LARGEUR_CAMERA}x{HAUTEUR_CAMERA})")

    def _boucle(self):
        while self._actif:
            ok, frame = self._cap.read()
            if ok:
                with self._lock:
                    self._frame = frame
            else:
                time.sleep(0.01)

    def lire(self):
        with self._lock:
            return None if self._frame is None else self._frame.copy()

    def arreter(self):
        self._actif = False
        self._cap.release()


# ======================== Initialisation ========================

def initialiser_camera(index=INDEX_CAMERA_DEFAUT):
    """Demarre la capture camera dans un thread dedie."""
    global _camera
    if _camera is not None:
        _camera.arreter()
    _camera = CaptureCamera(index)


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


def arreter_camera():
    """Arrete proprement la capture camera."""
    global _camera
    if _camera is not None:
        _camera.arreter()
        _camera = None


def derniere_image():
    """Retourne la derniere frame capturee, ou None."""
    if _camera is None:
        return None
    return _camera.lire()


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


def detecter_feux(frame):
    """Retourne [(x1, y1, x2, y2, confiance)] pour chaque feu detecte."""
    if _session is None:
        raise RuntimeError("Appeler initialiser_modele() d'abord.")

    h, w = frame.shape[:2]
    image = cv2.resize(frame, (TAILLE_INFERENCE, TAILLE_INFERENCE))
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    blob  = np.transpose(image, (2, 0, 1))[None]

    sortie = np.squeeze(
        _session.run(None, {_nom_entree: blob})[0]
    )

    confiances = sortie[_LIGNE_FEU, :]
    masque = confiances >= SEUIL_CONFIANCE
    if not np.any(masque):
        return []

    confiances = confiances[masque]
    brutes = sortie[:4, masque]
    sx, sy = w / TAILLE_INFERENCE, h / TAILLE_INFERENCE
    cx, cy, bw, bh = brutes[0], brutes[1], brutes[2], brutes[3]
    xyxy = np.stack([
        (cx - bw / 2) * sx, (cy - bh / 2) * sy,
        (cx + bw / 2) * sx, (cy + bh / 2) * sy
    ], axis=1)

    detections = []
    for i in _nms(xyxy, confiances, SEUIL_NMS):
        x1, y1, x2, y2 = xyxy[i]
        detections.append((
            max(int(x1), 0),  max(int(y1), 0),
            min(int(x2), w),  min(int(y2), h),
            float(confiances[i])
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


# ======================== Lignes de route (Canny + Hough) ========================

def detecter_lignes(frame):
    """Retourne (detecte, ecart_px).
    ecart_px positif = decale a droite, negatif = a gauche."""
    h, w = frame.shape[:2]
    roi   = frame[int(h * ROI_HAUT_LIGNES):, :]
    petit = cv2.resize(roi, (0, 0),
                       fx=REDUCTION_LIGNES, fy=REDUCTION_LIGNES)
    gris  = cv2.cvtColor(petit, cv2.COLOR_BGR2GRAY)
    gris  = cv2.GaussianBlur(gris, (5, 5), 0)
    bords = cv2.Canny(gris, 50, 150)

    lignes = cv2.HoughLinesP(
        bords, 1, np.pi / 180,
        threshold=40, minLineLength=30, maxLineGap=20
    )
    if lignes is None:
        return False, 0

    lp = petit.shape[1]
    x_gauche, x_droite = [], []

    lignes = lignes.reshape(-1, 4)
    for seg in lignes:
        x1, y1, x2, y2 = seg
        if x2 - x1 == 0:
            continue
        pente = (y2 - y1) / (x2 - x1)
        if abs(pente) < 0.3:
            continue
        xb = x1 if y1 > y2 else x2
        (x_gauche if pente < 0 else x_droite).append(xb)

    if not x_gauche and not x_droite:
        return False, 0

    centre = (
        (np.mean(x_gauche) if x_gauche else 0) +
        (np.mean(x_droite) if x_droite else lp)
    ) / 2
    return True, int((centre - lp / 2) / REDUCTION_LIGNES)