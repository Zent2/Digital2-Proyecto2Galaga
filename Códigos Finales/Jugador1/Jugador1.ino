#include "BluetoothSerial.h"

BluetoothSerial BT;
// Entradas análogicas
const int pinJ1X = 13;
const int pinJ1Y = 25;

// Valores entradas analógicas
int J1X = 0;
int J1Y = 0;
int J1Y_Value = 0;
int J1X_Value = 0;

// Valores entradas analógicas guardadas
int J1Y_ValueP = 0;
int J1X_ValueP = 0;

// Entrada Digital
const int pinButtonA = 34;
int bandera = 0; // Variable para gestionar el botón
const int ledPin = 2; // Pin para la LED interna
bool isConnected = false; // Variable para el estado de la conexión Bluetooth
void setup() {
  //Nombre ESP32 que controla el control del Jugador 1
  BT.begin("ESP32-J1-CC");
  pinMode(2, OUTPUT);

  // Configura los pines del potenciómetro como entrada
  pinMode(pinJ1X, INPUT);
  pinMode(pinJ1Y, INPUT);

  // Configura el pin del botón como entrada pullup
  pinMode(pinButtonA, INPUT_PULLUP);

  
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
  if (digitalRead(pinButtonA) == LOW && bandera == 0) { // Si se presiona el botón y antes no estaba presionado
    bandera = 1;
    //BT.println("Button Pressed");
    BT.write(0xA1); // Enviar código 0xA1 para el botón presionado
  } else if (digitalRead(pinButtonA) == HIGH && bandera == 1) { // Si se suelta el botón y antes estaba presionado
    bandera = 0; //No se manda nada solo se cambia el valor de la bandera
    //BT.println("Button Released");
    //BT.write(0xA1); // Enviar código 0xAx para el botón liberado
  }

  // Lee el valor del potenciómetro
  J1X = analogRead(pinJ1X);
  J1Y = analogRead(pinJ1Y);

  // Escala el valor del potenciómetro de 0-1023 a 0-15
  J1X_Value = map(J1X, 0, 1023, 0, 15);
  J1Y_Value = map(J1Y, 0, 1023, 0, 15);

   // Combina el código con el valor 
  byte J1X_CodedValue = (0x04 << 4) | J1X_Value; // 0x0x para J1X
  byte J1Y_CodedValue = (0x01 << 4) | J1Y_Value; // 0x1x para J1Y

  // Envía los valores codificados a través de Bluetooth si cambian
  if (J1X_ValueP != J1X_Value) {
    BT.write(J1X_CodedValue);
    J1X_ValueP = J1X_Value;
  }

  if (J1Y_ValueP != J1Y_Value) {
    BT.write(J1Y_CodedValue);
    J1Y_ValueP = J1Y_Value;
  }

  // Espera un tiempo antes de enviar el siguiente valor
  delay(100);
}
