/***************************************************************
 *  VisageTankETS.h
 *  TankETS  -  Visage Kawaii  -  ILI9341 320x240  -  UNO Q
 *  Librairies : Adafruit_GFX + Adafruit_ILI9341 (SPI materiel)
 *
 *  CABLAGE :
 *     DC  -> 9      CS  -> 10     RST -> 8 (a ajuster !)
 *     SCK -> 13     MOSI-> 11     MISO-> 12   (SPI materiel)
 *
 *  CONVENTIONS (TankETS) :
 *     Fonctions  : Module_PascalCase()   ex: Visage_Afficher()
 *     Variables  : camelCase             ex: expressionActuelle
 *     Constantes : MAJUSCULES_SNAKE      ex: OEIL_RAYON
 *
 *  NOTE police : police ASCII -> pas d'accents a l'ecran ("ETS").
 ***************************************************************/

#ifndef VISAGE_TANKETS_H
#define VISAGE_TANKETS_H

#include <Arduino.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// -------- Broches --------
#define PIN_TFT_DC   9
#define PIN_TFT_CS   10
#define PIN_TFT_RST  8        // <-- METS ICI la broche ou RST est branche

// -------- Infos firmware --------
#define VERSION_FIRMWARE  "1.0.0"
#define NOM_PROJET        "TankETS"
#define INFO_BUILD        __DATE__

// -------- Ecran --------
#define ECRAN_LARGEUR  320
#define ECRAN_HAUTEUR  240

// -------- Couleurs (RGB565) --------
#define COULEUR565(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))

// -------- Geometrie du visage --------
#define OEIL_Y        100
#define OEIL_GAUCHE_X 110
#define OEIL_DROITE_X 210
#define OEIL_RAYON     42
#define JOUE_Y        150
#define JOUE_GAUCHE_X  72
#define JOUE_DROITE_X 248
#define BOUCHE_X      160
#define BOUCHE_Y      182
#define BOUCHE_LARGEUR 34

// -------- Expressions --------
enum Expression { DEFAUT, CONTENT, TRISTE, PERPLEXE, RASSURE, COLERE,
                  ETOURDI, CLIN_OEIL, AMOUREUX, SURPRIS, ENDORMI, EXCITE };

// -------- Variables globales (definies dans VisageTankETS.cpp) --------
extern Adafruit_ILI9341 ecran;
extern Expression expressionActuelle;
extern int decalageX;
extern int decalageY;

extern const uint16_t COULEUR_FOND;
extern const uint16_t COULEUR_TRAIT;
extern const uint16_t COULEUR_JOUE;
extern const uint16_t COULEUR_BOUCHE;
extern const uint16_t COULEUR_BLANC;
extern const uint16_t COULEUR_ETINCELLE;
extern const uint16_t COULEUR_LARME;
extern const uint16_t COULEUR_COEUR;
extern const uint16_t COULEUR_COLERE;
extern const uint16_t COULEUR_ETS_ROUGE;

// =====================================================================
//  TRACE : primitives bas niveau (appliquent le decalage global)
// =====================================================================
void Trace_Cercle(int x,int y,int r,uint16_t c);
void Trace_CercleVide(int x,int y,int r,uint16_t c);
void Trace_Rect(int x,int y,int w,int h,uint16_t c);
void Trace_Ligne(int x,int y,int x2,int y2,uint16_t c);
void Trace_Triangle(int x,int y,int x2,int y2,int x3,int y3,uint16_t c);
void Trace_RectArrondi(int x,int y,int w,int h,int r,uint16_t c);
void Trace_Courbe(int x0,int y0,int xc,int yc,int x1,int y1,int epaisseur,uint16_t couleur);

// =====================================================================
//  DESSIN : petits elements
// =====================================================================
void Dessin_Etincelle(int x,int y,int taille,uint16_t couleur);
void Dessin_Coeur(int x,int y,int taille,uint16_t couleur);
void Dessin_Larme(int x,int y,uint16_t couleur);
void Dessin_MarqueColere(int x,int y);
void Dessin_Joues();

// =====================================================================
//  OEIL : styles d'yeux
// =====================================================================
void Oeil_Brillant(int cx,int cy,int r);
void Oeil_Large(int cx,int cy,int r);
void Oeil_ArcContent(int cx,int cy,int r);
void Oeil_ArcClin(int cx,int cy,int r);
void Oeil_MiClos(int cx,int cy,int r);
void Oeil_Spirale(int cx,int cy,int r);
void Oeil_Croix(int cx,int cy,int r);

// =====================================================================
//  BOUCHE
// =====================================================================
void Bouche_Sourire(int cx,int cy,int w);
void Bouche_Triste(int cx,int cy,int w);
void Bouche_Plate(int cx,int cy,int w);
void Bouche_Ronde(int cx,int cy,int r);
void Bouche_GrandSourire(int cx,int cy,int w);
void Bouche_Ondulee(int cx,int cy,int w);
void Bouche_Baillement(int cx,int cy);

// =====================================================================
//  VISAGE : affichage + animations
// =====================================================================
void Visage_Afficher(Expression e);
void Visage_Cligner();
void Visage_Trembler(Expression e,int nbCoups);
void Visage_Rebondir(Expression e,int nbCoups);
void Visage_PulserEtincelles(int nbFois);
void Visage_Animer(Expression e);

// =====================================================================
//  TEXTE / BOOT
// =====================================================================
void Texte_Centrer(const char* texte,int y,int taille,uint16_t couleur);
void Boot_Ecran();

// =====================================================================
//  SYSTEME : reveil / sommeil
// =====================================================================
void Systeme_Reveil();
void Systeme_Sommeil();

// =====================================================================
//  MEDIA : images / GIF / video
// =====================================================================
void Media_AfficherBitmap(const uint16_t* image,int w,int h,int x,int y);
void Media_JouerFrames(const uint16_t* const frames[],int nbFrames,int w,int h,
                       int x,int y,int delaiMs,int nbBoucles);
void Media_DemoVideo();

#endif // VISAGE_TANKETS_H
