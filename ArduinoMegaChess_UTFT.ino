//ArduinoMega Chess 1.0 (UTFT 240x320 try version)
//Sergey Urusov, ususovsv@gmail.com

#include <UTFTGLUE.h>
#include <stdint.h>
#include "TouchScreen.h"
#include <avr/pgmspace.h>
#include <EEPROM.h> 


uint16_t BLACK,BLUE,RED,GREEN,CYAN,MAGENTA,YELLOW,WHITE,GRAY,DARK,GRAY2,BLACKF,WHITEF;

//Adafruit_TFTLCD tft;
//#define YP A1  // must be an analog pin, use "An" notation!
//#define XM A2  // must be an analog pin, use "An" notation!
//#define YM 7   // can be a digital pin
//#define XP 6   // can be a digital pin

MCUFRIEND_kbv tft;
int XP = 6, YP = A1, XM = A2, YM = 7;  

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 350);
int touchx,touchy,touchx0,touchy0,ntouch;         
unsigned long lasttouch;
#define MINPRESSURE 10
#define MAXPRESSURE 1000
const int PinBuzz = 44;
int cycle=0; //счетчик таймера
const char fp=1;
const char fn=2;
const char fb=3;
const char fr=4;
const char fq=5;
const char fk=6;
const int fig_weight[]={0,100,320,330,500,900,0};
const char fig_symb[]="  NBRQK";
unsigned long count;
boolean rotate=false;
const char polezero[8][8] PROGMEM={
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             };  
const char polestart[8][8] PROGMEM={
             {-fr,-fn,-fb,-fq,-fk,-fb,-fn,-fr},
             {-fp,-fp,-fp,-fp,-fp,-fp,-fp,-fp},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             { fp, fp, fp, fp, fp, fp, fp, fp},
             { fr, fn, fb, fq, fk, fb, fn, fr},
             };  
             
char pole[8][8]={ //мат в 3х белые
             {-fr,-fn,-fb,-fq,-fk,-fb,-fn,-fr},
             {-fp,-fp,-fp,-fp,-fp,-fp,-fp,-fp},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             {  0,  0,  0,  0,  0,  0,  0,  0},
             { fp, fp, fp, fp, fp, fp, fp, fp},
             { fr, fn, fb, fq, fk, fb, fn, fr},
             };  
             
char pole0[8][8];  //позиция до обсчета хода
char poledisp[8][8]; //отображенная позиция
char polechoice[7]; //выбор фигур
boolean w00,w000,b00,b000;             
char blinkstep;
typedef struct {
  char fig1, fig2; //фигура ходящая и фигура побитая
  char x1,y1,x2,y2; //исходные и последние координаты  
  char check; //шах после хода
  char type; //тип хода 1-взятие на проходе,2-короткая рокировка,3-длинная рокировка,4-5-6-7-превращение в коня,слона,ладью,ферзя
  int weight; //вес ситуации, вспомогательное
} step_type;
const int MAXSTEPS=500; //макс.количество полуходов для обработки включая дерево вариантов
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int MINDEPTH; //макс.количество полностью рассматриваемых полуходов
int MAXDEPTH; //макс.количество рассматриваемых активных полуходов
int LIMDEPTH; //макс.текущее количество рассматриваемых активных полуходов с учетом продлений
const char MAXCUTS=10; //макс.кол.ходов бета-отсечений и альфа-превышений (10- чаще выстреливает, чем 20)
boolean TRACE=0;
boolean checkmatesearch=0; //поиск мата в позиции
boolean always=true; int alwaysDelayMax = 500; int alwaysDelay = 0;
boolean solving=false;
boolean choice=false;
boolean sound=true;
int cur_step=1; //номер текущего полухода, 1....
int limit=0; //предел времени размышления, 0-6;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const long limits[7]={10,30,60,180,600,1800,100000};
//                     0  1  2   3   4    5      6 
step_type steps[MAXSTEPS]; //массив сделанных ходов
step_type cuts[MAXCUTS]; //массив ходов бета-отсечек
step_type lastbest;
int lastscore;
int minbeta,maxalpha;
int startweight;
int cur_level; //текущий уровень алгоритма (веерно)
int start_var; //начальный номер текущего варианта, 1....
int cur_var; //номер текущего варианта, 1....
int cur_choice; //номер текущего выбора фигуры
boolean check_on_table; //шах на доске
boolean isstatus;
char WKJ=0,WKI=0,BKJ=0,BKI=0; //положения королей
boolean endspiel=false; //эндшпиль 
char progress; //процент анализа ходов 0-100
boolean only_action=false; //только ходы-бои и шахи
unsigned long starttime,limittime,quitime=0;             
boolean drag;
const uint8_t fig[6][32] PROGMEM={
{0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x3,  0xC0, 0x7,  0xE0, 0x7,  0xE0, 0x3,  0xC0, 0x3,  0xC0,
0x7,  0xE0, 0x3,  0xC0, 0x3,  0xC0, 0x7,  0xE0, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0x0,  0x0}, //пешка
{0x0, 0x0,  0x3,  0x40, 0x7,  0xE0, 0xF,  0xF0, 0x1F, 0xF8, 0x3F, 0xFC, 0x39, 0xFC, 0x33, 0xFC,
0x7,  0xFC, 0xF,  0xF8, 0xF,  0xF0, 0x7,  0xE0, 0x3,  0xC0, 0x7,  0xE0, 0x1F, 0xF8, 0x0,  0x0},  //конь
{0x1, 0x80, 0x3,  0xC0, 0x1,  0x80, 0xF,  0xF0, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
0x1F, 0xF8, 0x1F, 0xF8, 0xF,  0xF0, 0x7,  0xE0, 0x3,  0xC0, 0x31, 0x8C, 0x7F, 0xFE, 0x0,  0x0}, //слон 
{0x0, 0x0,  0x19, 0x98, 0x1F, 0xF8, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0,
0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0xF,  0xF0, 0x1F, 0xF8, 0x1F, 0xF8, 0x0,  0x0}, //ладья
{0x19, 0x98, 0xD9, 0x9B, 0xD9, 0x9B, 0xD9, 0x9B, 0x6D, 0xB6, 0x6D, 0xB6, 0x6D, 0xB6, 0x35, 0xAC,
0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x1F, 0xF8, 0xF,  0xF0, 0xF,  0xF0, 0x1F, 0xF8, 0x0,  0x0}, //ферзь
{0x1, 0x80, 0x1,  0x80, 0x79, 0x9E, 0x7D, 0xBE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x7F, 0xFE, 0x3F, 0xFC, 0x3F, 0xFC, 0x1F, 0xF8, 0xF,  0xF0, 0xF,  0xF0, 0x1F, 0xF8, 0x0,  0x0}  //король
};
const uint8_t fig_cont[6][32] PROGMEM={
{0x0, 0x0,  0x0,  0x0,  0x3,  0xC0, 0x4,  0x20, 0x8,  0x10, 0x8,  0x10, 0x4,  0x20, 0x4,  0x20,
0x8,  0x10, 0x4,  0x20, 0x4,  0x20, 0x8,  0x10, 0x10, 0x8,  0x10, 0x8,  0x10, 0x8,  0x1F, 0xF8}, //пешка
{0x3, 0x40, 0x4,  0xA0, 0x8,  0x10, 0x12, 0x8,  0x20, 0x4,  0x40, 0x2,  0x46, 0x2,  0x4C, 0x2,
0x38, 0x2,  0x10, 0x4,  0x10, 0x8,  0x8,  0x10, 0x4,  0x20, 0x18, 0x18, 0x20, 0x4,  0x3F, 0xFC},  //конь
{0x2, 0x40, 0x4,  0x20, 0x6,  0x60, 0x10, 0x8,  0x21, 0x84, 0x21, 0x84, 0x21, 0x84, 0x27, 0xE4,
0x21, 0x84, 0x21, 0x84, 0x11, 0x88, 0x8,  0x10, 0x34, 0x2C, 0x4E, 0x72, 0x80, 0x1,  0xFF, 0xFF}, //слон 
{0x39, 0x9C, 0x26, 0x64, 0x20, 0x4,  0x1F, 0xF8, 0x10, 0x8,  0x10, 0x8,  0x10, 0x8,  0x10, 0x8,
0x10, 0x8,  0x10, 0x8,  0x10, 0x8,  0x10, 0x8,  0x1F, 0xF8, 0x20, 0x4,  0x20, 0x4,  0x3F, 0xFC}, //ладья
{0x22, 0x24, 0x22, 0x24, 0x22, 0x24, 0x26, 0x64, 0x92, 0x49, 0x92, 0x49, 0x92, 0x49, 0x4A, 0x52,
0x40, 0x2,  0x40, 0x2,  0x5F, 0xFA, 0x20, 0x4,  0x1F, 0xF8, 0x10, 0x8,  0x20, 0x4,  0x3F, 0xFC}, //ферзь
{0x2, 0x40, 0x7A, 0x5E, 0x86, 0x61, 0x82, 0x41, 0x2,  0x40, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80,
0x80, 0x81, 0x41, 0x82, 0x5F, 0xFA, 0x20, 0x4,  0x1F, 0xF8, 0x10, 0x8,  0x20, 0x4,  0x3F, 0xFC}  //король
};

const uint8_t issound[2][32] PROGMEM={
{0x0, 0x0,  0x0,  0x0,  0x0,  0x32, 0x0,  0x54, 0x0,  0x98, 0x1F, 0x10, 0x10, 0x30, 0x10, 0x50,
0x10, 0x90, 0x11, 0x10, 0x1F, 0x10, 0x4,  0x90, 0x8,  0x50, 0x10, 0x30, 0x0,  0x0,  0x0,  0x0},
{0x0, 0x0,  0x0,  0x0,  0x0,  0x30, 0x0,  0x50, 0x0,  0x90, 0x1F, 0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x10, 0x10, 0x10, 0x1F, 0x10, 0x0,  0x90, 0x0,  0x50, 0x0,  0x30, 0x0,  0x0,  0x0,  0x0}
};
const uint8_t iswb[2][32] PROGMEM={
{0x0,  0x0,  0x0,  0x0,  0x3,  0xC0, 0x4,  0x20, 0x8,  0x10, 0x8,  0x10, 0x4,  0x20, 0x4,  0x20,
0x8,  0x10, 0x4,  0x20, 0x4,  0x20, 0x8,  0x10, 0x10, 0x8,  0x10, 0x8,  0x1F, 0xF8, 0x0,  0x0},
{0x0,  0x0,  0x0,  0x0,  0x3,  0xC0, 0x7,  0xE0, 0xF,  0xF0, 0xF,  0xF0, 0x7,  0xE0, 0x7,  0xE0,
0xF,  0xF0, 0x7,  0xE0, 0x7,  0xE0, 0xF,  0xF0, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x0,  0x0}
};
const uint8_t iscm[2][32] PROGMEM={
{0x00, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xff, 0x00, 0xff, 0x80, 0xff, 0xe0, 0xff, 0xf0, 
0xff, 0xf0, 0xff, 0xe0, 0xff, 0x80, 0xff, 0x00, 0xfc, 0x00, 0xf0, 0x00, 0xe0, 0x00, 0x00, 0x00},
{0x00, 0x00, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 
0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0x00, 0x00}
};
/*{0x0, 0x0,  0x7,  0xE0, 0xC,  0x30, 0xC,  0x30, 0xC,  0x30, 0xC,  0x30, 0x0,  0x30, 0x0,  0x60,
0x0,  0xC0, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x0,  0x0,  0x1,  0x80, 0x1,  0x80, 0x0,  0x0},
{0x0,  0x0,  0x3F, 0x0,  0x61, 0x80, 0x61, 0x80, 0x61, 0x80, 0x61, 0x80, 0x1,  0x80, 0x3,  0x0,
0x6,  0x24, 0xC,  0x24, 0xC,  0xFF, 0xC,  0x24, 0x0,  0xFF, 0xC,  0x24, 0xC,  0x24, 0x0,  0x0}
}*/
const uint8_t islimit[7][32] PROGMEM={
{0x0, 0x0,  0x4,  0x70, 0xC,  0x88, 0x4,  0x88, 0x4,  0x88, 0x4,  0x88, 0x4,  0x88, 0x4,  0x88,
0xE,  0x70, 0x0,  0x0,  0x1D, 0xDC, 0x11, 0x10, 0x1D, 0xD0, 0x5,  0x10, 0x1D, 0xDC, 0x0,  0x0},
{0x0, 0x0,  0xC,  0x70, 0x12, 0x88, 0x2,  0x88, 0xC,  0x88, 0x2,  0x88, 0x2,  0x88, 0x12, 0x88,
0xC,  0x70, 0x0,  0x0,  0x1D, 0xDC, 0x11, 0x10, 0x1D, 0xD0, 0x5,  0x10, 0x1D, 0xDC, 0x0,  0x0},
{0x0, 0x0,  0x1,  0x80, 0x3,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80,
0x3,  0xC0, 0x0,  0x0,  0x22, 0xA4, 0x36, 0xA4, 0x2A, 0xB4, 0x22, 0xAC, 0x22, 0xA4, 0x0,  0x0},
{0x0, 0x0,  0x3,  0xC0, 0x6,  0x60, 0x0,  0x60, 0x1,  0xC0, 0x0,  0x60, 0x6,  0x60, 0x6,  0x60,
0x3,  0xC0, 0x0,  0x0,  0x22, 0xA4, 0x36, 0xA4, 0x2A, 0xB4, 0x22, 0xAC, 0x22, 0xA4, 0x0,  0x0},
{0x0, 0x0,  0xC,  0x78, 0x1C, 0xCC, 0xC,  0xCC, 0xC,  0xCC, 0xC,  0xCC, 0xC,  0xCC, 0xC,  0xCC,
0x1E, 0x78, 0x0,  0x0,  0x22, 0xA4, 0x36, 0xA4, 0x2A, 0xB4, 0x22, 0xAC, 0x22, 0xA4, 0x0,  0x0},
{0x0, 0x0,  0x1C, 0x78, 0x36, 0xCC, 0x6,  0xCC, 0x1C, 0xCC, 0x6,  0xCC, 0x36, 0xCC, 0x36, 0xCC,
0x1C, 0x78, 0x0,  0x0,  0x22, 0xA4, 0x36, 0xA4, 0x2A, 0xB4, 0x22, 0xAC, 0x22, 0xA4, 0x0,  0x0},
{0x0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x3C, 0x3C, 0x7E, 0x7E, 0x63, 0xC6, 0x61, 0x86,
0x63, 0xC6, 0x66, 0x66, 0x3C, 0x3C, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0}
};
const uint8_t isdesc[2][32] PROGMEM={
{0x0,  0x0,  0xE3, 0x8E, 0xE3, 0x8E, 0xE3, 0x8E, 0x1C, 0x70, 0x1C, 0x70, 0x1C, 0x70, 0xE3, 0x8E,
0xE3, 0x8E, 0xE3, 0x8E, 0x1C, 0x70, 0x1C, 0x70, 0x1C, 0x70, 0xE3, 0x8E, 0xE3, 0x8E, 0xE3, 0x8E},
{0x0,  0x0,  0x0,  0x4,  0x0,  0xE,  0x0,  0xE,  0x0,  0x1C, 0x0,  0x1C, 0x0,  0x38, 0x0,  0x30,
0x30, 0x70, 0x78, 0xE0, 0x3C, 0xC0, 0x1F, 0xC0, 0xF,  0x80, 0x7,  0x0,  0x2,  0x0,  0x0,  0x0}

};
const uint8_t isback[32] PROGMEM={
0x0,  0x0,  0x0,  0x0,  0x20, 0xF0, 0x31, 0xF8, 0x3B, 0xFC, 0x3F, 0x8E, 0x3F, 0xE,  0x3F, 0xE,
0x3F, 0x8E, 0x3F, 0xCE, 0x0,  0xE,  0x0,  0xE,  0x0,  0x1C, 0x0,  0x38, 0x0,  0x70, 0x0,  0x0
};
const uint8_t issave[32] PROGMEM={
0x1,  0x80, 0x3,  0xC0, 0x7,  0xE0, 0x7,  0xE0, 0x3,  0xC0, 0x3F, 0xFC, 0x23, 0xC4, 0x23, 0xC4,
0x23, 0xC4, 0x23, 0xC4, 0x23, 0xC4, 0x23, 0xC4, 0x20, 0x4,  0x20, 0x4,  0x3F, 0xFC, 0x0,  0x0
};
const uint8_t isload[32] PROGMEM={
0x0,  0x0,  0x3,  0xC0, 0x3,  0xC0, 0x3,  0xC0, 0x3,  0xC0, 0x3F, 0xFC, 0x23, 0xC4, 0x23, 0xC4,
0x23, 0xC4, 0x27, 0xE4, 0x27, 0xE4, 0x23, 0xC4, 0x21, 0x84, 0x20, 0x4,  0x3F, 0xFC, 0x0,  0x0
};
const uint8_t isempty[32] PROGMEM={
0xFF,  0xFF, 0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,
0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0xFF, 0xFF
};
const uint8_t isinit[32] PROGMEM={
0xFF,  0xFF, 0x80, 0x1,  0x80, 0x1,  0xFF, 0xFF, 0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,
0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0x80, 0x1,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
const uint8_t isplay[32] PROGMEM={
0x0,  0x0,  0x3,  0xC0, 0x3,  0xC0, 0x3,  0xC0, 0x1,  0x80, 0x23, 0xF8, 0x17, 0xC8, 0xB,  0xC8,
0x3,  0xC0, 0x3,  0xC0, 0x2,  0x40, 0x4,  0x40, 0x4,  0x38, 0x4,  0x8,  0xC,  0x0,  0x0,  0x0
};
const uint8_t isrotate[2][32] PROGMEM={
{0x0,  0x0,  0x0,  0x0,  0x3F, 0xFC, 0x20, 0x4,  0x20, 0x4,  0x20, 0x4,  0x20, 0x4,  0x20, 0x4,
0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x0,  0x0,  0x0,  0x0},
{0x0, 0x0,  0x0,  0x0,  0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC, 0x3F, 0xFC,
0x20, 0x4,  0x20, 0x4,  0x20, 0x4,  0x20, 0x4,  0x20, 0x4,  0x3F, 0xFC, 0x0,  0x0,  0x0,  0x0}
};

const int pos[7][8][8] PROGMEM={
 {{  0,  0,  0,  0,  0,  0,  0,  0}, //пешка 
  {100,100,100,100,100,100,100,100},  //{ 50, 50, 50, 50, 50, 50, 50, 50},
  { 20, 30, 40, 50, 50, 40, 30, 20},  //{ 10, 10, 20, 30, 30, 20, 10, 10},
  {  5,  5, 10, 25, 25, 10,  5,  5},
  {  0,  0,  0, 20, 20,  0,  0,  0},
  {  5, -5,-10,  0,  0,-10, -5,  5},
  {  5, 10, 10,-20,-20, 10, 10,  5},  //{  5, 10, 10,-20,-20, 10, 10,  5},
  {  0,  0,  0,  0,  0,  0,  0,  0}},
  
 {{-50,-40,-30,-30,-30,-30,-40,-50}, //конь
  {-40,-20,  0,  0,  0,  0,-20,-40},
  {-30,  0, 10, 15, 15, 10,  0,-30},
  {-30,  5, 15, 20, 20, 15,  5,-30},
  {-30,  0, 15, 20, 20, 15,  0,-30},
  {-30,  5, 10, 15, 15, 10,  5,-30},
  {-40,-20,  0,  5,  5,  0,-20,-40},
  {-50,-40,-30,-30,-30,-30,-40,-50}}, 
  
 {{-20,-10,-10,-10,-10,-10,-10,-20}, //слон
  {-10,  0,  0,  0,  0,  0,  0,-10},
  {-10,  0,  5, 10, 10,  5,  0,-10},
  {-10,  5,  5, 10, 10,  5,  5,-10},
  {-10,  0, 10, 10, 10, 10,  0,-10},
  {-10, 10, 10, 10, 10, 10, 10,-10},
  {-10,  5,  0,  0,  0,  0,  5,-10},
  {-20,-10,-10,-10,-10,-10,-10,-20}},  

 {{  0,  0,  0,  0,  0,  0,  0,  0}, //ладья
  {  5, 10, 10, 10, 10, 10, 10,  5},
  { -5,  0,  0,  0,  0,  0,  0, -5},
  { -5,  0,  0,  0,  0,  0,  0, -5},
  { -5,  0,  0,  0,  0,  0,  0, -5},
  { -5,  0,  0,  0,  0,  0,  0, -5},
  { -5,  0,  0,  0,  0,  0,  0, -5},
  {  0,  0,  0,  5,  5,  0,  0,  0}},
  
 {{-20,-10,-10, -5, -5,-10,-10,-20}, //ферзь
  {-10,  0,  0,  0,  0,  0,  0,-10},
  {-10,  0,  5,  5,  5,  5,  0,-10},
  { -5,  0,  5,  5,  5,  5,  0, -5},
  {  0,  0,  5,  5,  5,  5,  0, -5},
  {-10,  5,  5,  5,  5,  5,  0,-10},
  {-10,  0,  5,  0,  0,  0,  0,-10},
  {-20,-10,-10, -5, -5,-10,-10,-20}},  

 {{-30,-40,-40,-50,-50,-40,-40,-30}, //король
  {-30,-40,-40,-50,-50,-40,-40,-30},
  {-30,-40,-40,-50,-50,-40,-40,-30},
  {-30,-40,-40,-50,-50,-40,-40,-30},
  {-20,-30,-30,-40,-40,-30,-30,-20},
  {-10,-20,-20,-20,-20,-20,-20,-10},
  { 10, 10,-10,-10,-10,-10, 10, 10},  //{ 20, 20,  0,  0,  0,  0, 20, 20},
  { 10, 40, 30,  0,  0,  0, 50, 10}}, //{ 20, 30, 10,  0,  0, 10, 30, 20}},

 {{-50,-40,-30,-20,-20,-30,-40,-50}, //король эндшпиль
  {-30,-20,-10,  0,  0,-10,-20,-30},
  {-30,-10, 20, 30, 30, 20,-10,-30},
  {-30,-10, 30, 40, 40, 30,-10,-30},
  {-30,-10, 30, 40, 40, 30,-10,-30},
  {-30,-10, 20, 30, 30, 20,-10,-30},
  {-30,-30,  0,  0,  0,  0,-30,-30},
  {-50,-30,-30,-30,-30,-30,-30,-50}}  
};

//******
class Button {
public:
int bx,by,bw,bh,bshift;
String bs;
const uint8_t* bp;
 Button(int x,int y,int w,int h,String s,const uint8_t* p, int shift);
 void Show(int shift); 
 void Show(String s); 
 void Hide();
 boolean IsPressed(int xc, int yc);
private:
boolean active;
 void ishow(uint16_t textcolor,uint16_t backcolor);
};
Button::Button(int x,int y,int w,int h,String s,const uint8_t* p=NULL, int shift=0) {
  bx=x; by=y; bw=w; bh=h; bs=s; bp=p; bshift=shift;
};
void Button::Show(int shift=0) {
 bshift=shift; 
 active=true;
 ishow(WHITE,DARK);  
};
void Button::Show(String s) {
 bs=s; 
 active=true;
 ishow(WHITE,DARK);  
};
void Button::Hide() { 
 active=false; 
 ishow(GRAY,DARK);  
};
boolean Button::IsPressed(int xc, int yc) {
  if (active&&abs(bx+bw/2-xc)<bw/2&&abs(by+bh/2-yc)<bh/2) {
    ishow(WHITE,GRAY); 
    touchx=0; touchy=0;        
    return true;
  } 
  return false;
};  
void Button::ishow(uint16_t textcolor,uint16_t backcolor) {  
  tft.fillRoundRect(bx+2,by+2,bw,bh,3,GRAY2);               
  tft.fillRoundRect(bx,by,bw,bh,3,backcolor);               
  tft.setTextColor(textcolor);
  tft.setTextSize(1);    
  tft.setCursor(bx+(bw-bs.length()*6)/2,by+(bh-8)/2);       
  if (bp!=NULL) { 
   tft.drawBitmap(1+bx+(bw-16)/2, by+(bh-16)/2, bp+bshift*32, 16, 16,textcolor);          
  } 
  else  tft.print(bs);    
}
Button BAction(65, 216, 55, 20, "START");
Button BBW(125, 216, 22, 20, "",&iswb[0][0]);
Button BBack(152, 216, 23, 20, "",&isback[0]);
Button BInit(152, 216, 23, 20, "",&isinit[0]);
Button BLimit(180, 216, 23, 20, "",&islimit[0][0]);
Button BEmpty(180, 216, 23, 20, "",&isempty[0]);
Button BRotate(208, 216, 23, 20, "",&isrotate[0][0]);
Button BPlay(208, 216, 23, 20, "",&isplay[0]);
Button BCM(236, 216, 23, 20, "",&iscm[0][0]);
Button BSave(236, 216, 23, 20, "",&issave[0]);
Button BDesc(264, 216, 23, 20, "",&isdesc[0][0]);
Button BSound(292, 216, 23, 20, "",&issound[0][0]);
Button BLoad(292, 216, 23, 20, "",&isload[0]);
//******
//********************************** 
void drawBitmap(int16_t x, int16_t y,
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color) {
  int16_t i, j, byteWidth = (w + 7) / 8;  
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        tft.drawPixel(x+i, y+j, color);
      }      
    }
  }
}
//****************************
void deletebuttons() {
  tft.fillRect(0,215,320,30,BLACK);               
}

//****************************
void clearstatus() {
  tft.fillRect(0,210,60,30,BLACK);             
  tft.drawFastHLine(5,210,200,BLACK);   
  tft.drawFastHLine(5,212,200,BLACK);  
}  
//****************************
void setup() {      
  Serial.begin(57600);  
  Serial.println(F("Start"));     
  pinMode(13, OUTPUT);
  tft.reset();  
  tft.begin(tft.readID()); 
  tft.setRotation(1);  
  definecolors();
  beep(100);      
  for (int i=0;i<MAXSTEPS;i++) {
    steps[i].x1=0; steps[i].y1=0;
    steps[i].x2=0; steps[i].y2=0;
    steps[i].fig1=0; steps[i].fig2=0;
    steps[i].check=0;
    steps[i].type=0;
    steps[i].weight=0;
  }     
  initboard();
}
//****************************
void initboard() {        
  for (int i=0;i<8;i++)
   for (int j=0;j<8;j++) poledisp[j][i]=-100; //очистка отображенного поля        
  tft.fillScreen(BLACK);      
  tft.setTextColor(GRAY);
  tft.setTextSize(1);  
  for (int j=1;j<9;j++) {
   tft.setCursor(2,j*24-15);  
   if (rotate) tft.print(j); else tft.print(9-j);
  }
  for (byte i=1;i<9;i++) { 
   tft.setCursor(i*24-1,198);    
   if (rotate) tft.print(char(96+9-i)); else tft.print(char(96+i));  
  }         
  initscreen();          
}
//****************************
void initscreen() {        
 show_board();   
 show_steps();      
 BAction.Show("START");
 BBW.Show(cur_step%2);   
 if (cur_step>1) BBack.Show(); else BBack.Hide();
 BSave.Hide();
 BLoad.Hide();
 BInit.Hide();
 BBack.Show();  
 BLimit.Show();         
 BPlay.Hide();           
 BRotate.Show(rotate);           
 BEmpty.Hide();
 BLimit.Show(limit);  
 BCM.Show(always/*checkmatesearch*/); 
 BDesc.Show(choice); 
 BSound.Show(sound);    
}
//****************************
void loop() {        
  gui();  
  if (solving) {        
    int len=70;
    if (!sound) len=2;  
    for (int i=1;i<4;i++) { tone(44,400+300*i,len); delay(70); }
    lastscore=solve_step();               
    float tim=float(millis()-starttime)/1000;
    for (int i=3;i>=1;i--) { tone(44,400+300*i,len); delay(70); }
    if (!checkmatesearch&&lastscore>-9000&&lastscore!=8999) {             
      movestep(cur_step);       
      cur_step++; steps[cur_step].fig1=0; 
    } 
    initscreen();        
    animate_step(cur_step-1,false);
    clearstatus();
    if (lastscore>9000) {       
      tft.setTextSize(1);    
      tft.setTextColor(GREEN); 
      tft.setCursor(0,210);    
      if (lastscore<9999) {
       tft.print("# in "); 
       tft.print((9999-lastscore)/2+1); tft.print(F(" st"));        
      } else tft.print("Checkmate"); 
    } else if (lastscore<-9000) {
      tft.setTextSize(1);    
      tft.setTextColor(RED); 
      tft.setCursor(0,210);    
      tft.print("GIVE UP!");       
      show_steps();
    } else if (lastscore==8999) { //Draw
      tft.setTextSize(1);    
      tft.setTextColor(YELLOW); 
      tft.setCursor(0,210);    
      tft.print("Draw");       
    }         
    tft.setTextSize(1);    
    tft.setTextColor(GRAY); 
    tft.setCursor(0,220);    
    tft.print(tim,0); 
    tft.print("s ");    
    if (abs(lastscore)<5000) {   
     tft.print(lastscore);     
    }
    tft.setCursor(0,230);        
    tft.print(count); 
    tft.print("pos");   
    Serial.print(F("Positions estimated=")); Serial.println(count);
    Serial.print(F("Time=")); Serial.print(tim,1); Serial.println("s");    
  }
  delay(10);  
  
  
}

//********************************** 
void load_choice() {
  for (int i=0;i<7;i++) 
   if (cur_step%2==1) polechoice[i]=i; else polechoice[i]=-i;
}
//********************************** 
void show_choice() {
uint16_t color,color_cont;    
  for (int i=0;i<7;i++) {
    color=BLACKF;
    if (i%2==0) color=WHITEF;              
    tft.fillRect(260,i*25+35,25,25,color);    
    color=DARK;
    if (i==cur_choice) color=WHITE;   
    tft.drawRect(260,i*25+35,25,25,color);
    color=BLACK; color_cont=GRAY;
    if (polechoice[i]>0) { color=WHITE; color_cont=BLACK; }
    if (polechoice[i]!=0) {
     drawBitmap(264, i*25+39,&fig[abs(polechoice[i])-1][0], 16, 16,color);          
     drawBitmap(264, i*25+39,&fig_cont[abs(polechoice[i])-1][0], 16, 16,color_cont);     
    }
  }  
}
//********************************** 
void animate_step(int nstep, boolean hide) {   
 if (!hide&&nstep-1>0&&steps[nstep-1].fig1!=0) animate_step(nstep-1,true); 
 if (nstep-1<1||steps[nstep].fig1==0) return;
 int j=steps[nstep].x1;
 int dj=steps[nstep].x2-steps[nstep].x1;
 int i=steps[nstep].y1;
 int di=steps[nstep].y2-steps[nstep].y1; 
 di=di/abs(di); dj=dj/abs(dj);  
 if (hide) show_fig(i,j);
 while (j!=steps[nstep].x2||i!=steps[nstep].y2) {  
  show_fig(i,j); 
  if (!hide) 
   if (!rotate) tft.drawRect(i*24+13,j*24+1,23,23,WHITE);    
    else tft.drawRect((7-i)*24+13,(7-j)*24+1,23,23,WHITE);    
  int mj=0;  
  if (j!=steps[nstep].x2) { j+=dj; mj=1; }   
  if (abs(steps[nstep].fig1)==fn&&mj==1) continue;
  if (i!=steps[nstep].y2) i+=di;  
 } 
 show_fig(i,j); 
 if (!hide) 
  if (!rotate) tft.drawRect(i*24+13,j*24+1,23,23,WHITE);    
   else tft.drawRect((7-i)*24+13,(7-j)*24+1,23,23,WHITE);     
}
//********************************** 
boolean load_usb() {
char s='x',i=0,j=0;  boolean load=false; 
  Serial.println(F("Wait for FEN position"));
  for (int i=0;i<8;i++)
      for (int j=0;j<8;j++) {
        pole0[j][i]=pole[j][i];
        pole[j][i]=(char)pgm_read_byte(&polezero[j][i]);                 
      }       
  while (s!=' ') {
    s=Serial.read();       
    if (i>7) { i=0; j++; }
    if (!getpole(j,i)) break;
    switch (s) {      
    case '/': i=0; break;  
    case 'p': pole[j][i]=-fp; i++; break;
    case 'P': pole[j][i]=fp; i++; break;
    case 'n': pole[j][i]=-fn; i++; break;
    case 'N': pole[j][i]=fn; i++; break;
    case 'b': pole[j][i]=-fb; i++; break;
    case 'B': pole[j][i]=fb; i++; break;
    case 'r': pole[j][i]=-fr; i++; break;
    case 'R': pole[j][i]=fr; i++; break;
    case 'q': pole[j][i]=-fq; i++; break;
    case 'Q': pole[j][i]=fq; i++; break;
    case 'k': pole[j][i]=-fk; i++; break;
    case 'K': pole[j][i]=fk; i++; break;
    case '1': i++; break;
    case '2': i+=2; break;
    case '3': i+=3; break;
    case '4': i+=4; break;
    case '5': i+=5; break;
    case '6': i+=6; break;
    case '7': i+=7; break;
    case '8': i=0; j++; break;    
    case ' ': break;    
    }    
    delay(20);
    if (i+j>0&&Serial.available()==0) break;
  }  
  s=0;
  if (Serial.available()>0) s=Serial.read();
  while (Serial.available()>0) Serial.read();
  if (s=='w'||s==0) { cur_step=1; load=true; }
   else if (s=='b') { cur_step=2; load=true; }
     else load=false;  
  if (load) {
     steps[1].fig1=0; steps[2].fig1=0;
     Serial.println(F("Position loaded"));
  } else {  
    for (int i=0;i<8;i++)
      for (int j=0;j<8;j++) 
        pole[j][i]=pole0[j][i];     
  }   
  return load;  
}
//********************************** 
void play() {
 BAction.Show("STOP"); 
 BSave.Hide();
 BBW.Hide();
 BLoad.Hide();
 BInit.Hide();   
 BPlay.Hide();   
 BEmpty.Hide();
 BDesc.Hide(); 
 clearstatus();       
 for (int i=cur_step-1;i>0;i--) {
  if (steps[i].fig1!=0) backstep(i);
 }
 show_board();
 show_steps();
 boolean st=false;
 unsigned long tim=millis();
 int i=1;
 while (i<cur_step) {
  if (steps[i].fig1!=0) movestep(i); 
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);    
  int x=388-p.y/2.4; 
  int y=p.x/3.2-50;       
  if (p.z>MINPRESSURE&&p.z<MAXPRESSURE)     
    if (BAction.IsPressed(x,y)) { st=true; BAction.Show(); } 
  if (millis()-tim>2000||st) {
    tim=millis();
    i++;
    show_board();
    beep(20);              
  }  
 }       
 choice=0; 
 beep(200);                
 kingpositions();        
 clearstatus();
 initscreen();                               
}
//********************************** 
void gui() {   
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);  
  // if sharing pins, you'll need to fix the directions of the touchscreen pins  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);    
  int x=388-p.y/2.4; 
  int y=330-p.x/3.2-50;       
  if (always) {
    if (alwaysDelay < alwaysDelayMax) {
      alwaysDelay++;
    } else {
      delay(100);        
      solving=true;   
      BAction.Show();         
      animate_step(cur_step-1,true);
      alwaysDelay=0;
    }
  }
  if (((abs(x-touchx)>5||abs(y-touchy)>5)||drag)&&p.z>MINPRESSURE&&p.z<MAXPRESSURE) {    
    touchx=x;
    touchy=y;             
    if (solving) {
      if (BAction.IsPressed(x,y)) { //стоп вычисление       
        beep(100); 
        solving=false;  
        BAction.Show("stopping");        
        BAction.Hide();        
        return;    
      }   
    } else { //not solve
      if (BAction.IsPressed(x,y)) { //старт вычисление
       if (!choice) {
        delay(100);        
        solving=true;   
        BAction.Show();         
        animate_step(cur_step-1,true);
        return;
       } else { //загрузка позиции с USB
        delay(100);
        BAction.Hide(); 
        BSave.Hide();
        BBW.Hide();
        BLoad.Hide();
        BInit.Hide();   
        BPlay.Hide();   
        BEmpty.Hide();
        BDesc.Hide();        
        show_steps();
        choice=0;         
        if (!load_usb()) return;
        beep(200);                
        kingpositions();        
        clearstatus();
        initscreen();                              
       }
      } else if (BBW.IsPressed(x,y)) {   //черные-белые    
       beep(100);         
       if (cur_step==1) cur_step=2;
        else if (cur_step==2) cur_step=1;
         else if (cur_step%2==0) cur_step--; else cur_step++;       
       BBW.Show(cur_step%2);        
       if (choice) { load_choice(); show_choice(); }
       return;
      } else if (cur_step>1&&BBack.IsPressed(x,y)) {  //ход назад     
       beep(100);                
       cur_step--;       
       animate_step(cur_step,true);
       lastscore=0;              
       backstep(cur_step);               
       steps[cur_step].fig1=0;
       show_board();       
       show_steps();       
       if (cur_step>1) BBack.Show(); else BBack.Hide();      
       BBW.Show(cur_step%2);        
       clearstatus();       
       return;        
      } else if (BLimit.IsPressed(x,y)) {    //предел   
       beep(100);  
       limit++; if (limit>6) limit=0;       
       BLimit.Show(limit);        
       return; 
      } else if (BCM.IsPressed(x,y)) {   //поиск мата                  
       beep(100); 
       always=!always;
       BCM.Show(always);   
       /*checkmatesearch=!checkmatesearch; 
       BCM.Show(checkmatesearch);        
       if (!checkmatesearch) steps[cur_step].fig1=0;
       lastscore=0;
       show_steps();*/
       return; 
      } else if (BRotate.IsPressed(x,y)) {   //поиск мата                  
       beep(100); 
       rotate=!rotate; 
       BRotate.Show(rotate);              
       initboard();       
       return;  
      } else if (BSound.IsPressed(x,y)) {   //звук                  
       if (sound==1) { beep(100); sound=0; } else { sound=1; beep(200); }       
       BSound.Show(sound);        
       return;
      } else if (BDesc.IsPressed(x,y)) {  //расстановка
       beep(100); 
       if (!choice) {
        choice=1;
        cur_choice=1;
        load_choice();
        show_choice();        
        BAction.Show("USB(FEN)");         
        BBack.Hide();  
        BInit.Show();  
        BSave.Show();  
        BLimit.Hide(); 
        BEmpty.Show();  
        BRotate.Hide();    
        BPlay.Show();    
        BCM.Hide();          
        BSound.Hide(); 
        BSave.Show();  
        BLoad.Show();    
       } else {
        choice=0;  
        lastscore=0;
        clearstatus();
        kingpositions();        
        initscreen();        
       }       
       BDesc.Show(choice);                                    
       return;
      } else if (BInit.IsPressed(x,y)) {  //начальная расстановка       
        beep(200);        
        cur_step=1;
        steps[1].fig1=0;
        for (int i=0;i<8;i++)
         for (int j=0;j<8;j++) pole[j][i]=(char)pgm_read_byte(&polestart[j][i]);         
        choice=0;         
        kingpositions();        
        clearstatus();
        initscreen();                     
      } else if (BEmpty.IsPressed(x,y)) {  //пустая доска       
        beep(200);        
        cur_step=1;        
        clearstatus();        
        steps[1].fig1=0; steps[2].fig1=0;
        for (int i=0;i<8;i++)
         for (int j=0;j<8;j++) pole[j][i]=(char)pgm_read_byte(&polezero[j][i]);                 
        BEmpty.Show();   
        show_board();          
      } else if (BSave.IsPressed(x,y)) { 
        beep(100); 
        for (int i=0;i<8;i++)
         for (int j=0;j<8;j++) EEPROM.write(i*8+j,pole[j][i]);         
        EEPROM.put(64,cur_step);
        EEPROM.put(66,limit);
        for (int i=1;i<cur_step;i++) {
          EEPROM.put(66+i*10,steps[i]);
        }
        BSave.Show(); 
      } else if (BLoad.IsPressed(x,y)) { 
        beep(200); 
        for (int i=0;i<8;i++)
         for (int j=0;j<8;j++) pole[j][i]=EEPROM.read(i*8+j);
        EEPROM.get(64,cur_step);                 
        if (cur_step<0||cur_step>400) cur_step=1;        
        EEPROM.get(66,limit);                 
        if (limit<0||limit>6) limit=2;        
        for (int i=1;i<cur_step;i++) {
          EEPROM.get(66+i*10,steps[i]);
        }        
        steps[0].fig1=0;         
        steps[cur_step].fig1=0;         
        choice=0;                 
        kingpositions();
        clearstatus();
        initscreen();                                   
      } else if (BPlay.IsPressed(x,y)) {    //проигрывание партии с начала
       beep(100);  
       BPlay.Show();        
       play();       
       return; 
      }
      if (!choice&&ntouch==0) {
       touchx0=touchx;
       touchy0=touchy;
       char x1=touchy0/24; 
       char y1=(touchx0-10)/24;       
       if (rotate) { x1=7-x1; y1=7-y1; }
       if (getpole(y1,x1)&&(cur_step%2==1&&pole[x1][y1]>0||cur_step%2==0&&pole[x1][y1]<0)) {        
        char p=pole[x1][y1];        
        drag=true;        
        pole[x1][y1]=0;
        show_fig(y1,x1);
        pole[x1][y1]=p;                
       } else drag=false; 
      }       
      ntouch++;
      if (drag&&ntouch>1) {
          uint16_t buf[256];          
          int x1=touchy0/24; 
          int y1=(touchx0-10)/24;       
          if (rotate) { x1=7-x1; y1=7-y1; }       
          if (abs(pole[x1][y1])>0&&(cur_step%2==1&&pole[x1][y1]>0||cur_step%2==0&&pole[x1][y1]<0)) {
          for (int i=0;i<16;i++)
            for (int j=0;j<16;j++) buf[i*16+j]=tft.readPixel(touchx-8+i,touchy-8+j);
          buf[0]=tft.readPixel(touchx-8,touchy-8);  
          uint16_t color,color_cont;    
          color=BLACK; color_cont=GRAY;
          if (pole[x1][y1]>0) { color=WHITE; color_cont=BLACK; }             
          drawBitmap(touchx-8, touchy-8,&fig[abs(pole[x1][y1])-1][0], 16, 16,color);          
          drawBitmap(touchx-8, touchy-8,&fig_cont[abs(pole[x1][y1])-1][0], 16, 16,color_cont);       
          delay(30);
          for (int i=0;i<16;i++)
            for (int j=0;j<16;j++) tft.drawPixel(touchx-8+i,touchy-8+j,buf[i*16+j]);            
          }  
      } else if (choice) {
        int x1=touchy/24; 
        int y1=(touchx-10)/24;             
        if (rotate) { x1=7-x1; y1=7-y1; } 
        if (touchy+touchx!=0&&choice&&getpole(y1,x1)) {        
         pole[x1][y1]=polechoice[cur_choice];
         show_board(); 
        }       
        if (touchx>=260&&touchx<=285&&touchy>=35&&touchy<=210) {
         cur_choice=(touchy-35)/25;
         show_choice();                       
        }
      }
      lasttouch=millis();
    } //not solve      
    if (!drag) { beep(10); ntouch=0; animate_step(cur_step-1,true); }     
  } else { //touching
    if (!solving&&!choice&&drag!=0&&ntouch>0&&p.z<MINPRESSURE&&millis()-lasttouch>300) { //drag 
     char x1=touchy0/24; 
     char y1=(touchx0-10)/24;     
     char x2=touchy/24; 
     char y2=(touchx-10)/24; 
     if (rotate) { x1=7-x1; y1=7-y1; x2=7-x2; y2=7-y2; }    
     if (getpole(y1,x1)) show_fig(y1,x1);
     if (getpole(y1,x1)&&getpole(y2,x2)) {      
       start_var=cur_step+1;
       only_action=false;   
       load_variants(cur_step);     
       for (int i=cur_step+1;i<cur_step+cur_var+1;i++)  
        if (steps[i].x1==x1&&steps[i].y1==y1&&steps[i].x2==x2&&steps[i].y2==y2) {
         steps[cur_step]=steps[i];          
         movestep(cur_step); cur_step++; steps[cur_step].fig1=0;
         int len=100;
         if (!sound) len=2;  
         for (int i=2;i>=1;i--) { tone(44,400+300*i,len); delay(100); }
         initscreen();
         animate_step(cur_step-1,false);
         break;                                 
        }            
     } else { 
      beep(10);       
     }
     ntouch=0;       
     drag=false;
    } 
  }
  if (millis()-quitime>1000) {
    if (isstatus) show_status(); 
    quitime=millis();
  }
  
  
}
//********************************** 
void show_status() {      
   int tim=(millis()-starttime)/1000; 
   int cur=200000*tim/(limittime-starttime);
   if (cur>200) { cur=200; solving=false; }   
   tft.drawFastHLine(5,210,cur,GRAY);                  
   tft.drawFastHLine(5,212,progress*2,GRAY);  
   int m=tim/60;
   int s=tim%60;
   tft.setTextWrap(1);
   tft.setTextColor(GRAY);
   tft.setTextSize(1);
   tft.fillRect(0,220,60,10,BLACK);             
   tft.setCursor(2,220);     
   if (m>0) { tft.print(m); tft.print(":"); }
   if (s>0) {
    if (s<10&&m>0) tft.print("0"); 
    tft.print(s);
   } else if (m>0) tft.print("00"); else tft.print("0");
   tft.setCursor(36,220);   
   tft.print(cur_level);
   
   tft.setCursor(2,230);   
   if (cur_step%2==1) tft.setTextColor(WHITE); else tft.setTextColor(GRAY);   
   if (lastbest.fig1!=steps[0].fig1||lastbest.x1!=steps[0].x1||lastbest.y1!=steps[0].y1||
       lastbest.x2!=steps[0].x2||lastbest.y2!=steps[0].y2) {
     beep(10); 
     lastbest=steps[0];    
     tft.fillRect(0,230,50,10,BLACK);             
     tft.setCursor(2,230);     
     tft.print(str_step(0));    
     blinkstep=0;
   }  
   if (steps[0].fig1!=0&&blinkstep==0&&tim>5) {
    char poleb[8][8];     
    for (int i=0;i<8;i++)
     for (int j=0;j<8;j++) { poleb[j][i]=pole[j][i]; pole[j][i]=pole0[j][i]; }         
    movestep(0);    
    show_board(); 
    delay(100);  
    backstep(0);   
    show_board();      
    for (int i=0;i<8;i++)
     for (int j=0;j<8;j++) pole[j][i]=poleb[j][i];           
    kingpositions();  
   }
   blinkstep++;      
   if (blinkstep>2) blinkstep=0;              
}
//****************************
void beep(int leng) {
  analogWrite(PinBuzz, 20);
  delay(2); 
  if (!sound) analogWrite(PinBuzz, 0);  
  delay(leng); 
  analogWrite(PinBuzz, 0);   
}  
//****************************
void definecolors() {
 BLACK  =0x0000;
 BLUE   =0x07FF;  //0x001F;
 RED    =0xF800;
 GREEN  =0x07E0;
 CYAN   =0x07FF;
 MAGENTA=0xF81F;
 YELLOW =0xFFE0;
 WHITE  =0xFFFF;
 GRAY   =0x7BEF;
 DARK   =getColor(32,32,32); 
 GRAY2   =getColor(16,16,16); 
 BLACKF  =getColor(94,58,0); 
 WHITEF  =getColor(180,114,0); 
}
//****************************
uint16_t getColor(uint8_t red, uint8_t green, uint8_t blue)
{
  red   >>= 3;
  green >>= 2;
  blue  >>= 3;
  return (red << 11) | (green << 5) | blue;
}
//****************************
void show_fig(int i,int j) {
uint16_t color,color_cont;  
   color=BLACKF;   
   if ((i+j+2)%2==0) color=WHITEF;           
   int jj=j, ii=i;
   if (rotate) { jj=7-j; ii=7-i; }   
   tft.fillRect(ii*24+12,jj*24,25,25,color);    
   tft.drawRect(ii*24+12,jj*24,25,25,WHITE);  
   color=BLACK; color_cont=GRAY;
   if (pole[j][i]>0) { color=WHITE; color_cont=BLACK; }
   if (pole[j][i]!=0) {
    drawBitmap(ii*24+17, jj*24+5,&fig[abs(pole[j][i])-1][0], 16, 16,color);          
    drawBitmap(ii*24+17, jj*24+5,&fig_cont[abs(pole[j][i])-1][0], 16, 16,color_cont);     
   }
}
//****************************
void show_board() { 
 for (int i=0;i<8;i++)
  for (int j=0;j<8;j++) {
    if (poledisp[j][i]!=pole[j][i]) show_fig(i,j);      
    poledisp[j][i]=pole[j][i];
  }  
}
//****************************
String str_step(int i) {
  String s="";
   if (steps[i].fig1==0) return s;
   if (steps[i].type==2) s="0-0";
    else if (steps[i].type==3) s="0-0-0";
   else  {
    if (abs(steps[i].fig1)>1) s=fig_symb[abs(steps[i].fig1)];   
    if (abs(steps[i].fig1<5)) {
     s=s+char(97+steps[i].y1);  
     s=s+String(8-steps[i].x1);    
     if (steps[i].fig2==0) s=s+"-";    
    }
    if (steps[i].fig2!=0) {
      s=s+"x";    
      if (abs(steps[i].fig2)>1) s=s+fig_symb[abs(steps[i].fig2)];
    }    
    s=s+char(97+steps[i].y2);  
    s=s+String(8-steps[i].x2);
   } 
   if (steps[i].type>3) s=s+fig_symb[steps[i].type-2];
   if (steps[i].check==1) s=s+"+"; else 
    if (steps[i].check==2) s=s+"#";
   return s;
}
//****************************
void show_steps() {
  tft.fillRect(206,0,114,214,getColor(8,8,8));  
  tft.fillRect(206,0,114,12,DARK);  
  tft.setTextSize(1); 
  tft.setTextColor(WHITE,DARK);    
  tft.setCursor(206,2);    
  tft.print(" ArduinoMega Chess "); 
  int i=1; int y=14;
  tft.setTextColor(GRAY);    
  int cur=(cur_step+1)/2; //номер текущего хода 
  int lim_step=cur_step;         
  if (checkmatesearch&&lastscore>9000) {     
    tft.setTextColor(GREEN);
    lim_step=cur_step+10000-lastscore;
    cur=lim_step/2;
  }
  if (cur>20) i=cur-20;
  while (i<=cur&&y<207) { //номер хода
    tft.setCursor(210,y);    
    tft.print(i); 
    tft.print("."); 
    if (steps[i*2-1].fig1!=0) tft.print(str_step(i*2-1));
    if (steps[i*2].fig1==0||i*2>lim_step) break;        
    tft.print(","); 
    tft.print(str_step(i*2));    
    i++; y+=10; 
  }
    
}
//****************************
boolean getdiagrowcheckw(char dj,char di) { //вычисляет шах по диагоналям белым
char d,j1,i1;           
   j1=WKJ; i1=WKI;
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==-fq||pole[j1][i1]==-fb) return true;      
      if (pole[j1][i1]!=0) break;      
    } else break;
   }
   return false;
}
//****************************
boolean getdiagrowcheckb(char dj,char di) { //вычисляет шах по диагоналям черным
char d,j1,i1;           
   j1=BKJ; i1=BKI;
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==fq||pole[j1][i1]==fb) return true;      
      if (pole[j1][i1]!=0) break;      
    } else break;
   }
   return false;
}
//****************************
boolean getstreightrowcheckw(char dj,char di) { //вычисляет шах по горизонталям-вертикалям белым
char d,j1,i1;  
   j1=WKJ; i1=WKI;
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;    
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==-fq||pole[j1][i1]==-fr) return true;      
      if (pole[j1][i1]!=0) break;      
    } else break;
   }
   return false;
}
//****************************
boolean getstreightrowcheckb(char dj,char di) { //вычисляет шах по горизонталям-вертикалям черным
char d,j1,i1;  
   j1=BKJ; i1=BKI;
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;    
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==fq||pole[j1][i1]==fr) return true;      
      if (pole[j1][i1]!=0) break;      
    } else break;
   }
   return false;
}
//****************************
boolean get_check(char king) { //Определяем шах для заданного короля  
  if (king==fk) { //белый король            
    if (getdiagrowcheckw(-1, 1)) return true;    
    if (getdiagrowcheckw(-1,-1)) return true;              
    if (getdiagrowcheckw( 1,-1)) return true;
    if (getdiagrowcheckw( 1, 1)) return true;    
    if (getstreightrowcheckw(-1, 0)) return true;                  
    if (getstreightrowcheckw( 0, 1)) return true;    
    if (getstreightrowcheckw( 0,-1)) return true;    
    if (getstreightrowcheckw( 1, 0)) return true;            
    if (getpole(WKJ-2,WKI-1)&&pole[WKJ-2][WKI-1]==-fn) return true;           
    if (getpole(WKJ-2,WKI+1)&&pole[WKJ-2][WKI+1]==-fn) return true;            
    if (getpole(WKJ-1,WKI-2)&&pole[WKJ-1][WKI-2]==-fn) return true;        
    if (getpole(WKJ-1,WKI+2)&&pole[WKJ-1][WKI+2]==-fn) return true;        
    if (getpole(WKJ+1,WKI-2)&&pole[WKJ+1][WKI-2]==-fn) return true;        
    if (getpole(WKJ+1,WKI+2)&&pole[WKJ+1][WKI+2]==-fn) return true;        
    if (getpole(WKJ+2,WKI-1)&&pole[WKJ+2][WKI-1]==-fn) return true;        
    if (getpole(WKJ+2,WKI+1)&&pole[WKJ+2][WKI+1]==-fn) return true;              
    if (getpole(WKJ-1,WKI-1)&&pole[WKJ-1][WKI-1]==-fp) return true;    
    if (getpole(WKJ-1,WKI+1)&&pole[WKJ-1][WKI+1]==-fp) return true;                
  } else { //черный король                
    if (getdiagrowcheckb( 1,-1)) return true;
    if (getdiagrowcheckb( 1, 1)) return true;
    if (getdiagrowcheckb(-1, 1)) return true;
    if (getdiagrowcheckb(-1,-1)) return true;      
    if (getstreightrowcheckb( 1, 0)) return true;    
    if (getstreightrowcheckb( 0, 1)) return true;
    if (getstreightrowcheckb( 0,-1)) return true;
    if (getstreightrowcheckb(-1, 0)) return true;              
    if (getpole(BKJ+2,BKI-1)&&pole[BKJ+2][BKI-1]==fn) return true;    
    if (getpole(BKJ+2,BKI+1)&&pole[BKJ+2][BKI+1]==fn) return true;            
    if (getpole(BKJ+1,BKI-2)&&pole[BKJ+1][BKI-2]==fn) return true;    
    if (getpole(BKJ+1,BKI+2)&&pole[BKJ+1][BKI+2]==fn) return true;                
    if (getpole(BKJ-1,BKI-2)&&pole[BKJ-1][BKI-2]==fn) return true;    
    if (getpole(BKJ-1,BKI+2)&&pole[BKJ-1][BKI+2]==fn) return true;    
    if (getpole(BKJ-2,BKI-1)&&pole[BKJ-2][BKI-1]==fn) return true;   
    if (getpole(BKJ-2,BKI+1)&&pole[BKJ-2][BKI+1]==fn) return true;           
    if (getpole(BKJ+1,BKI-1)&&pole[BKJ+1][BKI-1]==fp) return true;
    if (getpole(BKJ+1,BKI+1)&&pole[BKJ+1][BKI+1]==fp) return true;      
  }    
  if (abs(BKJ-WKJ)<=1&&abs(BKI-WKI)<=1) return true; //расстояние между королями  
  return false;
}
//****************************
boolean getpole(char j,char i) { //поле приемник существует       
  if (j>=0&&j<8&&i>=0&&i<8) return true; 
  return false; 
}
//****************************
void addstep(char j1,char i1,char j2,char i2,char type) {       
 int st=start_var+cur_var; 
 steps[st].x1=j1;
 steps[st].x2=j2;
 steps[st].y1=i1;
 steps[st].y2=i2;
 steps[st].fig1=pole[j1][i1];
 steps[st].fig2=pole[j2][i2];        
 if (type==1) { //взятие на проходе
  steps[st].fig2=-steps[st].fig1;        
 }
 steps[st].type=type;   
 char ko=-fk; //свой король  
 if (steps[st].fig1>0) ko=fk;   
 movestep(st);  
 if (get_check(ko)) { backstep(st); return; } //шах своему королю после хода - запрещен ход  
 boolean che=get_check(-ko); //шах чужому  королю после хода 
 backstep(st);
 steps[st].weight=abs(steps[st].fig2)-abs(steps[st].fig1);
 if (type>3) steps[st].weight+=fig_weight[type-2]; 
 if (endspiel&&steps[st].fig1==ko) steps[st].weight+=10; // в эндшпиле - шаги короля вперед
 steps[st].check=che;
 if (che) steps[st].weight+=10;
 if (only_action) {
  if (steps[st].fig1==fp&&steps[st].x2==1||steps[st].fig1==-fp&&steps[st].x2==6) //предпроходные пешки
   { cur_var++; return; }
  if (steps[st].fig2==0&&steps[st].type<4&&!che&&!check_on_table) return;    
 }
 
 cur_var++; 
}
//****************************
void getrowstepsw(char j,char i,char dj,char di) { //вычисляет прямые длинные ходы белых    
char d,j1,i1;     
   j1=j; i1=i; 
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]<=0) addstep(j,i,j1,i1,0);
      if (pole[j1][i1]!=0) break;
    } else break;
   }
}
//****************************
void getrowstepsb(char j,char i,char dj,char di) { //вычисляет прямые длинные ходы черных
char d,j1,i1;     
   j1=j; i1=i; 
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]>=0) addstep(j,i,j1,i1,0);
      if (pole[j1][i1]!=0) break;
    } else break;
   }
}
//****************************
void getonestepw(char j,char i,char dj,char di) { //вычисляет одиночный ход белых    
char j1,i1;  
  j1=j+dj; i1=i+di;
  if (getpole(j1,i1)) 
    if (pole[j1][i1]<=0) addstep(j,i,j1,i1,0);   
}
//****************************
void getonestepb(char j,char i,char dj,char di) { //вычисляет одиночный ход черных
char j1,i1;  
  j1=j+dj; i1=i+di;
  if (getpole(j1,i1)) 
    if (pole[j1][i1]>=0) addstep(j,i,j1,i1,0);   
}
//****************************
void sort_variants(int from, int to) { //сортировка заданного массива вариантов по весу
   while (1) { 
    int mov=0;     
    for (int i=from;i<to;i++)  //сортировка по весу позиции   
     if (steps[i].weight<steps[i+1].weight) {    
       mov++; 
       step_type buf=steps[i];
       steps[i]=steps[i+1];
       steps[i+1]=buf;      
     }                   
    if (mov==0) break;
   }    
}
//****************************  
void load_variants(int nstep) { //Заполняем массив вариантов ходов для хода номер nstep
  cur_var=0;       
  if (pole[WKJ][WKI]!=fk||pole[BKJ][BKI]!=-fk) kingpositions(); 
  if (nstep%2==1) check_on_table=get_check(fk);   
   else check_on_table=get_check(-fk); //шах на доске    
  for (char i=0;i<8;i++)
   for (char j=0;j<8;j++) 
    if (pole[j][i]>0&&nstep%2==1||pole[j][i]<0&&nstep%2==0) { 
     switch (pole[j][i]) { 
     case fp: //пешка белая         
      if (getpole(j-1,i)&&pole[j-1][i]==0) 
        if (j!=1) addstep(j,i,j-1,i,0); else 
          for (char t=4;t<8;t++) addstep(1,i,0,i,t); //пешка-...                        
      if (j==6&&pole[j-1][i]==0&&pole[j-2][i]==0) addstep(j,i,j-2,i,0);
      if (getpole(j-1,i-1)&&pole[j-1][i-1]<0) 
       if (j!=1) addstep(j,i,j-1,i-1,0); else 
          for (char t=4;t<8;t++) addstep(j,i,j-1,i-1,t); //пешка-...                                      
      if (getpole(j-1,i+1)&&pole[j-1][i+1]<0) 
       if (j!=1) addstep(j,i,j-1,i+1,0); else 
         for (char t=4;t<8;t++) addstep(j,i,j-1,i+1,t); //пешка-...                                      
      if (j==3&&steps[nstep-1].fig1==-fp&&steps[nstep-1].x2==3&&steps[nstep-1].x1==1) {          
          if (steps[nstep-1].y2-i==1) { //взятие на проходе справа             
            addstep(j,i,j-1,i+1,1);
          } else if (steps[nstep-1].y2-i==-1) { //взятие на проходе слева              
            addstep(j,i,j-1,i-1,1);
          }
      }  
     break;
     case -fp: //пешка черная                 
      if (getpole(j+1,i)&&pole[j+1][i]==0) 
        if (j!=6) addstep(j,i,j+1,i,0); else 
          for (char t=4;t<8;t++) addstep(j,i,j+1,i,t); //пешка-...                                
      if (j==1&&pole[j+1][i]==0&&pole[j+2][i]==0) addstep(j,i,j+2,i,0);
      if (getpole(j+1,i-1)&&pole[j+1][i-1]>0) 
       if (j!=6) addstep(j,i,j+1,i-1,0); else
         for (char t=4;t<8;t++) addstep(j,i,j+1,i-1,t); //пешка-...                                       
      if (getpole(j+1,i+1)&&pole[j+1][i+1]>0) 
       if (j!=6) addstep(j,i,j+1,i+1,0); else
        for (char t=4;t<8;t++) addstep(j,i,j+1,i+1,t); //пешка-...                                       
      if (j==4&&steps[nstep-1].fig1==fp&&steps[nstep-1].x2==4&&steps[nstep-1].x1==6) {                                      
          if (steps[nstep-1].y2-i==1) { //взятие на проходе справа                         
            addstep(j,i,j+1,i+1,1);
          } else if (steps[nstep-1].y2-i==-1) { //взятие на проходе слева              
            addstep(j,i,j+1,i-1,1);
          }
      }          
     break;   
     case fn: //конь белый  
      getonestepw(j,i,-2,-1);
      getonestepw(j,i,-2,1);
      getonestepw(j,i,-1,-2);
      getonestepw(j,i,-1,2);
      getonestepw(j,i,2,-1);
      getonestepw(j,i,2,1);
      getonestepw(j,i,1,-2);
      getonestepw(j,i,1,2);
      break;   
     case -fn: //конь черный         
      getonestepb(j,i,-2,-1);
      getonestepb(j,i,-2,1);
      getonestepb(j,i,-1,-2);
      getonestepb(j,i,-1,2);
      getonestepb(j,i,2,-1);
      getonestepb(j,i,2,1);
      getonestepb(j,i,1,-2);
      getonestepb(j,i,1,2);
      break;
     case fb: //слон белый  
      getrowstepsw(j,i,1,1);
      getrowstepsw(j,i,-1,-1);
      getrowstepsw(j,i,1,-1);
      getrowstepsw(j,i,-1,1);   
      break; 
     case -fb: //слон черный     
      getrowstepsb(j,i,1,1);
      getrowstepsb(j,i,-1,-1);
      getrowstepsb(j,i,1,-1);
      getrowstepsb(j,i,-1,1);   
      break;
     case fr: //ладья белая     
      getrowstepsw(j,i,1,0);
      getrowstepsw(j,i,-1,0);
      getrowstepsw(j,i,0,1);
      getrowstepsw(j,i,0,-1);
      break;
     case -fr: //ладья черная              
      getrowstepsb(j,i,1,0);
      getrowstepsb(j,i,-1,0);
      getrowstepsb(j,i,0,1);
      getrowstepsb(j,i,0,-1);
      break;
     case fq: //ферзь белый
      getrowstepsw(j,i,1,1);
      getrowstepsw(j,i,-1,-1);
      getrowstepsw(j,i,1,-1);
      getrowstepsw(j,i,-1,1);   
      getrowstepsw(j,i,1,0);
      getrowstepsw(j,i,-1,0);
      getrowstepsw(j,i,0,1);
      getrowstepsw(j,i,0,-1);   
      break;
     case -fq: //ферзь черный   
      getrowstepsb(j,i,1,1);
      getrowstepsb(j,i,-1,-1);
      getrowstepsb(j,i,1,-1);
      getrowstepsb(j,i,-1,1);   
      getrowstepsb(j,i,1,0);
      getrowstepsb(j,i,-1,0);
      getrowstepsb(j,i,0,1);
      getrowstepsb(j,i,0,-1);   
      break;
     case fk: //король белый
      getonestepw(j,i, 1,-1); 
      getonestepw(j,i, 1, 0); 
      getonestepw(j,i, 1, 1);       
      getonestepw(j,i, 0,-1);                        
      getonestepw(j,i, 0, 1);  
      getonestepw(j,i,-1,-1); 
      getonestepw(j,i,-1, 0); 
      getonestepw(j,i,-1, 1);            
      if (!check_on_table) add_rok(j,i,nstep);
      break;       
     case -fk: //король черный   
      getonestepb(j,i, 1,-1); 
      getonestepb(j,i, 1, 0); 
      getonestepb(j,i, 1, 1);       
      getonestepb(j,i, 0,-1);                        
      getonestepb(j,i, 0, 1);  
      getonestepb(j,i,-1,-1); 
      getonestepb(j,i,-1, 0); 
      getonestepb(j,i,-1, 1);            
      if (!check_on_table) add_rok(j,i,nstep);
      break;       
    } //switch
   } //if 

   if (nstep>cur_step) {
    for (int i=start_var;i<start_var+cur_var;i++) { //добавление к весу отсечек             
     for (char j=0;j<MAXCUTS;j++)    
      if (cuts[j].fig1==steps[i].fig1&&
       cuts[j].x1==steps[i].x1&&cuts[j].y1==steps[i].y1&&
       cuts[j].x2==steps[i].x2&&cuts[j].y2==steps[i].y2) {
        steps[i].weight+=100+cuts[j].weight;              
        break;
      }      
     if (nstep>1&&steps[nstep-1].fig2!=0)  //если предыдущий ход был взятие - вперед двигать ход с ответным взятием
       if (steps[nstep-1].x2==steps[i].x2&&steps[nstep-1].y2==steps[i].y2) 
         steps[i].weight+=1000;              
    }    
   }   

   sort_variants(start_var,start_var+cur_var-1);   
        
 }
//****************************
void movestep(int nstep) {     
  pole[steps[nstep].x1][steps[nstep].y1]=0;
  pole[steps[nstep].x2][steps[nstep].y2]=steps[nstep].fig1;   
  if (steps[nstep].fig1==fk) {
   WKJ=steps[nstep].x2; WKI=steps[nstep].y2; 
  } else if (steps[nstep].fig1==-fk) {
   BKJ=steps[nstep].x2; BKI=steps[nstep].y2; 
  }
  if (steps[nstep].type==0) return; 
  if (steps[nstep].type==1)  //взятие на проходе
   if (steps[nstep].fig1>0)
    pole[steps[nstep].x2+1][steps[nstep].y2]=0;
   else 
    pole[steps[nstep].x2-1][steps[nstep].y2]=0; 
  else if (steps[nstep].type==2)  //короткая рокировка
   if (steps[nstep].fig1>0) { //белые     
    pole[7][4]=0; pole[7][5]=fr; pole[7][6]=fk; pole[7][7]=0; //show_board(); delay(3000);
   } else { //черные        
    pole[0][4]=0; pole[0][5]=-fr; pole[0][6]=-fk; pole[0][7]=0; //show_board(); delay(3000);
    }
  else if (steps[nstep].type==3)  //длинная рокировка
   if (steps[nstep].fig1>0) { //белые     
    pole[7][0]=0; pole[7][1]=0; pole[7][2]=fk; pole[7][3]=fr; pole[7][4]=0;
   } else { //черные
    pole[0][0]=0; pole[0][1]=0; pole[0][2]=-fk; pole[0][3]=-fr; pole[0][4]=0;    
   }  
   else if (steps[nstep].type>3)  //пешка-....
   if (steps[nstep].fig1>0) pole[steps[nstep].x2][steps[nstep].y2]=steps[nstep].type-2;
    else pole[steps[nstep].x2][steps[nstep].y2]=2-steps[nstep].type;
}
//****************************  
void backstep(int nstep) {
  pole[steps[nstep].x1][steps[nstep].y1]=steps[nstep].fig1; 
  pole[steps[nstep].x2][steps[nstep].y2]=steps[nstep].fig2; 
  if (steps[nstep].fig1==fk) {
    WKJ=steps[nstep].x1; WKI=steps[nstep].y1; 
  } else if (steps[nstep].fig1==-fk) {
    BKJ=steps[nstep].x1; BKI=steps[nstep].y1; 
  }
  if (steps[nstep].type==0) return;
  if (steps[nstep].type==1) { //взятие на проходе
    pole[steps[nstep].x2][steps[nstep].y2]=0;
    if (steps[nstep].fig1>0)
     pole[steps[nstep].x2+1][steps[nstep].y2]=-fp; 
    else 
     pole[steps[nstep].x2-1][steps[nstep].y2]=fp; 
  } else if (steps[nstep].type==2)  //короткая рокировка
   if (steps[nstep].fig1>0) { //белые 
    pole[7][4]=fk; pole[7][5]=0; pole[7][6]=0; pole[7][7]=fr; 
   } else { //черные
    pole[0][4]=-fk; pole[0][5]=0; pole[0][6]=0; pole[0][7]=-fr; // show_board(); delay(3000);
   }
  else if (steps[nstep].type==3)  //длинная рокировка
   if (steps[nstep].fig1>0) { //белые 
    pole[7][0]=fr; pole[7][1]=0; pole[7][2]=0; pole[7][3]=0; pole[7][4]=fk;    
   } else { //черные
    pole[0][0]=-fr; pole[0][1]=0; pole[0][2]=0; pole[0][3]=0; pole[0][4]=-fk;
   }
     
}
//****************************
void get_wrocks(int nstep) {
  w00=true; w000=true; 
  for (int i=1;i<nstep;i++) {
   if (steps[i].fig1==fk) { w00=false; w000=false; return; }  
   if (steps[i].fig1==fr) 
    if (steps[i].x1==7&&steps[i].y1==7) w00=false;
     else if (steps[i].x1==7&&steps[i].y1==0) w000=false;
  } 
}
//****************************
void get_brocks(int nstep) {
  b00=true; b000=true; 
  for (int i=1;i<nstep;i++) {
   if (steps[i].fig1==-fk) { b00=false; b000=false; return; }  
   if (steps[i].fig1==-fr) 
    if (steps[i].x1==0&&steps[i].y1==7) b00=false;
     else if (steps[i].x1==0&&steps[i].y1==0) b000=false;
  } 
}
//****************************
void add_rok(char j,char i,int nstep) { //Добавляем рокировку если возможно
boolean che1,che2;   
 if (nstep%2==1) { //белые   
  if (j!=7||i!=4) return;  
  if (pole[7][5]==0&&pole[7][6]==0&&pole[7][7]==fr) { //короткая  
    pole[7][4]=0; 
    WKI=5; che1=get_check(fk); 
    WKI=6; che2=get_check(fk); 
    WKI=4; pole[7][4]=fk; 
    get_wrocks(nstep);
    if (!che1&&!che2&&w00) addstep(7,4,7,6,2);
  }
  if (pole[7][0]==fr&&pole[7][1]==0&&pole[7][2]==0&&pole[7][3]==0) { //длинная
    pole[7][4]=0; 
    WKI=2; che1=get_check(fk); 
    WKI=3; che2=get_check(fk); 
    WKI=4; pole[7][4]=fk; 
    get_wrocks(nstep);
    if (!che1&&!che2&&w000) addstep(7,4,7,2,3);
  }
 } else { //черные
  if (j!=0||i!=4) return;  
  if (pole[0][5]==0&&pole[0][6]==0&&pole[0][7]==-fr) { //короткая    
    pole[0][4]=0;
    BKI=5; che1=get_check(-fk); 
    BKI=6; che2=get_check(-fk); 
    BKI=4; pole[0][4]=-fk; 
    get_brocks(nstep);
    if (!che1&&!che2&&b00) addstep(0,4,0,6,2);
  }
  if (pole[0][0]==-fr&&pole[0][1]==0&&pole[0][2]==0&&pole[0][3]==0) { //длинная
    pole[0][4]=0;
    BKI=2; che1=get_check(-fk); 
    BKI=3; che2=get_check(-fk); 
    BKI=4; pole[0][4]=-fk; 
    get_brocks(nstep);
    if (!che1&&!che2&&b000) addstep(0,4,0,2,3);
  }
   
 }
}
//****************************
int addrowstepsw(char j,char i,char dj,char di,char dc) { //добавляет длинные ходы белых    
char d,j1,i1; int c=0;        
   j1=j; i1=i; 
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==0) c+=dc; 
      else if (pole[j1][i1]>0) { //уперлись в свою фигуру        
        c+=1; break;
      } else {        
        if (cur_step>6) c+=-pole[j1][i1]; //добавляем код фигуры под боем в виде прибавки        
        break;
      }
    } else break;
   }
   return c;
}
//****************************
int addrowstepsb(char j,char i,char dj,char di,char dc) { //добавляет длинные ходы черных
char d,j1,i1; int c=0;    
   j1=j; i1=i; 
   for (d=1;d<8;d++) {    
    j1+=dj; i1+=di;
    if (getpole(j1,i1)) {      
      if (pole[j1][i1]==0) c-=dc; 
      else if (pole[j1][i1]<0) { //уперлись в свою фигуру        
        c-=1; break;
      } else {        
        if (cur_step>6) c-=pole[j1][i1]; //вычитаем код фигуры под боем в виде прибавки        
        break;
      }
    } else break;
   }
   return c;
}
//****************************
int addonestepw(char j,char i,char dj,char di) { //добавляет одиночный ход белых    
char j1,i1;  
  j1=j+dj; i1=i+di;
  if (getpole(j1,i1)) 
    if (pole[j1][i1]==0) return 2;   
     else if (pole[j1][i1]>0) return 1;
      else return -pole[j1][i1];   
  return 0;    
}
//****************************
int addonestepb(char j,char i,char dj,char di) { //добавляет одиночный ход черных
char j1,i1; 
  j1=j+dj; i1=i+di;
  if (getpole(j1,i1)) 
    if (pole[j1][i1]==0) return 2;   
     else if (pole[j1][i1]<0) return 1;
      else return pole[j1][i1];   
  return 0;  
}
//****************************
int activity() { //Вычисляет активность позиции по числу ходов белых-черных + защита королей
int c=0;
char pwj[8],pwi[8],pbj[8],pbi[8],ipw=0,ipb=0,nbw=0,nbb=0; 
  if (checkmatesearch) return 0; //кроме поиска мата
  for (char i=0;i<8;i++)
   for (char j=0;j<8;j++) 
    if (pole[j][i]!=0) { 
     switch (pole[j][i]) { 
     case fp: //пешка белая     
      if (getpole(j,i+1)&&pole[j][i+1]==fp) c+=2; //фаланга      
      if (getpole(j-1,i)&&pole[j-1][i]==fp) c-=20; //сдвоенная пешка
      pwj[ipw]=j; pwi[ipw]=i; ipw++;
     break;
     case -fp: //пешка черная                 
      if (getpole(j,i-1)&&pole[j][i-1]==-fp) c-=2; //фаланга      
      if (getpole(j+1,i)&&pole[j+1][i]==-fp) c+=20; //сдвоенная пешка
      pbj[ipb]=j; pbi[ipb]=i; ipb++;
     break;   
     case fn: //конь белый  
      c+=addonestepw(j,i,-2,-1);
      c+=addonestepw(j,i,-2,1);
      c+=addonestepw(j,i,-1,-2);
      c+=addonestepw(j,i,-1,2);
      c+=addonestepw(j,i,2,-1);
      c+=addonestepw(j,i,2,1);
      c+=addonestepw(j,i,1,-2);
      c+=addonestepw(j,i,1,2);
      break;   
     case -fn: //конь черный         
      c+=addonestepb(j,i,-2,-1);
      c+=addonestepb(j,i,-2,1);
      c+=addonestepb(j,i,-1,-2);
      c+=addonestepb(j,i,-1,2);
      c+=addonestepb(j,i,2,-1);
      c+=addonestepb(j,i,2,1);
      c+=addonestepb(j,i,1,-2);
      c+=addonestepb(j,i,1,2);
      break;
     case fb: //слон белый  
      c+=addrowstepsw(j,i,1,1,1);
      c+=addrowstepsw(j,i,-1,-1,2);
      c+=addrowstepsw(j,i,1,-1,1);
      c+=addrowstepsw(j,i,-1,1,2);   
      nbw++;
      break; 
     case -fb: //слон черный     
      c+=addrowstepsb(j,i,1,1,2);
      c+=addrowstepsb(j,i,-1,-1,1);
      c+=addrowstepsb(j,i,1,-1,2);
      c+=addrowstepsb(j,i,-1,1,1);   
      nbb++;
      break;
     case fr: //ладья белая     
      c+=addrowstepsw(j,i,1,0,2); //вертикали 
      c+=addrowstepsw(j,i,-1,0,2);
      c+=addrowstepsw(j,i,0,1,1);
      c+=addrowstepsw(j,i,0,-1,1);
      break;
     case -fr: //ладья черная              
      c+=addrowstepsb(j,i,1,0,2); //вертикали
      c+=addrowstepsb(j,i,-1,0,2);
      c+=addrowstepsb(j,i,0,1,1);
      c+=addrowstepsb(j,i,0,-1,1);
      break;
     case fq: //ферзь белый
      if (cur_step>10) {
       c+=addrowstepsw(j,i,1,1,1);
       c+=addrowstepsw(j,i,-1,-1,2);
       c+=addrowstepsw(j,i,1,-1,1);
       c+=addrowstepsw(j,i,-1,1,2);   
       c+=addrowstepsw(j,i,1,0,1);
       c+=addrowstepsw(j,i,-1,0,2);
       c+=addrowstepsw(j,i,0,1,1);
       c+=addrowstepsw(j,i,0,-1,1);   
      } 
      break;
     case -fq: //ферзь черный   
      if (cur_step>10) {
       c+=addrowstepsb(j,i,1,1,2);
       c+=addrowstepsb(j,i,-1,-1,1);
       c+=addrowstepsb(j,i,1,-1,2);
       c+=addrowstepsb(j,i,-1,1,1);   
       c+=addrowstepsb(j,i,1,0,2);
       c+=addrowstepsb(j,i,-1,0,1);
       c+=addrowstepsb(j,i,0,1,1);
       c+=addrowstepsb(j,i,0,-1,1);   
      } 
      break;    
    } //switch
    if (cur_step>6) {
     if (pole[j][i]>0) { //белые рядом с королями
      if (abs(WKJ-j)<2&&abs(WKI-i)<2) c+=3;  
      if (abs(BKJ-j)<2&&abs(BKI-i)<2) c+=3;  
      if (abs(WKJ-j)<3&&abs(WKI-i)<3) c+=2;  
      if (abs(BKJ-j)<3&&abs(BKI-i)<3) c+=2;  
     } else { //черные рядом с королями
      if (abs(WKJ-j)<2&&abs(WKI-i)<2) c-=3;  
      if (abs(BKJ-j)<2&&abs(BKI-i)<2) c-=3;  
      if (abs(WKJ-j)<3&&abs(WKI-i)<3) c-=2;  
      if (abs(BKJ-j)<3&&abs(BKI-i)<3) c-=2;  
     }    
    } 
   }    
   if (nbw>1) c+=30; //два слона
   if (nbb>1) c-=30;
   for (char w=0;w<ipw;w++) { //поиск проходной белой пешки
    boolean pass=1;
    for (char b=0;b<ipb;b++) {
      if (pwi[w]>0&&pbi[b]==pwi[w]-1&&pbj[b]<pwj[w]) pass=0; //пешка слева
      if (pbi[b]==pwi[w]&&pbj[b]<pwj[w]) pass=0;  //пешка спереди
      if ( pwi[w]<7&&pbi[b]==pwi[w]+1&&pbj[b]<pwj[w])  pass=0; //пешка справа      
      if (!pass) break;
    }    
    if (pass) {      
      c+=50; 
      if (!endspiel) break; //в эндшпиле считать все проходные пешки
    }
   }
   for (char b=0;b<ipb;b++) { //поиск проходной черной пешки
    boolean pass=1;
    for (char w=0;w<ipw;w++) {
      if (pbi[b]>0&&pwi[w]==pbi[b]-1&&pwj[w]>pbj[b]) pass=0; //пешка слева
      if (pwi[w]==pbi[b]&&pwj[w]>pbj[b]) pass=0; //пешка спереди
      if (pbi[b]<7&&pwi[w]==pbi[b]+1&&pwj[w]>pbj[b]) pass=0; //пешка справа      
      if (!pass) break;
    }
    if (pass) {      
      c-=50; 
      if (!endspiel) break; //в эндшпиле считать все проходные пешки
    }
   }      
   return c;      
}
//****************************
int evaluate(int nstep) { //Вычисляем вес позиции пропорциональный
  long ww=0, wb=0;      
  for (char i=0;i<8;i++)
   for (char j=0;j<8;j++)     
    if (pole[j][i]<0) {
     wb+=fig_weight[-pole[j][i]]+(int)pgm_read_word(&pos[-pole[j][i]-1][7-j][i]);                               
    }
    else if (pole[j][i]>0) {
     ww+=fig_weight[pole[j][i]]+(int)pgm_read_word(&pos[pole[j][i]-1][j][i]);                              
    }   
  count++; //число оцененных позиций                
  long str=activity();   
  if (endspiel) { //для эндшпиля
   wb+=(int)pgm_read_word(&pos[6][7-BKJ][BKI])-(int)pgm_read_word(&pos[5][7-BKJ][BKI]);                               
   ww+=(int)pgm_read_word(&pos[6][WKJ][WKI])-(int)pgm_read_word(&pos[5][WKJ][WKI]);                                    
   if (wb<450&&ww>450) { //преимущество белых - ставим мат - штраф за расстояние между королями    
    str-=(abs(WKJ-BKJ)+abs(WKI-BKI))*30;
   } else if (ww<450&&wb>450) { //преимущество черных
    str+=(abs(WKJ-BKJ)+abs(WKI-BKI))*30;
   }
  } 
  if (nstep>8) { //проверка на повтор позиции
    if (steps[nstep-1].fig1==steps[nstep-5].fig1&&steps[nstep-2].fig1==steps[nstep-6].fig1&&
     steps[nstep-3].fig1==steps[nstep-7].fig1&&steps[nstep-4].fig1==steps[nstep-8].fig1)
      if (steps[nstep-1].x1==steps[nstep-5].x1&&steps[nstep-2].x1==steps[nstep-6].x1&&
       steps[nstep-3].x1==steps[nstep-7].x1&&steps[nstep-4].x1==steps[nstep-8].x1)
        if (steps[nstep-1].x2==steps[nstep-5].x2&&steps[nstep-2].x2==steps[nstep-6].x2&&
         steps[nstep-3].x2==steps[nstep-7].x2&&steps[nstep-4].x2==steps[nstep-8].x2)
          if (steps[nstep-1].y1==steps[nstep-5].y1&&steps[nstep-2].y1==steps[nstep-6].y1&&
           steps[nstep-3].y1==steps[nstep-7].y1&&steps[nstep-4].y1==steps[nstep-8].x1)
            if (steps[nstep-1].y2==steps[nstep-5].y2&&steps[nstep-2].y2==steps[nstep-6].y2&&
             steps[nstep-3].y2==steps[nstep-7].y2&&steps[nstep-4].y2==steps[nstep-8].y2)
               { Serial.println(F(" Draw - 3 repeat")); return 0;   }   
  }
  if (nstep%2==1) return 5000*(ww-wb)/(ww+wb+2000)+str; else return 5000*(wb-ww)/(ww+wb+2000)-str;    
}
//****************************  
void add_cut(int ind) {//добавление ind хода к массиву отсечек   
  int minbeta=30000, minindex=MAXCUTS-1;   
  for (char i=0;i<MAXCUTS;i++) {   
   if (cuts[i].weight==0) { minindex=i; break; }//пустое место 
   if (cuts[i].fig1==steps[ind].fig1&&
       cuts[i].x1==steps[ind].x1&&cuts[i].y1==steps[ind].y1&&
       cuts[i].x2==steps[ind].x2&&cuts[i].y2==steps[ind].y2) {
       cuts[i].weight++; return; //нашли - увеличиваем счетчик на 1
   }     
   if (cuts[i].weight<minbeta) { //место минимального индекса срабатывания
    minbeta=cuts[i].weight; 
    minindex=i;
   }      
  }
  cuts[minindex]=steps[ind];
  cuts[minindex].weight=1; //1 срабатывание
}
//****************************  
int quiescence(int start, int nstep, int alpha, int beta ) {    
    if (nstep-cur_step>=LIMDEPTH||start>MAXSTEPS-70) return evaluate(nstep);                
    if (!solving&&progress==0) return -5000;                
    int score=-20000;           
    start_var=start; 
    only_action=true;   
    load_variants(nstep);    
    if (!check_on_table) {
     int stand_pat = evaluate(nstep);
     if (stand_pat >= score) score=stand_pat; 
     if (score>alpha) alpha=score;
     if (alpha>=beta) return alpha;       
    }                     
    if (cur_var==0) { 
      if (check_on_table) {      
       if (TRACE) Serial.println(F("checkmate?"));                        
       return -10000+nstep-cur_step;                     
     } else return evaluate(nstep);
    }
    int j=start+cur_var;      
    for (int i=start;i<j;i++) { //перебор вариантов           
      if (TRACE) { //*****
       for (int u=0;u<nstep-cur_step;u++) Serial.print(F("   .   "));      
       Serial.println(str_step(i)); 
      }      
      movestep(i);            
      steps[nstep]=steps[i];              
      int tmp=-quiescence(j+1,nstep+1,-beta,-alpha);            
      backstep(i);              
      if (tmp>score) score=tmp;
      if (score>alpha) alpha=score;
      if (alpha>=beta )   {               
         add_cut(nstep);         
         return alpha;   
      }          
      if (checkmatesearch&&alpha>9000) break;     
      gui();                              
    }    
    return score;
}
//****************************  
int alphaBeta(int start, int nstep, int alpha, int beta, int depthleft) {
//start - начальный номер массива ходов для вариантов      
//nstep - номер полухода в партии
int score=-20000,best;
   if( depthleft==0) return quiescence(start,nstep,alpha,beta) ;      
   if (start>MAXSTEPS-70) return evaluate(nstep);              
   start_var=start; 
   only_action=false;         
   if (nstep!=cur_step) load_variants(nstep);     
   if (cur_var==0) { 
      if (check_on_table) {
       if (TRACE) Serial.println(F("checkmate!"));                 
       return -10000+nstep-cur_step;              
      } 
     return 0;
   }    
   int j=start+cur_var;     
   best=start;   
   for (int i=start;i<j;i++) { //перебор вариантов                 
      if (nstep==cur_step) {              
        Serial.print(str_step(i)); Serial.print("  ");
        Serial.print(i-start+1); Serial.print("/"); Serial.print(j-start);                        
        if (steps[i].weight<-9000) { Serial.println(F(" checkmate")); continue; }
        if (steps[i].fig2!=0||steps[i].check||alpha<-100) 
        //для взятий, шахов, первого хода и проигрыша -100 увеличенная глубина активного просмотра
         { LIMDEPTH=MAXDEPTH+2; Serial.print(F("+2")); } else LIMDEPTH=MAXDEPTH; 
      } else {
        if (TRACE) { //*****
         for (int u=0;u<nstep-cur_step;u++) Serial.print(F("       "));      
         Serial.println(str_step(i));
        } 
      }                  
      movestep(i);      
      steps[nstep]=steps[i];        
      int tmp=-alphaBeta(j+1,nstep+1,-beta,-alpha,depthleft-1);            
      backstep(i);
      steps[i].weight=tmp;
      if (tmp>score) score=tmp;
      if (score>alpha) {
        alpha=score;
        if (nstep>cur_step) add_cut(nstep); //добавляем ход к массиву отсечек       
        if (TRACE) { Serial.print(F("ALPHA+:")); Serial.println(score); }        
        best=i;         
        if (nstep==cur_step) {
         steps[0]=steps[best];           
         Serial.print(F(" BEST")); 
        }
      }
      if (alpha>=beta )   {        
       if (nstep>cur_step) add_cut(nstep); //добавляем ход к массиву отсечек                          
       if (TRACE) { Serial.print(F("BETA CUT:"));  Serial.println(score); }
       return alpha;            
      }   
      if (nstep==cur_step) {         
        Serial.print(F("        ")); Serial.println(tmp);        
        progress=100*(i-start+1)/(j-start);      
        if (alpha==9999||alpha==-5000) break;                     
        if (checkmatesearch&&alpha>9000&&limit<3) break;      
        if (!solving) {
         if (alpha>startweight&&cur_level>1) break;
          else { //ищем защиту от потери или мата - работаем до конца отведенного времени +50%
           if (BAction.bs==F("stopping")) break;                       
           if (cur_level>1&&100*(millis()-starttime)/(limittime-starttime)>240-limit*20) break;           
         }  
        }                   
      }       
      if (nstep==cur_step+1&&!solving&&progress==0) break;        
      gui();                                    
   }
   if (nstep==cur_step) { steps[nstep]=steps[best]; steps[0]=steps[best]; }                         
   return score;
}
//****************************  
void kingpositions() { 
 for (char i=0;i<8;i++) //положения королей
  for (char j=0;j<8;j++)     
   if (pole[j][i]==fk) {
      WKJ=j; WKI=i; 
   } else if (pole[j][i]==-fk) {
      BKJ=j; BKI=i; 
   }    
}
//****************************  
int get_endspiel() { //определение эндшпиля  
  int weight=0;  
  for (char i=0;i<8;i++)
   for (char j=0;j<8;j++)     
    if (pole[j][i]<0) 
     weight+=fig_weight[-pole[j][i]];                               
    else if (pole[j][i]>0) 
     weight+=fig_weight[pole[j][i]]; //полный вес стартовый 8000
  if (weight<3500) endspiel=true; else endspiel=false;  
  return weight;
}
//****************************  
boolean is_drawn() { //определяет ничью на доске
  boolean drawn=false;
  int cn=0,cbw=0,cbb=0,co=0,cb=0,cw=0;
  for (char i=0;i<8;i++)
   for (char j=0;j<8;j++)   {  
    if (abs(pole[j][i])==1) co++;  
    if (abs(pole[j][i])>3&&abs(pole[j][i])<6) co++; //подсчет пешек, ладей, ферзей  
    if (abs(pole[j][i])==6) continue;  //без королей
    if (abs(pole[j][i])==2) cn++; //число коней
    if (abs(pole[j][i])==3&&(i+j+2)%2==0) cbb++; //число белопольных слонов
    if (abs(pole[j][i])==3&&(i+j+2)%2==1) cbw++; //число чернопольных слонов 
    if (pole[j][i]==3) cw++; //число белых слонов   
    if (pole[j][i]==-3) cb++; //число черных слонов   
  }
  if (cn==1&&co+cbb+cbw==0) drawn=true;  //один конь
  if (cbb+cbw==1&&co+cn==0) drawn=true;  //один слон
  if (co+cn+cbb==0||co+cn+cbw==0) drawn=true; //однопольные слоны или два короля
  if (co+cn==0&&cb==1&&cw==1) drawn=true; //два слона разного цвета 
  if (drawn) return drawn;
  int rep=1;
  for (int s=cur_step-1; s>0; s--) {
    if (steps[s].fig1==0) continue;
    backstep(s); 
    boolean eq=true;    
    for (char i=0;i<8;i++)
     for (char j=0;j<8;j++) 
      if (pole[j][i]!=pole0[j][i]) 
       { eq=false; break; }
    if (eq) rep++;
    if (rep>2) break;    
  }  
  if (rep>1) { Serial.print(rep); Serial.println(" repetitions"); }
  for (char i=0;i<8;i++)
    for (char j=0;j<8;j++) pole[j][i]=pole0[j][i];   //восстанавливаем стартовую позицию        
  if (rep>2) drawn=true;
  return drawn;
}
//****************************  
int solve_step() { 
const int LMIN[6]={2,3,4,5,6,7};  
const int LMAX[6]={4,6,8,10,12,14};
const int LCMIN[6]={2,4,6,8,10,12};  
const int LCMAX[6]={4,6,8,10,12,14};
boolean check_on; 
 tft.fillRect(0,210,60,10,BLACK);             
 tft.drawFastHLine(5,210,200,DARK);   
 starttime=millis();    
 isstatus=1;
 deletebuttons();
 BAction.Show("STOP"); 
 BBW.Hide();   
 BBack.Hide();  
 BLimit.Hide();  
 BCM.Hide(); 
 BDesc.Hide(); 
 BSound.Hide(); 
 BRotate.Hide();  
 limittime=starttime+limits[limit]*1000; //ограничение времени обсчета в миллисекундах   
 for (char i=0;i<8;i++)
    for (char j=0;j<8;j++) pole0[j][i]=pole[j][i];   //сохраняем стартовую позицию       
 lastbest.fig1=0;    
 kingpositions(); 
 int wei=get_endspiel();     
 count=0; 
 startweight=evaluate(cur_step);
 Serial.println("");
 Serial.println(F("---------------"));
 if (endspiel) { Serial.print(F("Endspiel: ")); Serial.println(wei); }   
 if (cur_step%2==1) Serial.print(F("WHITE, ")); else Serial.print(F("BLACK, "));
 Serial.print(F("start Score= ")); Serial.println(startweight);
 start_var=cur_step+21;
 only_action=false;     
 lastbest.fig1=0;
 steps[0].fig1=0;
 load_variants(cur_step);       
 check_on=check_on_table; 
 if (cur_var==0||is_drawn()) {  
  beep(500);
  tft.drawFastHLine(5,210,200,BLACK);   
  tft.drawFastHLine(5,212,200,BLACK);   
  BAction.Hide();
  isstatus=0;
  solving=false;  
  if (check_on_table) return -9999; else return 8999;
 }
 if (cur_var==1) { //ход всего 1
  tft.drawFastHLine(5,210,200,BLACK);   
  tft.drawFastHLine(5,212,200,BLACK);   
  BAction.Hide();
  isstatus=0;
  solving=false;  
  steps[cur_step]=steps[cur_step+21];
  return startweight; 
 }          
 int vars=cur_var;   
 int ALPHA=-20000; 
 int BETA=20000;
 int score; 
 solving=true; 
 if (checkmatesearch) { 
  Serial.println(F("Checkmate search"));
  steps[cur_step].fig1=0; lastscore=0; show_steps();
 }
 int l=0;
 for (char i=0;i<MAXCUTS;i++) { cuts[i].weight=0; cuts[i].fig1=0; } //очистка массива отсечек 
 for (int i=cur_step+21;i<cur_step+21+vars;i++) {  //предварительная определение весов позиций
  movestep(i);
  steps[i].weight=evaluate(cur_step);
  if (steps[i].fig2!=0) steps[i].weight-=steps[i].fig1;
  backstep(i);
 } 
 while (l<6) {
  cur_level=l+1;  
  progress=0;
  tft.drawFastHLine(5,212,200,DARK);  
  MINDEPTH=LMIN[l];  
  MAXDEPTH=LMAX[l];        
  if (checkmatesearch) {   
    ALPHA=9000; BETA=10000; 
    MINDEPTH=LCMIN[l];
    MAXDEPTH=LCMAX[l];    
  }    
  if (l>0) Serial.println(""); Serial.print(F("******* LEVEL=")); Serial.print(l+1);   
  Serial.print(" "); Serial.print(MINDEPTH); Serial.print("-"); Serial.print(MAXDEPTH);     
  Serial.print("  "); Serial.print((millis()-starttime)/1000.,1); Serial.println("s");                                          
  if (checkmatesearch) {
    for (int i=cur_step+21;i<cur_step+21+vars;i++) { //вес -число ходов противника 
     movestep(i);   
     start_var=cur_step+22+vars;
     only_action=false;   
     load_variants(cur_step+1);     
     backstep(i);           
     steps[i].weight=-cur_var;     
    }
  }   
  sort_variants(cur_step+21,cur_step+20+vars);  //сортировка по весу позиции         
  cur_var=vars;
  check_on_table=check_on;
  score=alphaBeta(cur_step+21,cur_step,ALPHA,BETA,MINDEPTH);  
  if (score>9996) break;   
  if (score>9000&&limit<3) break;  
  if (100*(millis()-starttime)/(limittime-starttime)>70-l*8) break;          
  if (!solving) break;              
  l++;     
 } //while l    
 if (score<-9000) {   
   Serial.println(F("GIVE UP!")); 
 } else {
  Serial.print(F("STEP=")); Serial.println(str_step(cur_step));  
  if (score>9000) {   
   Serial.print(F("CHECKMATE ")); 
   if (score<9999) {
     Serial.print(F("in ")); Serial.print((9999-score)/2+1); Serial.print(F(" steps"));        
   } else steps[cur_step].check=2;
   Serial.println("");  
   if (checkmatesearch) {    
     steps[cur_step+9999-score].check=2;    
     steps[cur_step+10000-score].fig1=0;    
     for (int i=cur_step;i<cur_step+10000-score;i++) Serial.println(str_step(i));   
   }    
  } 
 }
 tft.drawFastHLine(5,210,200,BLACK);   
 tft.drawFastHLine(5,212,200,BLACK);  
 progress=0; 
 show_status();
 BAction.Hide(); 
 solving=0; 
 isstatus=0;
 return score;
 
 
}
//****************************  
