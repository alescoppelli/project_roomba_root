# main.py -- put your code here!

import roomba
from machine import UART



if __name__ == "__main__":

	uart = UART(6)
	uart.init(baudrate=57600,bits=8,parity=None,stop=1)
	
	robot = roomba.roomba("name",uart)






