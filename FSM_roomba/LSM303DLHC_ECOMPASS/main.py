from pyb import I2C
from lsm303dlhc import lsm303dlhc
from utime import sleep_ms


if __name__ == "__main__":

	i2c = I2C(1,I2C.MASTER)
	a   = lsm303dlhc(i2c)
	a.setup()

	while(True):
		print(a.accel())
		#sleep_ms(100)


