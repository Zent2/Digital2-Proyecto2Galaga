#include "BluetoothSerial.h"

BluetoothSerial BT;
// Entradas análogicas
const int pinJ2X = 13;
const int pinJ2Y = 25;

// Valores entradas analógicas
int J2X = 0;
int J2Y = 0;
int J2Y_Value = 0;
int J2X_Value = 0;

// Valores entradas analógicas guardadas
int J2Y_ValueP = 0;
int J2X_ValueP = 0;

// Entrada Digital
const int pinButtonB = 34;
int bandera = 0; // Variable para gestionar el botón
const int ledPin = 2; // Pin para la LED interna
bool isConnected = false; // Variable para el estado de la conexión Bluetooth
void setup() {
  //Nombre ESP32 que controla el control del Jugador 1
  BT.begin("ESP32-J2-CC");
  pinMode(2, OUTPUT);

  // Configura los pines del potenciómetro como entrada
  pinMode(pinJ2X, INPUT);
  pinMode(pinJ2Y, INPUT);

  // Configura el pin del botón como entrada pullup
  pinMode(pinButtonB, INPUT_PULLUP);

  
}

void loop() {
  if (BT.connected() && !isConnected) {
    isConnected = true;
    digitalWrite(ledPin, HIGH); // Enciende la LED interna cuando se conecta
  } else if (!BT.connected() && isConnected) {
    isConnected = false;
    digitalWrite(ledPin, LOW); // Apaga la LED interna cuando se desconecta
  }
  // Lee el estado del botón
  if (digitalRead(pinButtonB) == LOW && bandera == 0) { // Si se presiona el botón y antes no estaba presionado
    bandera = 1;
    //BT.println("Button Pressed");
    BT.write(0xB1); // Enviar código 0xB1 para el botón presionado
  } else if (digitalRead(pinButtonB) == HIGH && bandera == 1) { // Si se suelta el botón y antes estaba presionado
    bandera = 0; //No se manda nada solo se cambia el valor de la bandera
    //BT.println("Button Released");
    //BT.write(0xA1); // Enviar código 0xAx para el botón liberado
  }

  // Lee el valor del potenciómetro
  J2X = analogRead(pinJ2X);
  J2Y = analogRead(pinJ2Y);

  // Escala el valor del potenciómetro de 0-1023 a 0-15
  J2X_Value = map(J2X, 0, 1023, 0, 15);
  J2Y_Value = map(J2Y, 0, 1023, 0, 15);

   // Combina el código con el valor 
  byte J2X_CodedValue = (0x02 << 4) | J2X_Value; // 0x2x para J2X
  byte J2Y_CodedValue = (0x03 << 4) | J2Y_Value; // 0x3x para J2Y

  // Envía los valores codificados a través de Bluetooth si cambian
  if (J2X_ValueP != J2X_Value) {
    BT.write(J2X_CodedValue);
    J2X_ValueP = J2X_Value;
  }

  if (J2Y_ValueP != J2Y_Value) {
    BT.write(J2Y_CodedValue);
    J2Y_ValueP = J2Y_Value;
  }

  // Espera un tiempo antes de enviar el siguiente valor
  delay(100);
}
