import serial

    

if __name__ == "__main__":

    ser = serial.Serial(port='/dev/ttyUSB0', baudrate=57600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)

    #ser.open()
    
    for i in range(25):
        xx = ser.read()
        x = int.from_bytes(xx, "little")
        if x == 128 :
            print("Recived: START")
        elif x == 130 :
            print("Recived: CONTROL")
        elif x == 131 :
            print("Recived: SAFE")
        elif x == 132 :
            print("Recived: FULL")
        elif x == 133 :
            print("Recived: POWER")
        else:
            print("Recived: ",  int.from_bytes(xx, "little") )

    ser.close()
    
            
