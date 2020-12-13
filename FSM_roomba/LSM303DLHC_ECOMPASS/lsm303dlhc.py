from micropython import const
from pyb import Pin


LSM303DLHC_SLAVE_ADDRESS_READ  = const(0x33)
LSM303DLHC_SLAVE_ADDRESS_WRITE = const(0x32)


DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Y_DIS = const(0x73)
LSM303DLHC_CTRL_REG1_A = const(0x20)

CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE = const(0x08)
LSM303DLHC_CTRL_REG4_A = const(0x23)

LSM303DLHC_STATUS_REG_A = const(0x27)
LSM303DLHC_OUT_X_L_A = const(0x28)
LSM303DLHC_OUT_X_L_A = const(0x29)

LSM303DLHC_OUT_Y_L_A = const(0x2A)
LSM303DLHC_OUT_Y_L_A = const(0x2B)

LSM303DLHC_OUT_Z_L_A = const(0x2C)
LSM303DLHC_OUT_Z_L_A = const(0x2D)

// Shift values to create properly formed integer (low byte first)
  // KTOWN: 12-bit values are left-aligned, no shift needed
  // accelData.x = (xlo | (xhi << 8)) >> 4;
  // accelData.y = (ylo | (yhi << 8)) >> 4;
  // accelData.z = (zlo | (zhi << 8)) >> 4;
  accelData.x = (int16_t)((xhi << 8) | xlo);
  accelData.y = (int16_t)((yhi << 8) | ylo);
  accelData.z = (int16_t)((zhi << 8) | zlo);
  
  
