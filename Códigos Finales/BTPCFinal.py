import serial

try:
    BT1 = serial.Serial('COM13', 115200)
    print('BT Jugador1 Conectado')
except:
    print('BT Error con Jugador1')

try:
    BT2 = serial.Serial('COM14', 115200)
    print('BT Jugador2 Conectado')
except:
    print('BT Error con Jugador2')

try:
    PS2 = serial.Serial('COM8', 115200)
    print('TivaC Conectada')
except:
    print('Error con TivaC')

while True:
    # Leer datos de cualquier dispositivo BT Serial
    data = BT1.read()  # Lee datos de BT1
    if data:
        PS2.write(data)  # Reenvía los datos a través de COM8
        # Leer y mostrar los mensajes recibidos por COM8
        message = PS2.readline().decode('utf-8')  # Assuming messages are UTF-8 encoded
        if message:
            print('Mensaje recibido por COM8:', message)

    data2 = BT2.read()  # Lee datos de BT2
    if data2:
        PS2.write(data2)  # Reenvía los datos a través de COM8 a la Tiva-C
        # Leer y mostrar los mensajes recibidos por COM8
        message = PS2.readline().decode('utf-8')  # Assuming messages are UTF-8 encoded
        if message:
            print('Mensaje recibido por la Tiva:', message)