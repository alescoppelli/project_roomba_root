from machine import Pin, SoftI2C
from micropython import const
import pyb
#import uctypes
#import ustruct
#from struct import unpack


serial_clock=pyb.Pin.board.PB6
serial_data=pyb.Pin.board.PB9
i2c = SoftI2C(scl=serial_clock,sda=serial_data, freq=400000)

DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN = const(0x77)
CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE = const(0x08)

i2c.writeto_mem(25,32,b'\x77')
i2c.writeto_mem(25,35,b'\x08')

i2c.readfrom_mem(25,40,1)
i2c.readfrom_mem(25,41,1)

x = bytearray((i2c.readfrom_mem(25,41,1),i2c.readfrom_mem(25,40,1)))
(num,) = struct.unpack('>i', x)

 
