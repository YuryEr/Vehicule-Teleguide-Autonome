/***************************************************************
 *  VisageTankETS.cpp
 *  Implementation des fonctions declarees dans VisageTankETS.h
 ***************************************************************/

#include "VisageTankETS.h"

// -------- Objet ecran --------
Adafruit_ILI9341 ecran = Adafruit_ILI9341(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

// -------- Couleurs (RGB565) --------
const uint16_t COULEUR_FOND      = COULEUR565(255,240,228);
const uint16_t COULEUR_TRAIT     = COULEUR565( 45, 42, 78);
const uint16_t COULEUR_JOUE      = COULEUR565(255,150,160);
const uint16_t COULEUR_BOUCHE    = COULEUR565(210, 70, 90);
const uint16_t COULEUR_BLANC     = 0xFFFF;
const uint16_t COULEUR_ETINCELLE = COULEUR565(255,214, 90);
const uint16_t COULEUR_LARME     = COULEUR565(120,200,245);
const uint16_t COULEUR_COEUR     = COULEUR565(255, 90,120);
const uint16_t COULEUR_COLERE    = COULEUR565(230, 40, 50);
const uint16_t COULEUR_ETS_ROUGE = COULEUR565(225, 30, 45);

// -------- Expressions / decalage global --------
Expression expressionActuelle = DEFAUT;
int decalageX = 0;
int decalageY = 0;

// =====================================================================
//  TRACE : primitives bas niveau (appliquent le decalage global)
// =====================================================================
void Trace_Cercle(int x,int y,int r,uint16_t c){ ecran.fillCircle(x+decalageX,y+decalageY,r,c); }
void Trace_CercleVide(int x,int y,int r,uint16_t c){ ecran.drawCircle(x+decalageX,y+decalageY,r,c); }
void Trace_Rect(int x,int y,int w,int h,uint16_t c){ ecran.fillRect(x+decalageX,y+decalageY,w,h,c); }
void Trace_Ligne(int x,int y,int x2,int y2,uint16_t c){ ecran.drawLine(x+decalageX,y+decalageY,x2+decalageX,y2+decalageY,c); }
void Trace_Triangle(int x,int y,int x2,int y2,int x3,int y3,uint16_t c){ ecran.fillTriangle(x+decalageX,y+decalageY,x2+decalageX,y2+decalageY,x3+decalageX,y3+decalageY,c); }
void Trace_RectArrondi(int x,int y,int w,int h,int r,uint16_t c){ ecran.fillRoundRect(x+decalageX,y+decalageY,w,h,r,c); }

// Courbe epaisse (Bezier quadratique) : sourires, sourcils, yeux fermes
void Trace_Courbe(int x0,int y0,int xc,int yc,int x1,int y1,int epaisseur,uint16_t couleur){
  const int nbPas=22;
  for(int i=0;i<=nbPas;i++){
    float t=(float)i/nbPas, u=1.0f-t;
    int x=(int)(u*u*x0 + 2*u*t*xc + t*t*x1);
    int y=(int)(u*u*y0 + 2*u*t*yc + t*t*y1);
    Trace_Cercle(x,y,epaisseur,couleur);
  }
}

// =====================================================================
//  DESSIN : petits elements
// =====================================================================
void Dessin_Etincelle(int x,int y,int taille,uint16_t couleur){
  Trace_Triangle(x,y-taille, x-taille/3,y, x+taille/3,y, couleur);
  Trace_Triangle(x,y+taille, x-taille/3,y, x+taille/3,y, couleur);
  Trace_Triangle(x-taille,y, x,y-taille/3, x,y+taille/3, couleur);
  Trace_Triangle(x+taille,y, x,y-taille/3, x,y+taille/3, couleur);
}
void Dessin_Coeur(int x,int y,int taille,uint16_t couleur){
  Trace_Cercle(x-taille/2,y-taille/3,taille/2,couleur);
  Trace_Cercle(x+taille/2,y-taille/3,taille/2,couleur);
  Trace_Triangle(x-taille,y-taille/4, x+taille,y-taille/4, x,y+taille, couleur);
}
void Dessin_Larme(int x,int y,uint16_t couleur){
  Trace_Cercle(x,y,6,couleur);
  Trace_Triangle(x-6,y-2, x+6,y-2, x,y-16, couleur);
  Trace_Cercle(x-2,y-2,2,COULEUR_BLANC);
}
void Dessin_MarqueColere(int x,int y){
  for(int o=0;o<2;o++){
    Trace_Ligne(x+o,y, x+9,y-7, COULEUR_COLERE); Trace_Ligne(x+o,y, x+9,y+7, COULEUR_COLERE);
    Trace_Ligne(x+o,y, x-9,y-7, COULEUR_COLERE); Trace_Ligne(x+o,y, x-9,y+7, COULEUR_COLERE);
  }
  Trace_Cercle(x,y,2,COULEUR_COLERE);
}
void Dessin_Joues(){
  Trace_Cercle(JOUE_GAUCHE_X,JOUE_Y,15,COULEUR_JOUE);
  Trace_Cercle(JOUE_DROITE_X,JOUE_Y,15,COULEUR_JOUE);
}

// =====================================================================
//  OEIL : styles d'yeux
// =====================================================================
void Oeil_Brillant(int cx,int cy,int r){
  Trace_Cercle(cx,cy,r,COULEUR_TRAIT);
  Trace_Cercle(cx-r/3,cy-r/3,r/3,COULEUR_BLANC);
  Trace_Cercle(cx+r/4,cy+r/4,r/6,COULEUR_BLANC);
}
void Oeil_Large(int cx,int cy,int r){
  Trace_Cercle(cx,cy,r,COULEUR_BLANC);
  Trace_CercleVide(cx,cy,r,COULEUR_TRAIT);
  Trace_Cercle(cx,cy,r/2,COULEUR_TRAIT);
  Trace_Cercle(cx-r/6,cy-r/6,r/8,COULEUR_BLANC);
}
void Oeil_ArcContent(int cx,int cy,int r){
  Trace_Courbe(cx-r,cy+r/3, cx,cy-r/2, cx+r,cy+r/3, 4, COULEUR_TRAIT);
}
void Oeil_ArcClin(int cx,int cy,int r){
  Trace_Courbe(cx-r,cy-r/3, cx,cy+r/2, cx+r,cy-r/3, 4, COULEUR_TRAIT);
}
void Oeil_MiClos(int cx,int cy,int r){
  Oeil_Brillant(cx,cy,r);
  Trace_Rect(cx-r-2,cy-r-2,2*r+4,(int)(r*1.25f),COULEUR_FOND);
  Trace_Courbe(cx-r,cy, cx,cy+5, cx+r,cy, 3, COULEUR_TRAIT);
}
void Oeil_Spirale(int cx,int cy,int r){
  float a=0; int px=cx,py=cy;
  for(int i=1;i<=42;i++){ a+=0.5f; float rr=(float)r*i/42.0f;
    int x=cx+(int)(cos(a)*rr), y=cy+(int)(sin(a)*rr);
    Trace_Ligne(px,py,x,y,COULEUR_TRAIT); px=x; py=y; }
}
void Oeil_Croix(int cx,int cy,int r){
  for(int o=-1;o<=1;o++){
    Trace_Ligne(cx-r,cy-r+o, cx+r,cy+r+o, COULEUR_TRAIT);
    Trace_Ligne(cx-r,cy+r+o, cx+r,cy-r+o, COULEUR_TRAIT);
  }
}

// =====================================================================
//  BOUCHE
// =====================================================================
void Bouche_Sourire(int cx,int cy,int w){ Trace_Courbe(cx-w,cy-4, cx,cy+18, cx+w,cy-4, 4, COULEUR_TRAIT); }
void Bouche_Triste (int cx,int cy,int w){ Trace_Courbe(cx-w,cy+12, cx,cy-12, cx+w,cy+12, 4, COULEUR_TRAIT); }
void Bouche_Plate  (int cx,int cy,int w){ Trace_Courbe(cx-w,cy, cx,cy+3, cx+w,cy, 3, COULEUR_TRAIT); }
void Bouche_Ronde  (int cx,int cy,int r){ Trace_Cercle(cx,cy,r,COULEUR_TRAIT); Trace_Cercle(cx,cy,r-3,COULEUR_BOUCHE); }
void Bouche_GrandSourire(int cx,int cy,int w){
  Trace_Cercle(cx,cy-2,w,COULEUR_TRAIT);
  Trace_Rect(cx-w-1,cy-w-2,2*w+2,w,COULEUR_FOND);
  Trace_Cercle(cx,cy+w-8,w/2,COULEUR_BOUCHE);
}
void Bouche_Ondulee(int cx,int cy,int w){
  int px=cx-w,py=cy;
  for(int i=1;i<=4;i++){ int x=cx-w+(2*w*i/4); int y=cy+((i%2)?7:-7);
    Trace_Ligne(px,py,x,y,COULEUR_TRAIT); Trace_Ligne(px,py+1,x,y+1,COULEUR_TRAIT); px=x; py=y; }
}
void Bouche_Baillement(int cx,int cy){
  Trace_RectArrondi(cx-22,cy-16,44,46,18,COULEUR_TRAIT);
  Trace_RectArrondi(cx-13,cy+8,26,16,8,COULEUR_BOUCHE);
}

// =====================================================================
//  VISAGE : affichage d'une expression
// =====================================================================
void Visage_Afficher(Expression e){
  ecran.fillScreen(COULEUR_FOND);
  Dessin_Joues();
  switch(e){
    case DEFAUT:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_Brillant(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Bouche_Sourire(BOUCHE_X,BOUCHE_Y,BOUCHE_LARGEUR);
      Dessin_Etincelle(OEIL_DROITE_X+34,OEIL_Y-30,6,COULEUR_ETINCELLE);
      break;
    case CONTENT:
      Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Bouche_GrandSourire(BOUCHE_X,BOUCHE_Y,28);
      Dessin_Etincelle(40,60,7,COULEUR_ETINCELLE); Dessin_Etincelle(285,70,8,COULEUR_ETINCELLE);
      Dessin_Etincelle(60,200,6,COULEUR_ETINCELLE); Dessin_Etincelle(270,195,6,COULEUR_ETINCELLE);
      break;
    case TRISTE:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y+4,OEIL_RAYON-4); Oeil_Brillant(OEIL_DROITE_X,OEIL_Y+4,OEIL_RAYON-4);
      Trace_Courbe(OEIL_GAUCHE_X-30,OEIL_Y-46, OEIL_GAUCHE_X,OEIL_Y-40, OEIL_GAUCHE_X+26,OEIL_Y-30,3,COULEUR_TRAIT);
      Trace_Courbe(OEIL_DROITE_X-26,OEIL_Y-30, OEIL_DROITE_X,OEIL_Y-40, OEIL_DROITE_X+30,OEIL_Y-46,3,COULEUR_TRAIT);
      Bouche_Triste(BOUCHE_X,BOUCHE_Y,26);
      Dessin_Larme(OEIL_GAUCHE_X-26,OEIL_Y+34,COULEUR_LARME);
      break;
    case PERPLEXE:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON-6); Oeil_Brillant(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Trace_Courbe(OEIL_GAUCHE_X-26,OEIL_Y-40, OEIL_GAUCHE_X,OEIL_Y-50, OEIL_GAUCHE_X+26,OEIL_Y-42,3,COULEUR_TRAIT);
      Bouche_Ondulee(BOUCHE_X,BOUCHE_Y,24);
      ecran.setTextColor(COULEUR_TRAIT); ecran.setTextSize(4); ecran.setCursor(262,40); ecran.print("?");
      break;
    case RASSURE:
      Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Bouche_Sourire(BOUCHE_X,BOUCHE_Y-2,24);
      Trace_CercleVide(OEIL_DROITE_X+40,OEIL_Y-6,4,COULEUR_LARME);
      Trace_CercleVide(OEIL_DROITE_X+50,OEIL_Y-12,6,COULEUR_LARME);
      break;
    case COLERE:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y+2,OEIL_RAYON-6); Oeil_Brillant(OEIL_DROITE_X,OEIL_Y+2,OEIL_RAYON-6);
      for(int o=0;o<3;o++){
        Trace_Ligne(OEIL_GAUCHE_X-30,OEIL_Y-44+o, OEIL_GAUCHE_X+26,OEIL_Y-26+o, COULEUR_TRAIT);
        Trace_Ligne(OEIL_DROITE_X+30,OEIL_Y-44+o, OEIL_DROITE_X-26,OEIL_Y-26+o, COULEUR_TRAIT);
      }
      Bouche_Triste(BOUCHE_X,BOUCHE_Y,22);
      Dessin_MarqueColere(250,55);
      break;
    case ETOURDI:
      Oeil_Spirale(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON-4); Oeil_Spirale(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON-4);
      Bouche_Ondulee(BOUCHE_X,BOUCHE_Y,26);
      break;
    case CLIN_OEIL:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcClin(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Bouche_Sourire(BOUCHE_X,BOUCHE_Y,BOUCHE_LARGEUR);
      Dessin_Etincelle(OEIL_DROITE_X+30,OEIL_Y-28,8,COULEUR_ETINCELLE);
      break;
    case AMOUREUX:
      Dessin_Coeur(OEIL_GAUCHE_X,OEIL_Y,28,COULEUR_COEUR); Dessin_Coeur(OEIL_DROITE_X,OEIL_Y,28,COULEUR_COEUR);
      Bouche_GrandSourire(BOUCHE_X,BOUCHE_Y,26);
      Dessin_Coeur(40,70,12,COULEUR_COEUR); Dessin_Coeur(282,80,14,COULEUR_COEUR);
      break;
    case SURPRIS:
      Oeil_Large(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_Large(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Trace_Courbe(OEIL_GAUCHE_X-26,OEIL_Y-52, OEIL_GAUCHE_X,OEIL_Y-58, OEIL_GAUCHE_X+26,OEIL_Y-52,3,COULEUR_TRAIT);
      Trace_Courbe(OEIL_DROITE_X-26,OEIL_Y-52, OEIL_DROITE_X,OEIL_Y-58, OEIL_DROITE_X+26,OEIL_Y-52,3,COULEUR_TRAIT);
      Bouche_Ronde(BOUCHE_X,BOUCHE_Y,14);
      Dessin_Larme(OEIL_DROITE_X+34,OEIL_Y-6,COULEUR_LARME);
      break;
    case ENDORMI:
      Oeil_MiClos(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_MiClos(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
      Bouche_Plate(BOUCHE_X,BOUCHE_Y,16);
      ecran.setTextColor(COULEUR_TRAIT);
      ecran.setTextSize(2); ecran.setCursor(248,40); ecran.print("z");
      ecran.setTextSize(3); ecran.setCursor(262,20); ecran.print("Z");
      break;
    case EXCITE:
      Oeil_Brillant(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON+2); Oeil_Brillant(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON+2);
      Bouche_GrandSourire(BOUCHE_X,BOUCHE_Y,30);
      Dessin_Etincelle(45,55,9,COULEUR_ETINCELLE); Dessin_Etincelle(280,60,9,COULEUR_ETINCELLE);
      Dessin_Etincelle(150,40,7,COULEUR_ETINCELLE);
      break;
  }
  expressionActuelle = e;
}

// =====================================================================
//  VISAGE : animations
// =====================================================================
void Visage_Cligner(){
  Trace_Rect(OEIL_GAUCHE_X-OEIL_RAYON-2,OEIL_Y-OEIL_RAYON-2,2*OEIL_RAYON+4,2*OEIL_RAYON+4,COULEUR_FOND);
  Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON);
  Trace_Rect(OEIL_DROITE_X-OEIL_RAYON-2,OEIL_Y-OEIL_RAYON-2,2*OEIL_RAYON+4,2*OEIL_RAYON+4,COULEUR_FOND);
  Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
  delay(110);
  Visage_Afficher(expressionActuelle);
}
void Visage_Trembler(Expression e,int nbCoups){
  for(int i=0;i<nbCoups;i++){ decalageX=(i%2)?6:-6; Visage_Afficher(e); delay(45); }
  decalageX=0; Visage_Afficher(e);
}
void Visage_Rebondir(Expression e,int nbCoups){
  for(int i=0;i<nbCoups;i++){ decalageY=(i%2)?-10:0; Visage_Afficher(e); delay(70); }
  decalageY=0; Visage_Afficher(e);
}
void Visage_PulserEtincelles(int nbFois){
  const int posX[4]={45,280,60,270}, posY[4]={60,68,200,196};
  for(int t=0;t<nbFois;t++){
    for(int i=0;i<4;i++) Dessin_Etincelle(posX[i],posY[i],8,COULEUR_ETINCELLE);
    delay(160);
    for(int i=0;i<4;i++) Dessin_Etincelle(posX[i],posY[i],10,COULEUR_FOND);
    delay(120);
  }
  Visage_Afficher(expressionActuelle);
}
void Visage_Animer(Expression e){
  Visage_Afficher(e);
  switch(e){
    case CONTENT: case EXCITE:  Visage_PulserEtincelles(2); break;
    case AMOUREUX:              Visage_PulserEtincelles(2); break;
    case COLERE:                Visage_Trembler(e,6); break;
    case ETOURDI:               Visage_Trembler(e,4); break;
    case SURPRIS:               Visage_Rebondir(e,3); break;
    default:                    Visage_Cligner(); break;
  }
}

// =====================================================================
//  TEXTE / BOOT
// =====================================================================
void Texte_Centrer(const char* texte,int y,int taille,uint16_t couleur){
  ecran.setTextSize(taille); ecran.setTextColor(couleur);
  int16_t x1,y1; uint16_t w,h;
  ecran.getTextBounds(texte,0,y,&x1,&y1,&w,&h);
  ecran.setCursor((ECRAN_LARGEUR-(int)w)/2,y);
  ecran.print(texte);
}
void Boot_Ecran(){
  ecran.fillScreen(COULEUR_BLANC);
  ecran.fillRect(0,0,ECRAN_LARGEUR,70,COULEUR_ETS_ROUGE);
  Texte_Centrer("ETS",14,6,COULEUR_BLANC);
  Texte_Centrer("Ecole de technologie superieure",80,1,COULEUR_TRAIT);
  Texte_Centrer(NOM_PROJET,104,2,COULEUR_TRAIT);

  ecran.setTextSize(1); ecran.setTextColor(COULEUR565(90,90,110));
  ecran.setCursor(20,134); ecran.print("Firmware : v"); ecran.print(VERSION_FIRMWARE);
  ecran.setCursor(20,148); ecran.print("Build    : "); ecran.print(INFO_BUILD);
  ecran.setCursor(20,162); ecran.print("MCU      : STM32U585 (UNO Q)");
  ecran.setCursor(20,176); ecran.print("Ecran    : ILI9341 320x240");

  int barreX=40,barreY=205,barreL=240,barreH=16;
  ecran.drawRoundRect(barreX,barreY,barreL,barreH,5,COULEUR_TRAIT);
  const char* etapes[]={"Init ecran...","Init capteurs...","Init moteurs...","Pret !"};
  for(int p=0;p<=100;p+=4){
    ecran.fillRoundRect(barreX+2,barreY+2,(barreL-4)*p/100,barreH-4,3,COULEUR_ETS_ROUGE);
    if(p%25==0){
      ecran.fillRect(0,224,ECRAN_LARGEUR,16,COULEUR_BLANC);
      Texte_Centrer(etapes[p/25>3?3:p/25],226,1,COULEUR_TRAIT);
    }
    delay(28);
  }
  delay(500);
}

// =====================================================================
//  SYSTEME : reveil / sommeil
// =====================================================================
void Systeme_Reveil(){
  ecran.fillScreen(COULEUR_FOND); Dessin_Joues();
  Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
  Bouche_Plate(BOUCHE_X,BOUCHE_Y,16);
  ecran.setTextColor(COULEUR_TRAIT); ecran.setTextSize(2);
  ecran.setCursor(250,40); ecran.print("z"); ecran.setCursor(266,22); ecran.print("Z");
  delay(800);

  for(int k=0;k<2;k++){
    ecran.fillScreen(COULEUR_FOND); Dessin_Joues();
    Oeil_MiClos(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_MiClos(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
    Bouche_Plate(BOUCHE_X,BOUCHE_Y,16); delay(180);
    ecran.fillScreen(COULEUR_FOND); Dessin_Joues();
    Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
    Bouche_Plate(BOUCHE_X,BOUCHE_Y,14); delay(150);
  }
  Visage_Afficher(EXCITE);
  Visage_Rebondir(EXCITE,4);
  Visage_PulserEtincelles(3);
  delay(400);
  Visage_Animer(DEFAUT);
}
void Systeme_Sommeil(){
  ecran.fillScreen(COULEUR_FOND); Dessin_Joues();
  Oeil_MiClos(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_MiClos(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
  Bouche_Baillement(BOUCHE_X,BOUCHE_Y);
  delay(900);
  ecran.fillScreen(COULEUR_FOND); Dessin_Joues();
  Oeil_ArcContent(OEIL_GAUCHE_X,OEIL_Y,OEIL_RAYON); Oeil_ArcContent(OEIL_DROITE_X,OEIL_Y,OEIL_RAYON);
  Bouche_Plate(BOUCHE_X,BOUCHE_Y,16);
  ecran.setTextColor(COULEUR_TRAIT);
  int zx[3]={250,268,286}, zy[3]={60,40,18}, zt[3]={2,3,4};
  for(int i=0;i<3;i++){ ecran.setTextSize(zt[i]); ecran.setCursor(zx[i],zy[i]); ecran.print("Z"); delay(450); }
  delay(700);
  ecran.fillScreen(COULEUR565(20,22,40));
  Texte_Centrer("...zzz...",120,2,COULEUR565(90,95,130));
}

// =====================================================================
//  MEDIA : images / GIF / video
// =====================================================================
void Media_AfficherBitmap(const uint16_t* image,int w,int h,int x,int y){
  ecran.drawRGBBitmap(x,y,image,w,h);
}
void Media_JouerFrames(const uint16_t* const frames[],int nbFrames,int w,int h,
                       int x,int y,int delaiMs,int nbBoucles){
  for(int b=0;b<nbBoucles;b++)
    for(int f=0;f<nbFrames;f++){ ecran.drawRGBBitmap(x,y,frames[f],w,h); delay(delaiMs); }
}
// DEMO executable : une "video" generee a la volee (balle qui rebondit)
void Media_DemoVideo(){
  ecran.fillScreen(COULEUR_TRAIT);
  const int W=96,H=96; static uint16_t tampon[96*96];
  int bx=10,by=10,vx=5,vy=4,rayon=12;
  for(int frame=0; frame<70; frame++){
    for(int i=0;i<W*H;i++) tampon[i]=COULEUR565(20,22,40);
    bx+=vx; by+=vy;
    if(bx<rayon||bx>W-rayon) vx=-vx;
    if(by<rayon||by>H-rayon) vy=-vy;
    for(int yy=-rayon;yy<=rayon;yy++)
      for(int xx=-rayon;xx<=rayon;xx++)
        if(xx*xx+yy*yy<=rayon*rayon){
          int px=bx+xx, py=by+yy;
          if(px>=0&&px<W&&py>=0&&py<H) tampon[py*W+px]=COULEUR_ETINCELLE;
        }
    ecran.drawRGBBitmap((ECRAN_LARGEUR-W)/2,(ECRAN_HAUTEUR-H)/2,tampon,W,H);
    delay(20);
  }
}
// Emplacement pour le vrai logo ETS (tableau RGB565 genere par image2cpp) :
// const uint16_t LOGO_ETS[64*64] PROGMEM = { ... };
// void Media_AfficherLogo(){ Media_AfficherBitmap(LOGO_ETS,64,64,(ECRAN_LARGEUR-64)/2,90); }
