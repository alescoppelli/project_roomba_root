from micropython import const
from machine import I2C


DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN  = const(0x77) 
CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE = const(0x08) 
I2C_TIMEOUT_MS (50)
ACCEL_ADDR = const(0x19)  
CTRL_REG1_A = const(0x20)  
CTRL_REG4_A = const(0x23)  
OUT_X_L_A = const(0x28)   
OUT_X_H_A = const(0x29)    
OUT_Y_L_A = const(0x2A)   
OUT_Y_H_A = const(0x2B)   
OUT_Z_L_A = const(0x2C)   
OUT_Z_H_A = const(0x2D)   



class STMAccel:

	def __init__(self):
		self.i2c = I2C(1)
		i2c.writeto_mem(ACCEL_ADDR,CTRL_REG1_A,DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN)
		i2c.writeto_mem(ACCEL_ADDR,CTRL_REG4_A,CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE )
		
		

	def read(self):
		xl=self.i2c.readfrom_mem(ACCEL_ADDR,OUT_X_L_A,1)
		xh=self.i2c.readfrom_mem(ACCEL_ADDR,OUT_X_H_A,1)	
			
		
