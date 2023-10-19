//int buttonPin = PUSH1;  // Pin del botón
int pinBuzzer = PC_4;

// Notas requeridas
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
int pausa = negra/5; // Duración de una pausa en milisegundos

int melodia[] = {Sol3, Do, Re, Fa, Mi, Do, Re, La, 
  Sol, Do, Re, Fa, Mi, Do, Sol, Si, 
  Do5, Si_b, La_b, Sol, Fa, Mi_b, Re, Si3_b,
  Si_b, Do5, Si, Sol, La, Fa, Re, Sol, Mi, Re};

int melodiaGanador[] = {
  Sol, Sol, Sol, Mi, Sol, La, Fa,
  Mi, Fa, Do, La, Si, La,
  Sol, Mi5, La5, Sol5, Fa5, Mi5,
  Do5, Do5, Do5, Si, Re5, Do5
};


//QUITAR DESPUES XD 
int disparo = 0;
int Explosion = 0;
int Explosion2 = 0;
int Muerte = 0;
int Ganar = 1;
int menu=0; 
void setup() {
  
  //pinMode(buttonPin, INPUT_PULLUP);  // Configurar el pin del botón como entrada con resistencia pull-up
  pinMode(pinBuzzer, OUTPUT);  // Configurar el pin del buzzer como salida
}

void loop() {
  if (menu==1){
    menu=0;
    for (int i = 0; i < sizeof(melodia) / sizeof(melodia[0]); i++) {
      // Toca las notas en ambos buzzers al mismo tiempo
      tone(pinBuzzer, melodia[i]);
      delay(corchea);
      noTone(pinBuzzer);
      // Agrega una pausa
      delay(pausa);
    }
  }
  if (disparo==1){
    disparo=0;
    //Disparo
    for (int i = 2500; i >= 100; i -= 100) {
      tone(pinBuzzer, i, 20);  // Tocar una nota con una frecuencia descendente y corta duración
      delay(10);  // Pequeña pausa entre las notas
      noTone(pinBuzzer);  // Detener el sonido
    }
  }
  if (Explosion==1){
    Explosion=0;
    //Explosión 1
    for (int i = 200; i <= 800; i += 200) {
      tone(pinBuzzer, i, 100);  // Tocar una nota con una frecuencia descendente y corta duración
      delay(10);  // Pequeña pausa entre las notas
      noTone(pinBuzzer);  // Detener el sonido
    }
  }

  if (Explosion2==1){
    Explosion2=0;
    //Explosión 2
    for (int i = 300; i <= 700; i += 100) {
      tone(pinBuzzer, i, 100);  // Tocar una nota con una frecuencia descendente y corta duración
      delay(10);  // Pequeña pausa entre las notas
      noTone(pinBuzzer);  // Detener el sonido
    }
  }

  if (Muerte==1){
    Muerte=0;
    //Muerte Explosión
    for (int i = 100; i <= 500; i += 50) {
      tone(pinBuzzer, i, 100);
      delay(50);  // Pausa breve entre las notas
    }
    // Sonido de retumbe
    tone(pinBuzzer, 200, 500);
  }

  if (Ganar==1){
    Ganar=0;
    // Sonido de ganador
    for (int i = 0; i < sizeof(melodiaGanador) / sizeof(melodiaGanador[0]); i++) {
      tone(pinBuzzer, melodiaGanador[i]);
      delay(corchea);
      noTone(pinBuzzer);
      // Agrega una pausa
      delay(pausa);
    }
  }
  
  
}
