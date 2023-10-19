//***************************************************************************************************************************************
/*
*/
//***************************************************************************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include <SPI.h>
#include <SD.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "bitmaps_galaga.h"
#include "lcd_registers.h"

// El SPI es el 0
//MOSI va a PA_5
//MISO va a PA_4
//SCK va a PA_2

//*****************************************
// CONSTANTES
//*****************************************

const int pinBuzzer = PC_4;   //Pin Buzzer

const int Right = PUSH1;      //Constantes para botones
const int Left = PUSH2;

const int ALIVE = 1;          //Constantes para enemigos y balas
const int DEAD = 0;
const int SHOT = 1;
const int NOT_SHOT = 0;

const int START = 0;          //Constantes para estados de juego
const int DONE = 1;
const int MENU = 2;
const int CONFIG = 3;
const int LOAD = 4;
const int PLAY = 5;

#define LCD_RST PD_0
#define LCD_DC PD_1
#define LCD_CS PB_6 //PA_3

//*****************************************
// Estructuras
//*****************************************

// Estructura con los datos internos de los enemigos
struct enemy {
  int posX;                 //Posicion X
  int posY;                 //Posicion Y
  unsigned char direction;  //Direccion
  unsigned char space;      //Tamaño Sprite
  unsigned char speed;      //Velocidad
  unsigned char state;      //Estado (Vivo/Muerto)
};

//Estructura con los datos internos de las naves
struct ship {
  int posX;                 //Posicion X
  int posY;                 //Posicion Y
  unsigned char direction;  //Direccion
  int animation;            //Animacion
  unsigned char speed;      //Velocidad
  unsigned char lives;      //Vidas
  int score;                //Punteo
};

//Estructura con los datos internos de los disparos
struct bullet {
  int posX;                 //Posicion X
  int posY;                 //Posicion Y
  unsigned char speed;      //Velocidad
  unsigned char state;      //Estado (No disparada/Disparada)
  unsigned char shipId;     //Vinculo Nave
};

//Definicion de estructuras
//Enemigos
struct enemy bat1;
struct enemy bat2;
struct enemy bat3;
struct enemy fly1;
struct enemy fly2;
struct enemy fly3;
struct enemy shrimp1;
struct enemy shrimp2;
struct enemy shrimp3;
struct enemy rock1;
struct enemy rock2;
//Naves
struct ship ship1;
struct ship ship2;
//Balas
struct bullet bullet1;
struct bullet bullet2;
struct bullet bullet3;
struct bullet bullet4;
struct bullet bullet5;
struct bullet bullet6;
//METER A LA SD xd
extern unsigned char purpleBat[];
extern unsigned char greenFly[];
extern unsigned char blueShrimp[];
extern unsigned char fontSprite[];
extern unsigned char rock[];
extern unsigned char titleSprite[];
extern unsigned char menuSprite[];
extern unsigned char menuConfigSprite[];
extern unsigned char ship1Big[];
extern unsigned char ship2Big[];
extern unsigned char playerWinsSprite[];
extern unsigned char OneSprite[];
extern unsigned char TwoSprite[];
extern unsigned char playersWinSprite[];
//***************************************************************************************************************************************
// PROTOTIPOS DE FUNCIONES PARA LCD
//***************************************************************************************************************************************
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset);
//***************************************************************************************************************************************
// PROTOTIPOS DE FUNCIONES PROPIAS
//***************************************************************************************************************************************
void checkBoundary(int variable, int inf, int sup);   //Funcion para verificar limites
void checkDirection(int variable, int inf, int sup);  //Funcion para verificar direccion
void checkBulletCollision(struct bullet *bufBullet, struct enemy bufEnemy); //Funcion para verificar la colision entre las balas y los enemigos
void checkShipCollision(struct ship * bufShip, struct enemy * bufEnemy);  //Funcion para verificar la colision entre los enemigos y las naves
void checkAllBullets(struct enemy bufEnemy);  //Funcion para verificar todas las colisiones entre las balas y los enemigos
void scrollingEnemy(struct enemy bufStruct, int inf, int sup, unsigned char sprite[], unsigned char frames, unsigned char speed); //Funcion para enemigos en horizontal
void flyingEnemy(struct enemy *bufStruct, int inf, int sup, unsigned char sprite[], unsigned char frames);  //Funcion para enemigos en vertical
void bulletControl(struct bullet *bufBullet, unsigned char offset, unsigned char bulletType); //Funcion para mostrar balas en pantalla
void star(int number); //Funcion para imprimir una estrella :D
void melody();                  //Melodia
void bulletSound (void);        //Sonido de disparo
void deathSound (void);         //Sonido de muerte
void winSound (void);           //Sonido de victoria
void resetAllBullets (void);    //Reset balas
void resetEnemies(void);        //Reset enemigos

void BitmapFromSD(unsigned int x, unsigned int y, unsigned int width, unsigned int height, char fileName[]);
uint8_t StrToHex(char str[]);
//Secuencias de enemigos
void sequence1(void);
void sequence2(void);
void sequence3(void);
void sequence4(void);
//***************************************************************************************************************************************
// Valores Notas requeridas
int Sol3 = 196;    // Frecuencia de Sol en octava 3 (G3)
int Si3_b = 233.08;// Frecuencia de Si bemol en octava 3 (Bb3/A#3)
int Do = 261;      // Frecuencia de Do (C4)
int Re = 294;      // Frecuencia de Re (D4)
int Mi = 330;      // Frecuencia de Mi (E4)
int Fa = 349;      // Frecuencia de Fa (F4)
int Sol = 392;     // Frecuencia de Sol (G4)
int La_b = 466;    // Frecuencia de La bemol (Ab4)
int Si_b = 466;    // Frecuencia de Si bemol (A#4/Bb4)
int Si = 494;      // Frecuencia de Si (B4)
int Mi_b = 311.13; // Frecuencia de Mi bemol (Eb4/D#4)
int Do5 = 523;     // Frecuencia de Do en octava 5 (C5)
int La = 440;      // Frecuencia de La (A4)
int Re5 = 587;  // Frecuencia de Re (Re5)
int Mi5 = 659;  // Frecuencia de Mi (Mi5)
int Fa5 = 698;  // Frecuencia de Fa (Fa5)
int Sol5 = 784; // Frecuencia de Sol (Sol5)
int La5 = 880;  // Frecuencia de La (La5)
int Do6 = 1046.50; // Frecuencia de Do en octava 6 (C6)
// Calcula las duraciones de otras notas en función de la negra
int negra = 500; // Duración de una negra en milisegundos
int corchea = negra / 2;
int pausa = negra / 5; // Duración de una pausa en milisegundos
// Array con la melodia del menu
int melodia[] = {
  Sol3, Do, Re, Fa, Mi, Do, Re, La,
  Sol, Do, Re, Fa, Mi, Do, Sol, Si,
  Do5, Si_b, La_b, Sol, Fa, Mi_b, Re, Si3_b,
  Si_b, Do5, Si, Sol, La, Fa, Re, Sol, Mi, Re
};
// Array con la melodia de ganador
int melodiaGanador[] = {
  Sol, Sol, Sol, Mi, Sol, La, Fa,
  Mi, Fa, Do, La, Si, La,
  Sol, Mi5, La5, Sol5, Fa5, Mi5,
  Do5, Do5, Do5, Si, Re5, Do5
};
int songCounter = 0;    //Contador interno para melodia
int noteCounter = 0;    //Contador para el numero de nota de melodia
int delayCounter = 0;
int pauseCounter = 0;
int musicType = 0;
//***************************************************************************************************************************************
// Variables globales

// como le puedo declarar mi amor si lo unico que se es declarar variables

int internalTimer = 0;  //Contador interno

//Variables para menus y switches
int sequenceCase;       //Nivel
int gameState = START;  //Estado de juego
int menuOption = 1;     //Opciones menu
int menuConfig = 1;     //Opciones menu de configuracion
int gameType = 0;       //Tipo de juego (1P, PVP, COOP)

//Variables a configurar
int bulletSpeed = 3;
int enemySpeedPlus = 0;

int coopScore = 0;      //Punteo cooperativo
int onChange = 1;       //Variable para verificar push

//Variables para controles inalambricos
const int midValue = 7;

int joystickXPlayer1 = midValue;
int joystickYPlayer1 = midValue;
int pushPlayer1 = 0;

int joystickXPlayer2 = midValue;
int joystickYPlayer2 = midValue;
int pushPlayer2 = 0;


long debounceDelay = 200;

//int enemy1Pos = 10, enemy1Direction = 1;    mi pobre alma no sabia acerca de las estructuras en las primeras horas de programacion

// Variables para la SD
File myFile;
char input[2];
uint8_t pixel1, pixel2;

//BOTONES IGNORAR ELIMINAR!!!!!!!!!!!!!!!!!!!!!!!!
int lastButton1State = LOW;
int lastButton2State = LOW;
int button1State;
int button2State;

//!!!!!!!!!!!!!!!!!!!!!!


//***************************************************************************************************************************************
// Inicialización
//***************************************************************************************************************************************
void setup() {

  SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
  Serial.begin(115200);
  SPI.setModule(0);
  pinMode(Left, INPUT_PULLUP);
  pinMode(Right, INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(PA_3, OUTPUT);    //CS SPI0
  pinMode(PB_6, OUTPUT);


  if (!SD.begin(12)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  randomSeed(analogRead(18));
  sequenceCase = random(3);
  Serial.println("Inicio");
  LCD_Init();
  LCD_Clear(0x0000);
  //LCD_Bitmap(0,0,320,240, background1);



  delay(1000);

  //Inicializar las estructuras

  //Iniciar los enemigos
  resetEnemies();
  //Iniciar primer nave
  ship1.posX = 120;
  ship1.posY = 10;
  ship1.direction = 0;
  ship1.animation = 0;
  ship1.speed = 3;
  ship1.lives = 3;
  ship1.score = 0;

  //Iniciar segunda nava
  ship2.posX = 120;
  ship2.posY = 30;
  ship2.direction = 0;
  ship2.animation = 0;
  ship2.speed = 2;
  ship2.lives = 3;
  ship2.score = 0;

  bullet1.shipId = 1;
  bullet2.shipId = 1;
  bullet3.shipId = 1;
  bullet4.shipId = 2;
  bullet5.shipId = 2;
  bullet6.shipId = 2;

  resetAllBullets();



}
//***************************************************************************************************************************************
// Loop Infinito
//***************************************************************************************************************************************
void loop() {
  int reading1 = digitalRead(Left);
  int reading2 = digitalRead(Right);
  int ship1Frame, ship2Frame;

  //ELIMINAR DESPUES
  if (lastButton1State == HIGH && reading1 == LOW) {
    button1State = HIGH;
  } else if (lastButton1State == LOW && reading1 == HIGH) {
    button1State = LOW;
  }

  if (lastButton2State == HIGH && reading2 == LOW) {
    button2State = HIGH;
  } else if (lastButton2State == LOW && reading2 == HIGH) {
    button2State = LOW;
  }

  // Implementacion Joysticks ******************************************

  uint8_t temp;   //Variable para almacenar el valor recibido

  if (Serial.available()) {
    temp = Serial.read();
    // Si hay datos disponibles, léelos
    if (temp > 15) {
      uint8_t code = temp >> 4;  // Los 4 bits más significativos
      uint8_t value = temp & 0x0F; // Los 4 bits menos significativos

      switch (code) {
        case 0xA: //Botón Jugador1
          pushPlayer1 = 1;
          break;
        case 0xB: //Botón Jugador2
          pushPlayer2 = 1;
          break;

        case 0x4: // Jugador 1 en X
          joystickXPlayer1 = value;
          break;

        case 0x1: //Jugador 1 en Y
          joystickYPlayer1 = value;
          break;

        case 0x2: //Jugador 2 en X
          joystickXPlayer2 = value;
          break;

        case 0x3: //Jugador 2 en Y
          joystickYPlayer2 = value;
          break;

        default:
          joystickXPlayer1 = 7;
          joystickYPlayer1 = 7;
          pushPlayer1 = 0;

          joystickXPlayer2 = 7;
          joystickYPlayer2 = 7;
          pushPlayer2 = 0;
          break;
      }
    }
  }

  // *******************************************************************
  switch (gameState) {
    //***************************************************************************************************************************************
    case START:     //EMPIEZA A CARGAR EL JUEGO PANTALLA DE INICIO GALAGA
      LCD_Clear(0x0000);  //BORRA PANTALLA
      LCD_Bitmap(140, 85, 40, 70, titleSprite);   //Pantalla de Galaga inicial
      delay(1000);
      star(15);  //Estrellas iniciales
      delay(200);
      for (int i = 0; i < 60; i++) LCD_Bitmap(140 + i, 85, 40, 70, titleSprite);  //Movimiento Galaga inicial
      LCD_Bitmap(120, 80, 71, 94, menuSprite);  //Se imprime el menu 1 vez
      //BitmapFromSD(120, 80, 71, 94, "test.txt");
      gameState = MENU; //Cambiamos al menu
      break;
    //***************************************************************************************************************************************
    case DONE:      //ESTADO CUANDO EL JUEGO TERMINA/REINICIO JUEGO
      delay(1500);
      LCD_Clear(0x0000);  //BORRA PANTALLA
      star(20);  //Estrellas :)
      delay(100);
      for (int i = 0; i < 60; i++) LCD_Bitmap(140 + i, 85, 40, 70, titleSprite);  //Movimiento Galaga inicial
      LCD_Bitmap(120, 80, 71, 94, menuSprite);  //Se imprime el menu 1 vez
      onChange = 1;
      gameState = MENU; //Cambiamos al menu
      break;
    //***************************************************************************************************************************************
    case MENU:    //ESTADO PARA EL MENU
      melody();

      if (onChange == 1) {    //Si cambiamos de opcion
        switch (menuOption) {
          case 1:
            LCD_Bitmap(180, 60, 11, 11, arrow);   //Imprime la flecha del menu para cada caso
            FillRect(160, 60, 11, 11, 0x0000);    //Imprime cuadrados en los otros lugares
            FillRect(140, 60, 11, 11, 0x0000);
            FillRect(120, 60, 11, 11, 0x0000);
            break;
          case 2:
            FillRect(180, 60, 11, 11, 0x0000);
            LCD_Bitmap(160, 60, 11, 11, arrow);
            FillRect(140, 60, 11, 11, 0x0000);
            FillRect(120, 60, 11, 11, 0x0000);
            break;
          case 3:
            FillRect(180, 60, 11, 11, 0x0000);
            FillRect(160, 60, 11, 11, 0x0000);
            LCD_Bitmap(140, 60, 11, 11, arrow);
            FillRect(120, 60, 11, 11, 0x0000);
            break;
          case 4:
            FillRect(180, 60, 11, 11, 0x0000);
            FillRect(160, 60, 11, 11, 0x0000);
            FillRect(140, 60, 11, 11, 0x0000);
            LCD_Bitmap(120, 60, 11, 11, arrow);
            break;
        }
        onChange = 0;
      }

      if (button1State == HIGH || joystickYPlayer1 < midValue || joystickYPlayer2 < midValue) {
        delay(debounceDelay);
        menuOption++;
        onChange = 1;
      }
      else if (joystickYPlayer1 > midValue || joystickYPlayer2 > midValue) {
        delay(debounceDelay);
        menuOption--;
        onChange = 1;
      }

      if (menuOption > 4) menuOption = 1;
      if (menuOption < 1) menuOption = 4;

      if (button2State == HIGH || pushPlayer1 == 1 || pushPlayer2 == 1) { //  Elige la opcion seleccionada por la flecha
        delay(debounceDelay);
        switch (menuOption) {
          case 1:            //MODO 1PLAYER
            gameType = 1;
            gameState = LOAD; //Cambiamos a LOAD para comenzar a jugar
            break;
          case 2:            //MODO PVP
            gameType = 2;
            gameState = LOAD; //Cambiamos a LOAD para comenzar a jugar
            break;
          case 3:            //MODO COOP
            gameType = 3;
            gameState = LOAD; //Cambiamos a LOAD para comenzar a jugar
            break;
          case 4:            //CONFIG
            noTone(pinBuzzer);
            onChange = 1;
            LCD_Clear(0x0000);    //BORRA PANTALLA
            star(20);  //Imprime estrellas
            LCD_Bitmap(120, 80, 71, 88, menuConfigSprite);  //Imprime el menu de configuracion 1 vez
            gameState = CONFIG; //Cambiamos a CONFIG para configurar velocidades
            break;
        }
      }

      break;
    //***************************************************************************************************************************************
    case CONFIG:    //ESTADO PARA EL MENU DE CONFIGURACION
      //Imprime las configuraciones actuales
      LCD_Sprite(160, 180, 11, 11, fontSprite, 10, bulletSpeed, 0, 0);          //Velocidad de las balas
      LCD_Sprite(140, 180, 11, 11, fontSprite, 10, ship1.speed, 0, 0);          //Velocidad de las naves
      LCD_Sprite(120, 180, 11, 11, fontSprite, 10, enemySpeedPlus + 1, 0, 0);   //Velocidad de los enemigos

      if (onChange == 1) {
        //Mismo menu de arriba
        switch (menuConfig) {
          case 1:
            LCD_Bitmap(160, 60, 11, 11, arrow);
            FillRect(140, 60, 11, 11, 0x0000);
            FillRect(120, 60, 11, 11, 0x0000);
            break;
          case 2:
            FillRect(160, 60, 11, 11, 0x0000);
            LCD_Bitmap(140, 60, 11, 11, arrow);
            FillRect(120, 60, 11, 11, 0x0000);
            break;
          case 3:
            FillRect(160, 60, 11, 11, 0x0000);
            LCD_Bitmap(120, 60, 11, 11, arrow);
            FillRect(140, 60, 11, 11, 0x0000);
            break;
        }
        onChange = 0;
      }

      if (button1State == HIGH || joystickYPlayer1 < midValue || joystickYPlayer2 < midValue) {
        delay(debounceDelay);
        menuConfig++;
        onChange = 1;
      }
      else if (joystickYPlayer1 > midValue || joystickYPlayer2 > midValue) {
        delay(debounceDelay);
        menuConfig--;
        onChange = 1;
      }

      if (menuConfig > 3) menuOption = 1;
      if (menuConfig < 1) menuOption = 4;

      if (joystickXPlayer1 < midValue || joystickXPlayer2 < midValue) {
        gameState = DONE;
      }

      if (button2State == HIGH || pushPlayer1 == 1 || pushPlayer2 == 1) {   //Mismo switch para seleccionar
        delay(debounceDelay);
        switch (menuConfig) {
          case 1: //Cambiar velocidad disparo
            bulletSpeed++;
            if (bulletSpeed > 4) bulletSpeed = 1;
            bullet1.speed = bulletSpeed;
            bullet2.speed = bulletSpeed;
            bullet3.speed = bulletSpeed;
            bullet4.speed = bulletSpeed;
            bullet5.speed = bulletSpeed;
            bullet6.speed = bulletSpeed;
            break;
          case 2: //Cambiar velocidad naves
            ship1.speed++;
            if (ship1.speed > 5) ship1.speed = 1;
            ship2.speed++;
            if (ship2.speed > 5) ship2.speed = 1;
            break;
          case 3: //Cambiar velocidad de los enemigos
            enemySpeedPlus++;
            if (enemySpeedPlus > 3) enemySpeedPlus = 0;
            break;
        }
      }
      break;
    //***************************************************************************************************************************************
    case LOAD:    //ESTADO PARA EMPEZAR A JUGAR
      noTone(pinBuzzer);
      resetAllBullets();    //Reset balas
      resetEnemies();       //Reset Enemigos
      coopScore = 0;       //Reset Scores
      ship1.score = 0;
      ship2.score = 0;
      ship1.lives = 3;      //Reset Vidas
      ship2.lives = 3;
      LCD_Clear(0x0000);    //BORRA PANTALLA
      star(15);   //Estrellas :D
      for (int k = 0; k < ship1.lives; k++) LCD_Bitmap(296, 8 + 17 * k, 16, 16, ship1Sprite);     //Imprimir vidas iniciales Nave 1
      if (gameType != 1) {                                                                        //Si el modo no es de 1 jugador
        for (int k = 0; k < ship2.lives; k++) LCD_Bitmap(296, 216 - 17 * k, 16, 16, ship2Sprite); //Imprimir vidas iniciales Nave 2
      }
      if (gameType != 3) {                                                            //Si el modo no es cooperativo
        LCD_Sprite(282, 8, 11, 11, fontSprite, 10, ship1.score / 10, 0, 0);           //Imprimir punteo Nave1
        LCD_Sprite(282, 8 + 10, 11, 11, fontSprite, 10, ship1.score % 10, 0, 0);
        LCD_Sprite(282, 8 + 20, 11, 11, fontSprite, 10, 0, 0, 0);
        LCD_Sprite(282, 8 + 30, 11, 11, fontSprite, 10, 0, 0, 0);

        if (gameType != 1) {                                                          //Si el modo no es cooperativo y tampoco 1 jugador
          LCD_Sprite(282, 190, 11, 11, fontSprite, 10, ship2.score / 10, 0, 0);       //Imprimir punteo Nave 2
          LCD_Sprite(282, 190 + 10, 11, 11, fontSprite, 10, ship2.score % 10, 0, 0);
          LCD_Sprite(282, 190 + 20, 11, 11, fontSprite, 10, 0, 0, 0);
          LCD_Sprite(282, 190 + 30, 11, 11, fontSprite, 10, 0, 0, 0);
        }
      } else {                                                                        //Si el modo es cooperativo
        LCD_Sprite(282, 98, 11, 11, fontSprite, 10, coopScore / 10, 0, 0);            //Imprimir punteo cooperativo
        LCD_Sprite(282, 98 + 10, 11, 11, fontSprite, 10, coopScore % 10, 0, 0);
        LCD_Sprite(282, 98 + 20, 11, 11, fontSprite, 10, 0, 0, 0);
        LCD_Sprite(282, 98 + 30, 11, 11, fontSprite, 10, 0, 0, 0);
      }
      gameState = PLAY;   //Cambiamos de modo para el juego principal
      break;
    //***************************************************************************************************************************************

    case PLAY:
      internalTimer++;      //Contador interno para estrellas
      if (internalTimer >= 1000) internalTimer = 0;
      if (internalTimer % 200 == 0) star(1);   //Estrella :D

      //Movimiento Nave 1 =======================================================================
      if (joystickXPlayer1 > midValue || button1State == HIGH) {
        if (joystickXPlayer1 > 14) ship1.speed++;
        ship1.posX = ship1.posX + ship1.speed;    //Incrementar posicion Nave 1
        ship1.direction = 0;
      }
      if (joystickXPlayer1 < midValue || button2State == HIGH) {
        if (joystickXPlayer1 < 1) ship1.speed++;
        ship1.posX = ship1.posX - ship1.speed;      //Decrementar posicion Nave 1
        ship1.direction = 1;                        //Direccion Nave 1 Izquierda
      }

      if (joystickXPlayer1 > midValue || joystickXPlayer1 < midValue || button1State == HIGH || button2State == HIGH) { //ANIMACION PARA PRIMER NAVE
        ship1.animation = ship1.animation + ship1.speed;    //Si la nave se esta moviendo aumentar su contador para la animacion
      }  else {
        ship1.animation = ship1.animation - ship1.speed;    //Si la nave no se esta moviendo decrementar el contador para animacion
      }

      //Movimiento Nave 2 =======================================================================
      if (gameType != 1) {                              //Si el modo es de 1 jugador no lo hace
        if (joystickXPlayer2 > midValue || button1State == HIGH) {
          if (joystickXPlayer2 > 14) ship2.speed++;
          ship2.posX = ship2.posX + ship2.speed;    //Incrementar posicion Nave 2
          ship2.direction = 0;                      //Direccion Nave 2 a la derecha
        }
        if (joystickXPlayer2 < midValue || button2State == HIGH) {
          if (joystickXPlayer2 < 1) ship2.speed++;
          ship2.posX = ship2.posX - ship2.speed;      //Decrementar posicion Nave 2
          ship2.direction = 1;                        //Direccion Nave 2 Izquierda
        }

        if (joystickXPlayer2 > midValue || joystickXPlayer2 < midValue || button1State == HIGH || button2State == HIGH) { //ANIMACION PARA SEGUNDA NAVE
          ship2.animation = ship2.animation + ship2.speed;  //Si la nave se esta moviendo aumentar su contador para la animacion
        }  else {
          ship2.animation = ship2.animation - ship2.speed;  //Si la nave no se esta moviendo decrementar el contador para animacion
        }

      }
      //Balas Nave 1 =======================================================================
      if (pushPlayer1 == 1 || button1State == HIGH) {
        if (bullet1.state == NOT_SHOT) bullet1.posX = ship1.posX + 8; //Si la bala no ha sido disparada... Dispararla
        if (bullet2.state == NOT_SHOT) bullet2.posX = ship1.posX + 8;
        if (bullet3.state == NOT_SHOT) bullet3.posX = ship1.posX + 8;
        if (bullet3.posY > 60 || bullet3.posY == 25) {      //Si la bala anterior no ha llegado tan lejos no dispara
          if (bullet1.state != SHOT) bulletSound();         //Sonido cuando dispare
          bullet1.state = SHOT;
        }
        if (bullet1.posY > 60) {
          if (bullet2.state != SHOT) bulletSound();
          bullet2.state = SHOT;
        }
        if (bullet2.posY > 60) {
          if (bullet3.state != SHOT) bulletSound();
          bullet3.state = SHOT;
        }
      }
      //Balas Nave 2 =======================================================================

      if (gameType != 1) {
        if (pushPlayer2 == 1 || button2State == HIGH) {
          if (bullet4.state == NOT_SHOT) bullet4.posX = ship2.posX + 8; //Si la bala no ha sido disparada... Dispararla
          if (bullet5.state == NOT_SHOT) bullet5.posX = ship2.posX + 8;
          if (bullet6.state == NOT_SHOT) bullet6.posX = ship2.posX + 8;
          if (bullet6.posY > 80 || bullet6.posY == 45) {      //Si la bala anterior no ha llegado tan lejos no dispara
            if (bullet4.state != SHOT) bulletSound();           //Sonido cuando dispare
            bullet4.state = SHOT;
          }
          if (bullet4.posY > 80) {
            if (bullet5.state != SHOT) bulletSound();
            bullet5.state = SHOT;
          }
          if (bullet5.posY > 80) {
            if (bullet6.state != SHOT) bulletSound();
            bullet6.state = SHOT;
          }
        }
      }

      //***************************************************************************************************************************************
      // MOVIMIENTO NAVES
      //***************************************************************************************************************************************
      checkBoundary(&ship1.posX, 0, 239 - 15);    //Limites para el movimiento de la Nave 1
      checkBoundary(&ship1.animation, 0, 34);     //Limites para la animacion de la Nave 1
      if (gameType != 1) {                        //Si el modo no es 1 jugador
        checkBoundary(&ship2.posX, 0, 239 - 15);  //Limites para el movimiento de la Nave 2
        checkBoundary(&ship2.animation, 0, 34);   //Limites para la animacion de la Nave 2
      }
      ship1Frame = (ship1.animation / 7) % 5;          //
      ship2Frame = (ship2.animation / 7) % 5;
      //Nave 1
      if (ship1.direction == 0) {
        LCD_Sprite(ship1.posY, ship1.posX, 16, 16, ship1Right, 5, ship1Frame, 0, 0);    //Animacion para movimiento a la derecha
      } else {
        LCD_Sprite(ship1.posY, ship1.posX, 16, 16, ship1Left, 5, ship1Frame, 0, 0);     //Animacion para movimiento a la izquierda
      }
      for (int i = 1; i <= ship1.speed; i++) {                    //Se imprimen mas lineas dependiendo de la velocidad para eliminar el rastro
        H_line(ship1.posY, ship1.posX - i, 16, 0x0000);
        H_line(ship1.posY, ship1.posX + 15 + i, 16, 0x0000);
      }

      if ((gameType != 1)) {                      //Si el modo no es 1 jugador
        //Nave 2
        if (ship2.direction == 0) {
          LCD_Sprite(ship2.posY, ship2.posX, 16, 16, ship2Right, 5, ship2Frame, 0, 0);  //Animacion para movimiento a la derecha
        } else {
          LCD_Sprite(ship2.posY, ship2.posX, 16, 16, ship2Left, 5, ship2Frame, 0, 0);   //Animacion para movimiento a la izquierda
        }
        for (int i = 1; i <= ship2.speed; i++) {                  //Se imprimen mas lineas dependiendo de la velocidad para eliminar el rastro
          H_line(ship2.posY, ship2.posX - i, 16, 0x0000);
          H_line(ship2.posY, ship2.posX + 15 + i, 16, 0x0000);
        }
      }
      //***************************************************************************************************************************************
      // MOVIMIENTO DISPAROS
      //***************************************************************************************************************************************
      //Disparos primer nave
      if (bullet1.state == SHOT) bulletControl(&bullet1, 0, 0);
      if (bullet2.state == SHOT) bulletControl(&bullet2, 0, 0);
      if (bullet3.state == SHOT) bulletControl(&bullet3, 0, 0);
      if (gameType != 1) {
        //Disparos segunda nave
        if (bullet4.state == SHOT) bulletControl(&bullet4, 20 , 1);
        if (bullet5.state == SHOT) bulletControl(&bullet5, 20, 1);
        if (bullet6.state == SHOT) bulletControl(&bullet6, 20, 1);
      }
      //***************************************************************************************************************************************
      // ENEMIGOS
      //***************************************************************************************************************************************
      switch (sequenceCase) {
        case 0: sequence1();
          break;
        case 1: sequence2();
          break;
        case 2: sequence3();
          break;
        case 3: sequence4();
          break;
          //case 4: sequence5();
          //break;
          //case 5: sequence6();
          //break;
      }
      //***************************************************************************************************************************************
      // SECUENCIA PARA VERIFICAR QUIEN GANA
      //***************************************************************************************************************************************
      if (gameType != 3) {    //Si el modo no es cooperativo
        if (ship1.lives == 0 || ship2.score >= 15 && gameType != 1) {
          //GANA EL JUGADOR 2
          LCD_Clear(0x0000); //BORRA PANTALLA
          star(15);  //Estrella :D
          for (int j = 0; j < 120 - 32; j++) {  //Muestra Nave 2 en movimiento
            H_line(30, j - 1, 64, 0x0000);
            LCD_Bitmap(30, j, 64, 64, ship2Big);
          }
          winSound();                           //Sonido victoria
          LCD_Bitmap(180, 10, 64, 181, playerWinsSprite); //Imprime "PLAYER 2 WINS"
          LCD_Bitmap(180 + 33, 200, 31, 27, TwoSprite);
          delay(2000);
          LCD_Clear(0x0000);  //BORRA PANTALLA
          gameState = DONE;   //Estado DONE para reiniciar juego

        } else if (ship2.lives == 0 || ship1.score >= 15) {
          //GANA EL JUGADOR 1
          LCD_Clear(0x0000); //BORRA PANTALLA
          star(15);  //  Estrella :)
          for (int j = 0; j < 120 - 32; j++) {  //Muestra Nave 1 en movimiento
            H_line(30, j - 1, 64, 0x0000);
            LCD_Bitmap(30, j, 64, 64, ship1Big);
          }
          winSound();                           //Sonido victoria
          LCD_Bitmap(180, 10, 64, 181, playerWinsSprite); //Imprime "PLAYER 1 WINS"
          LCD_Bitmap(180 + 34, 200, 31, 27, OneSprite);
          delay(2000);
          LCD_Clear(0x0000);
          gameState = DONE;   //Estado DONE para reiniciar juego
        }
      } else {
        if (ship1.lives == 0 && ship2.lives == 0) {
          //PERDEMOS EL MODO COOPERATIVO
          LCD_Clear(0xa800);  //LA PANTALLA DE PERDER PARTIDA SE POSPONE HASTA QUE ME DEN GANAS, AUNQUE LA PANTALLA ROJA SE MIRA COOL
          delay(1500);
          LCD_Clear(0x0000);  //BORRA PANTALLA
          gameState = DONE;   //Estado DONE para reiniciar juego
        }
        else if (coopScore == 25) {
          //GANAMOS EL MODO COOPERATIVO
          LCD_Clear(0x0000);  //BORRA PANTALLA
          star(15);
          for (int j = 0; j < 70 - 32; j++) { //Muestra ambas Naves en movimiento
            H_line(30, j - 1, 64, 0x0000);
            LCD_Bitmap(30, j, 64, 64, ship1Big);
          }
          for (int j = 110; j < 180 - 32; j++) {
            H_line(30, j - 1, 64, 0x0000);
            LCD_Bitmap(30, j, 64, 64, ship2Big);
          }
          winSound();                         //Sonido victoria
          LCD_Bitmap(180, 13, 64, 213, playersWinSprite); //Imprime "PLAYERS WIN"
          delay(2000);
          LCD_Clear(0x0000);  //BORRA PANTALLA
          gameState = DONE;   //Estado DONE para reiniciar juego
        }
        break;
      }
  }
  lastButton1State = reading1;
  lastButton2State = reading2;

  pushPlayer1 = 0;
  pushPlayer2 = 0;
  if (joystickXPlayer1 > 14) ship1.speed--;
  if (joystickXPlayer1 < 1) ship1.speed--;
  if (joystickXPlayer2 > 14) ship2.speed--;
  if (joystickXPlayer2 < 1) ship2.speed--;

}
//***************************************************************************************************************************************
// Funciones propias
//***************************************************************************************************************************************

// Funcion para verificar limites
void checkBoundary(int* var, int inf, int sup) {
  if (*var > sup) {
    *var = sup;
  } else if (*var < inf) {
    *var = inf;
  }
}

//Funcion para verificar direccion
void checkDirection(int var, unsigned char* directionVar, int inf, int sup) {
  if (var > sup) {
    *directionVar = 0;
  }
  if (var < inf) {
    *directionVar = 1;
  }
}

//Funcion para verificar la colision entre las balas y los enemigos
/*  Mandamos a llamar a cada enemigo o bala por medio de punteros de estructuras y el indicador -> para propiedades especificas de la estructura
   1. Si la bala esta disparada
   2. Si la hitbox de la bala se encuentra dentro del rango del enemigo, tanto en x como en y
    Tomando la posicion de la bala y sumandole su tamaño para limite inferior y compararlo con la posicion del enemigo
    Tomando la posicion de la bala para limite superior y compararlo con la posicion del enemigo mas su tamaño
   3. Dependiendo del modo de juego sumamos al contador de puntos de cada nave o al contador de puntos cooperatos
    Por medio de una propiedad de cada bala sabemos a que nave pertenece cada bala
   4. Cuando se genera una colision debemos generar un cuadrado negro sobre la bala y el enemigo
      Luego se imprime un 100 sobre el enemigo y se imprime un cuadrado negro de nuevo
   5. Cambiamos el estado de la bala a "no disparada" y el estado del enemigo a "muerto"
   6. Reiniciamos posicion de la bala
*/
void checkBulletCollision(struct bullet * bufBullet, struct enemy * bufEnemy) {
  if (bufBullet->state == SHOT) {
    if (bufEnemy->posX <= bufBullet->posX + 3 && bufBullet->posX <= (bufEnemy->posX + bufEnemy->space)) {
      if (bufEnemy->posY <= bufBullet->posY + 7 && bufBullet->posY + 7 <= (bufEnemy->posY + bufEnemy->space)) {
        int k;
        if (k == 1) {
          for (int i = 200; i <= 800; i += 200) {
            tone(pinBuzzer, i, 100);  // Tocar una nota con una frecuencia descendente y corta duración
            delay(10);  // Pequeña pausa entre las notas
            noTone(pinBuzzer);  // Detener el sonido
            k = 0;
          }
        } else {
          for (int i = 300; i <= 700; i += 100) {
            tone(pinBuzzer, i, 100);  // Tocar una nota con una frecuencia descendente y corta duración
            delay(10);  // Pequeña pausa entre las notas
            noTone(pinBuzzer);  // Detener el sonido
            k = 1;
          }
        }

        if (bufBullet->shipId == 1) {
          if (gameType != 3) {
            ship1.score++;
            LCD_Sprite(282, 8, 11, 11, fontSprite, 10, ship1.score / 10, 0, 0);
            LCD_Sprite(282, 8 + 10, 11, 11, fontSprite, 10, ship1.score % 10, 0, 0);
          } else {
            coopScore++;
            LCD_Sprite(282, 98, 11, 11, fontSprite, 10, coopScore / 10, 0, 0);
            LCD_Sprite(282, 98 + 10, 11, 11, fontSprite, 10, coopScore % 10, 0, 0);
          }


        } else {
          if (gameType != 3) {
            ship2.score++;
            LCD_Sprite(282, 190, 11, 11, fontSprite, 10, ship2.score / 10, 0, 0);
            LCD_Sprite(282, 190 + 10, 11, 11, fontSprite, 10, ship2.score % 10, 0, 0);
          } else {
            coopScore++;
            LCD_Sprite(282, 98, 11, 11, fontSprite, 10, coopScore / 10, 0, 0);
            LCD_Sprite(282, 98 + 10, 11, 11, fontSprite, 10, coopScore % 10, 0, 0);
          }
        }

        FillRect(bufBullet->posY, bufBullet->posX, 7, 3, 0x0000);
        bufBullet->state = NOT_SHOT;
        bufEnemy->state = DEAD;
        FillRect(bufEnemy->posY, bufEnemy->posX, bufEnemy->space, bufEnemy->space, 0x0000);
        LCD_Bitmap(bufEnemy->posY, bufEnemy->posX, 13, 13, reward);
        delay(200);
        FillRect(bufEnemy->posY, bufEnemy->posX, bufEnemy->space, bufEnemy->space, 0x0000);
        if (bufBullet->shipId == 1) {
          bufBullet->posY = 25;
        } else {
          bufBullet->posY = 45;
        }
      }
    }
  }
}

//Funcion para verificar la colision entre los enemigos y las naves
/*
   El proceso es muy similar al de las balas y enemigos pero en este caso utilizamos las estructuras de las anves y los enemigos
   En este caso restamos vidas e igual matamos a los enemigos
   Tambien se reproduce un sonido de muerte
*/
void checkShipCollision(struct ship * bufShip, struct enemy * bufEnemy) {
  if (bufShip->lives != 0) {
    if (bufEnemy->posX <= bufShip->posX + 16 && bufShip->posX <= (bufEnemy->posX + bufEnemy->space)) {
      if (bufEnemy->posY <= bufShip->posY + 8 && bufShip->posY <= (bufEnemy->posY + bufEnemy->space)) {
        FillRect(296, 8, 15, 50, 0x0000);
        FillRect(296, 182, 15, 50, 0x0000);
        if (bufShip->lives != 0) {
          bufShip->lives--;
          deathSound();
        }

        bufEnemy->state = DEAD;
        FillRect(bufEnemy->posY, bufEnemy->posX, bufEnemy->space, bufEnemy->space, 0x0000);

        for (int k = 0; k < ship1.lives; k++) {
          LCD_Bitmap(296, 8 + 17 * k, 16, 16, ship1Sprite);
        }
        if (gameType != 1) {
          for (int k = 0; k < ship2.lives; k++) {
            LCD_Bitmap(296, 216 - 17 * k, 16, 16, ship2Sprite);
          }
        }
      }
    }
  }
}
//Funcion para verificar todas las colisiones entre las balas y los enemigos
void checkAllBullets(struct enemy * bufEnemy) {
  checkBulletCollision(&bullet1, bufEnemy);
  checkBulletCollision(&bullet2, bufEnemy);
  checkBulletCollision(&bullet3, bufEnemy);
  if (gameType != 1) {
    checkBulletCollision(&bullet4, bufEnemy);
    checkBulletCollision(&bullet5, bufEnemy);
    checkBulletCollision(&bullet6, bufEnemy);
  }
}

//Funcion para enemigos en horizontal
void scrollingEnemy(struct enemy * bufStruct, int inf, int sup, unsigned char sprite[], unsigned char frames) {
  checkDirection(bufStruct->posX, &bufStruct->direction, inf, sup - bufStruct->space);

  if (bufStruct->direction == 1) {    //Dependiendo de la direccion incrementamos o decrementamos posicion
    bufStruct->posX = bufStruct->posX + bufStruct->speed;
  } else {
    bufStruct->posX = bufStruct->posX - bufStruct->speed;
  }

  for (int i = 0; i < bufStruct->speed; i++) {  //Se imprimen mas lineas dependiendo de la velocidad para eliminar el rastro
    H_line(bufStruct->posY, bufStruct->posX - 1 - i, bufStruct->space, 0x0000);
    H_line(bufStruct->posY, bufStruct->posX + bufStruct->space + i, bufStruct->space , 0x0000);
  }

  LCD_Sprite(bufStruct->posY, bufStruct->posX, bufStruct->space, bufStruct->space, sprite, frames, (bufStruct->posX / 8) % frames, 0, 0); //Imprime al enemigo en pantalla
}

//Funcion para enemigos en vertical
void flyingEnemy(struct enemy * bufStruct, int inf, int sup, unsigned char sprite[], unsigned char frames) {
  checkDirection(bufStruct->posY, &bufStruct->direction, inf, sup - bufStruct->space);
  int temp;
  if (bufStruct->direction == 1) {
    bufStruct->posY = bufStruct->posY + bufStruct->speed;
    temp = 0;
  } else {
    bufStruct->posY = bufStruct->posY - bufStruct->speed;
    temp = 1;
  }

  for (int i = 0; i < bufStruct->speed; i++) {
    V_line(bufStruct->posY - 1 - i, bufStruct->posX, bufStruct->space, 0x0000);
    V_line(bufStruct->posY + bufStruct->space + i, bufStruct->posX, bufStruct->space , 0x0000);
  }

  LCD_Sprite(bufStruct->posY, bufStruct->posX, bufStruct->space, bufStruct->space, sprite, frames, (bufStruct->posY / 8) % frames, temp, 0);
}

//Funcion para mostrar balas en pantalla
void bulletControl(struct bullet * bufBullet, unsigned char offset, unsigned char bulletType) {
  if (bufBullet->state == SHOT) { //Si la bala ha sido disparada
    bufBullet->posY += bufBullet->speed;
    if (bulletType == 1) {  //Balas Nave 1
      LCD_Bitmap(bufBullet->posY, bufBullet->posX, 7, 3, bulletSprite2);
    } else {                //Balas Nave 2
      LCD_Bitmap(bufBullet->posY, bufBullet->posX, 7, 3, bulletSprite);
    }
    if (bufBullet->posY >= 250) { //Reset de las balas cuando llegan al limite
      bufBullet->state = NOT_SHOT;
      FillRect(bufBullet->posY - 2, bufBullet->posX, 7, 3, 0x0000);
      bufBullet->posY = 25 + offset;

    }
  }

  for (int i = 1; i <= bufBullet->speed; i++) { //Se imprimen mas lineas dependiendo de la velocidad para eliminar el rastro
    V_line(bufBullet->posY - i + 3, bufBullet->posX, 3, 0x0000);
  }
}

//Funcion para imprimir estrellas :D
void star(int number) {
  for (int j = 0; j < number; j++) {
    int starX = random(300);          //Posicion X aleatoria
    int starY = random(240);          //Posicion Y aleatoria
    int starSize = random(1, 4);      //Tamaño aleatorio
    int starColor = random(0xFFFF);   //Color aleatorio
    FillRect(starX, starY, starSize, starSize + 1, starColor);  //Se imprime un cuadrado con estos parametros en pantalla
  }
}

//Melodia
void melody() {
  if (songCounter > sizeof(melodia) / sizeof(melodia[0])) songCounter = 0; //Maximo contador con tamaño melodia
  if (musicType == 0) {   //Modo melodia
    noteCounter++;
    if (noteCounter < corchea * 90) {  //Incrementar contador hasta que alcance el valor deseado en tiempo
      tone(pinBuzzer, melodia[songCounter]);  //Nota musica para el buzzer
    } else {
      songCounter++;    //Contador que sustituya al for para la melodia
      noteCounter = 0;  //Reiniciar contador
      musicType = 1;    //Cambiar de modo melodia a modo silencio
    }
  } else {                //Modo silencio
    pauseCounter++;
    if (pauseCounter < pausa * 90) {   //Incrementar contador hasta que alcance el valor deseado en tiempo
      noTone(pinBuzzer);                      //SILENCIO
    } else {
      pauseCounter = 0;
      musicType = 0;    //Cambiar de modo melodia a modo melodia
    }
  }
}

//Sonido de disparo
void bulletSound() {
  for (int i = 2500; i >= 250; i -= 250) {
    tone(pinBuzzer, i, 20);  // Tocar una nota con una frecuencia descendente y corta duración
    delay(8);  // Pequeña pausa entre las notas
    noTone(pinBuzzer);  // Detener el sonido
  }
}

//Sonido de muerte
void deathSound() {
  for (int i = 100; i <= 500; i += 50) {
    tone(pinBuzzer, i, 100);
    delay(50);  // Pausa breve entre las notas
  }
  // Sonido de retumbe
  tone(pinBuzzer, 200, 500);
  noTone(pinBuzzer);  // Detener el sonido
}

//Sonido de victoria
void winSound() {
  for (int i = 0; i < sizeof(melodiaGanador) / sizeof(melodiaGanador[0]); i++) {
    tone(pinBuzzer, melodiaGanador[i]);
    delay(corchea);
    noTone(pinBuzzer);
    // Agrega una pausa
    delay(pausa);
  }
  noTone(pinBuzzer);  // Detener el sonido
}

//Reset balas
void resetAllBullets() {
  bullet1.posY = 25;
  bullet1.speed = bulletSpeed;
  bullet1.state = NOT_SHOT;

  bullet2.posY = 25;
  bullet2.speed = bulletSpeed;
  bullet2.state = NOT_SHOT;

  bullet3.posY = 25;
  bullet3.speed = bulletSpeed;
  bullet3.state = NOT_SHOT;

  bullet4.posY = 45;
  bullet4.speed = bulletSpeed;
  bullet4.state = NOT_SHOT;

  bullet5.posY = 45;
  bullet5.speed = bulletSpeed;
  bullet5.state = NOT_SHOT;

  bullet6.posY = 45;
  bullet6.speed = bulletSpeed;
  bullet6.state = NOT_SHOT;

}

//Reset enemigos
void resetEnemies() {
  randomSeed(analogRead(18));
  bat1.posX = 0;
  bat1.posY = random(100, 230);
  bat1.direction = 1;
  bat1.space = 16;
  bat1.speed = 1 + enemySpeedPlus;
  bat1.state = ALIVE;

  fly1.posX = random(227);
  fly1.posY = 200;
  fly1.direction = 1;
  fly1.space = 13;
  fly1.speed = random(1, 3) + enemySpeedPlus;
  fly1.state = ALIVE;

  bat2.posX = 16;
  bat2.posY = random(100, 230);
  bat2.direction = 1;
  bat2.space = 16;
  bat2.speed = 1 + enemySpeedPlus;
  bat2.state = ALIVE;

  bat3.posX = 32;
  bat3.posY = random(100, 230);
  bat3.direction = 1;
  bat3.space = 16;
  bat3.speed = 1 + enemySpeedPlus;
  bat3.state = ALIVE;

  fly2.posX = random(227);
  fly2.posY = 200;
  fly2.direction = 1;
  fly2.space = 13;
  fly2.speed = random(1, 3) + enemySpeedPlus;
  fly2.state = ALIVE;

  rock2.posX = random(230);
  rock2.posY = random(100, 200);
  rock2.direction = 1;
  rock2.space = 10;
  rock2.speed = random(1, 2) + enemySpeedPlus;
  rock2.state = ALIVE;

  shrimp2.posX = random(50, 150);
  shrimp2.posY = random(60, 180);
  shrimp2.direction = 1;
  shrimp2.space = 13;
  shrimp2.speed = 1 + enemySpeedPlus;
  shrimp2.state = ALIVE;

  shrimp1.posX = random(50, 150);
  shrimp1.posY = random(60, 180);
  shrimp1.direction = 1;
  shrimp1.space = 13;
  shrimp1.speed = 1 + enemySpeedPlus;
  shrimp1.state = ALIVE;

  shrimp3.posX = random(50, 150);
  shrimp3.posY = random(60, 180);
  shrimp3.direction = 1;
  shrimp3.space = 13;
  shrimp3.speed = 1 + enemySpeedPlus;
  shrimp3.state = ALIVE;

  fly3.posX = random(227);
  fly3.posY = 200;
  fly3.direction = 1;
  fly3.space = 13;
  fly3.speed = random(1, 3) + enemySpeedPlus;
  fly3.state = ALIVE;

  rock1.posX = random(230);
  rock1.posY = random(100, 200);
  rock1.direction = 1;
  rock1.space = 10;
  rock1.speed = random(1, 2) + enemySpeedPlus;
  rock1.state = ALIVE;
}

//Secuencias de enemigos
void sequence1() {
  if (bat1.state == ALIVE) {
    scrollingEnemy(&bat1, 0, 208, purpleBat, 5);
    checkAllBullets(&bat1);
    //checkBulletCollision(&bullet1, &bat1);
  }

  if (bat2.state == ALIVE) {
    scrollingEnemy(&bat2, 16, 224, purpleBat, 5);
    checkAllBullets(&bat2);
    //checkBulletCollision(&bullet1, &bat2);
  }
  if (bat3.state == ALIVE) {
    scrollingEnemy(&bat3, 32, 240, purpleBat, 5);
    checkAllBullets(&bat3);
    //checkBulletCollision(&bullet1, &bat3);

  }
  if (fly1.state == ALIVE) {
    flyingEnemy(&fly1, 0, 250, greenFly, 2);
    checkAllBullets(&fly1);
    //checkBulletCollision(&bullet1, &fly1);
    checkShipCollision(&ship1, &fly1);
    if (gameType != 1) checkShipCollision(&ship2, &fly1);
  }
  if (fly2.state == ALIVE && fly1.state == DEAD) {
    flyingEnemy(&fly2, 0, 250, greenFly, 2);
    checkAllBullets(&fly2);
    //checkBulletCollision(&bullet1, &fly2);
    checkShipCollision(&ship1, &fly2);
    if (gameType != 1) checkShipCollision(&ship2, &fly2);
  }

  if (fly3.state == ALIVE && fly1.state == DEAD) {
    flyingEnemy(&fly3, 0, 250, greenFly, 2);
    checkAllBullets(&fly3);
    //checkBulletCollision(&bullet1, &fly3);
    checkShipCollision(&ship1, &fly3);
    if (gameType != 1) checkShipCollision(&ship2, &fly3);
  }

  if (shrimp1.state == ALIVE && bat1.state == DEAD) {
    scrollingEnemy(&shrimp1, 20, 194, blueShrimp, 4);
    checkAllBullets(&shrimp1);
    //checkBulletCollision(&bullet1, &shrimp1);
  }

  if (shrimp2.state == ALIVE && bat2.state == DEAD) {
    scrollingEnemy(&shrimp2, 20, 207, blueShrimp, 4);
    checkAllBullets(&shrimp2);
    //checkBulletCollision(&bullet1, &shrimp2);
  }

  if (shrimp3.state == ALIVE && bat3.state == DEAD) {
    scrollingEnemy(&shrimp3, 20, 220, blueShrimp, 4);
    checkAllBullets(&shrimp3);
    //checkBulletCollision(&bullet1, &shrimp3);
  }

  if (shrimp1.state == DEAD && shrimp2.state == DEAD && shrimp3.state == DEAD && fly2.state == DEAD && fly3.state == DEAD) {
    sequenceCase = random(3);
    resetEnemies();
    resetAllBullets();

  }

}

void sequence2() {
  if (rock1.state == ALIVE) {
    flyingEnemy(&rock1, 0, 250, rock, 4);
    checkShipCollision(&ship1, &rock1);
    if (gameType != 1) checkShipCollision(&ship2, &rock1);
  }

  if (rock2.state == ALIVE) {
    flyingEnemy(&rock2, 0, 250, rock, 4);
    checkShipCollision(&ship1, &rock2);
    if (gameType != 1) checkShipCollision(&ship2, &rock2);
  }

  if (fly1.state == ALIVE) {
    flyingEnemy(&fly1, 0, 250, greenFly, 2);
    checkAllBullets(&fly1);
    //checkBulletCollision(&bullet1, &fly1);
    checkShipCollision(&ship1, &fly1);
    if (gameType != 1) checkShipCollision(&ship2, &fly1);
  }
  if (fly2.state == ALIVE) {
    flyingEnemy(&fly2, 0, 250, greenFly, 2);
    checkAllBullets(&fly2);
    //checkBulletCollision(&bullet1, &fly2);
    checkShipCollision(&ship1, &fly2);
    if (gameType != 1) checkShipCollision(&ship2, &fly2);
  }

  if (fly3.state == ALIVE) {
    flyingEnemy(&fly3, 0, 250, greenFly, 2);
    checkAllBullets(&fly3);
    //checkBulletCollision(&bullet1, &fly3);
    checkShipCollision(&ship1, &fly3);
    if (gameType != 1) checkShipCollision(&ship2, &fly3);
  }

  if (fly1.state == DEAD && fly2.state == DEAD && fly3.state == DEAD) {

    FillRect(rock1.posY, rock1.posX, rock1.space, rock1.space, 0x0000);
    FillRect(rock2.posY, rock2.posX, rock2.space, rock2.space, 0x0000);
    sequenceCase = random(3);
    resetEnemies();
    resetAllBullets();

  }


}

void sequence3() {
  if (rock1.state == ALIVE) {
    flyingEnemy(&rock1, 0, 250, rock, 4);
    checkShipCollision(&ship1, &rock1);
    if (gameType != 1) checkShipCollision(&ship2, &rock1);
  }

  if (rock2.state == ALIVE) {
    flyingEnemy(&rock2, 0, 250, rock, 4);
    checkShipCollision(&ship1, &rock2);
    if (gameType != 1) checkShipCollision(&ship2, &rock2);
  }

  if (shrimp1.state == ALIVE) {
    scrollingEnemy(&shrimp1, 20, 194, blueShrimp, 4);
    checkAllBullets(&shrimp1);
    //checkBulletCollision(&bullet1, &shrimp1);
  }

  if (shrimp2.state == ALIVE) {
    scrollingEnemy(&shrimp2, 20, 207, blueShrimp, 4);
    checkAllBullets(&shrimp2);
    //checkBulletCollision(&bullet1, &shrimp2);
  }

  if (shrimp3.state == ALIVE) {
    scrollingEnemy(&shrimp3, 20, 220, blueShrimp, 4);
    checkAllBullets(&shrimp3);
    //checkBulletCollision(&bullet1, &shrimp3);
  }

  if (shrimp1.state == DEAD && shrimp2.state == DEAD && shrimp3.state == DEAD) {
    FillRect(rock1.posY, rock1.posX, rock1.space, rock1.space, 0x0000);
    FillRect(rock2.posY, rock2.posX, rock2.space, rock2.space, 0x0000);
    sequenceCase = random(3);
    resetEnemies();
    resetAllBullets();
  }

}

void sequence4() {
  if (bat1.state == ALIVE) {
    scrollingEnemy(&bat1, 0, 208, purpleBat, 5);
    checkAllBullets(&bat1);
    //checkBulletCollision(&bullet1, &bat1);
  }

  if (bat2.state == ALIVE) {
    scrollingEnemy(&bat2, 16, 224, purpleBat, 5);
    checkAllBullets(&bat2);
    //checkBulletCollision(&bullet1, &bat2);
  }

  if (shrimp1.state == ALIVE) {
    scrollingEnemy(&shrimp1, 20, 194, blueShrimp, 4);
    checkAllBullets(&shrimp1);
    //checkBulletCollision(&bullet1, &shrimp1);
  }

  if (shrimp2.state == ALIVE) {
    scrollingEnemy(&shrimp2, 20, 207, blueShrimp, 4);
    checkAllBullets(&shrimp2);
    //checkBulletCollision(&bullet1, &shrimp2);
  }

  if (bat1.state == DEAD && bat2.state == DEAD && shrimp1.state == DEAD && shrimp2.state == DEAD) {
    sequenceCase = random(3);
    resetEnemies();
    resetAllBullets();
  }
}

void BitmapFromSD(unsigned int x, unsigned int y, unsigned int width, unsigned int height, char fileName[]) {

  int temp;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 = x + width;
  y2 = y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int i, j;

  myFile = SD.open(fileName);
  if (myFile) {
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {

        while (temp != 120 && myFile.available()) {
          temp = myFile.read();
        }

        temp = myFile.read();
        input[0] = temp;
        temp = myFile.read();
        input[1] = temp;
        pixel1 = StrToHex(input);
        Serial.print(input);
        Serial.print(" - ");
        if (pixel1 == 255)Serial.println(255);
        if (pixel1 == 0)Serial.println(0);

        while (temp != 120 && myFile.available()) {
          temp = myFile.read();
        }

        temp = myFile.read();
        input[0] = temp;
        temp = myFile.read();
        input[1] = temp;
        pixel2 = StrToHex(input);
        Serial.print(input);
        Serial.print(" - ");
        if (pixel2 == 255)Serial.println(255);
        if (pixel2 == 0)Serial.println(0);
        LCD_DATA(255);
        LCD_DATA(255);
      }
    }

  }
  Serial.println("closed");
  myFile.close();
  // close the file:


  digitalWrite(LCD_CS, HIGH);
}

uint8_t StrToHex(char str[])
{
  return (uint8_t) strtol(str, 0, 16);
}





//***************************************************************************************************************************************
// FUNCIONES PARA LA LCD
//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER)
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40 | 0x80 | 0x20 | 0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
  //  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_DC, LOW);
  SPI.transfer(cmd);
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_DC, HIGH);
  SPI.transfer(data);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);
  LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
    }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8);
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y + h, w, c);
  V_line(x  , y  , h, c);
  V_line(x + w, y  , h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y + i, w, c);
  }
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 = x + width;
  y2 = y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k + 1]);
      //LCD_DATA(bitmap[k]);
      k = k + 2;
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);

  unsigned int x2, y2;
  x2 =   x + width;
  y2 =    y + height;
  SetWindows(x, y, x2 - 1, y2 - 1);
  int k = 0;
  int ancho = ((width * columns));
  if (flip) {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width - 1 - offset) * 2;
      k = k + width * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k - 2;
      }
    }
  } else {
    for (int j = 0; j < height; j++) {
      k = (j * (ancho) + index * width + 1 + offset) * 2;
      for (int i = 0; i < width; i++) {
        LCD_DATA(bitmap[k]);
        LCD_DATA(bitmap[k + 1]);
        k = k + 2;
      }
    }
  }
  digitalWrite(LCD_CS, HIGH);
}
