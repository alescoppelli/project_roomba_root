from pyb        import I2C
from lsm303dlhc import lsm303dlhc
from utime      import sleep_ms,sleep
from time       import ticks_ms

if __name__ == "__main__":

	i2c = I2C(1,I2C.MASTER)
	a   = lsm303dlhc(i2c)
	a.setup()
	
	sleep(8)
      	print("Start...")
	  	

	for i in range(5,10):
		start = ticks_ms()     
		a.guess_dt(i*1000)
		stop = ticks_ms()
		print(stop-start)

#start = ticks_ms()
#a.guess_dt(1000)
#stop = ticks_ms()
#print(stop-start)



#
#	while(True):
#		print(a.accel())
#		#sleep_ms(100)
#
#
