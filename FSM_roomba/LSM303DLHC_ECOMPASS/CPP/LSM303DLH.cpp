/** LSM303DLH Interface Library
 *
 * Michael Shimniok http://bot-thoughts.com
 *
 * Based on test program by @tosihisa and 
 *
 * Pololu sample library for LSM303DLH breakout by ryantm:
 *
 * Copyright (c) 2011 Pololu Corporation. For more information, see
 *
 * http://www.pololu.com/
 * http://forum.pololu.com/
 *
 */
#include "mbed.h"
#include "LSM303DLH.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FILTER_SHIFT 6      // used in filtering acceleromter readings

const int addr_acc = 0x30;
const int addr_mag = 0x3c;

enum REG_ADDRS {
    /* --- Mag --- */
    CRA_REG_M   = 0x00,
    CRB_REG_M   = 0x01,
    MR_REG_M    = 0x02,
    OUT_X_M     = 0x03,
    OUT_Y_M     = 0x05,
    OUT_Z_M     = 0x07,
    /* --- Acc --- */
    CTRL_REG1_A = 0x20,
    CTRL_REG4_A = 0x23,
    OUT_X_A     = 0x28,
    OUT_Y_A     = 0x2A,
    OUT_Z_A     = 0x2C,
};

bool LSM303DLH::write_reg(int addr_i2c,int addr_reg, char v)
{
    char data[2] = {addr_reg, v}; 
    return LSM303DLH::_compass.write(addr_i2c, data, 2) == 0;
}

bool LSM303DLH::read_reg(int addr_i2c,int addr_reg, char *v)
{
    char data = addr_reg; 
    bool result = false;
    
    __disable_irq();
    if ((_compass.write(addr_i2c, &data, 1) == 0) && (_compass.read(addr_i2c, &data, 1) == 0)){
        *v = data;
        result = true;
    }
    __enable_irq();
    return result;
}

bool LSM303DLH::read_reg_short(int addr_i2c,int addr_reg, short *v)
{
    char *pv = (char *)v;
    bool result;
    
    result =  read_reg(addr_i2c,addr_reg+0,pv+1);
    result &= read_reg(addr_i2c,addr_reg+1,pv+0);
  
    return result;
}

LSM303DLH::LSM303DLH(PinName sda, PinName scl):
    _compass(sda, scl), _offset_x(0), _offset_y(0), _offset_z(0), _scale_x(0), _scale_y(0), _scale_z(0), _filt_ax(0), _filt_ay(0), _filt_az(6000)
{
    char reg_v;
    _compass.frequency(100000);
        
    reg_v = 0;
    reg_v |= 0x01 << 5;     /* Normal mode  */
    reg_v |= 0x07;          /* X/Y/Z axis enable. */
    write_reg(addr_acc,CTRL_REG1_A,reg_v);
    reg_v = 0;
    read_reg(addr_acc,CTRL_REG1_A,&reg_v);

    reg_v = 0;
    reg_v |= 0x01 << 6;     /* 1: data MSB @ lower address */
    reg_v |= 0x01 << 4;     /* +/- 4g */
    write_reg(addr_acc,CTRL_REG4_A,reg_v);

    /* -- mag --- */
    reg_v = 0;
    reg_v |= 0x04 << 2;     /* Minimum data output rate = 15Hz */
    write_reg(addr_mag,CRA_REG_M,reg_v);

    reg_v = 0;
    //reg_v |= 0x01 << 5;     /* +-1.3Gauss */
    reg_v |= 0x07 << 5;     /* +-8.1Gauss */
    write_reg(addr_mag,CRB_REG_M,reg_v);

    reg_v = 0;              /* Continuous-conversion mode */
    write_reg(addr_mag,MR_REG_M,reg_v);
}


void LSM303DLH::setOffset(float x, float y, float z)
{
    _offset_x = x;
    _offset_y = y;
    _offset_z = z;   
}

void LSM303DLH::setScale(float x, float y, float z)
{
    _scale_x = x;
    _scale_y = y;
    _scale_z = z;
}

void LSM303DLH::read(vector &a, vector &m)
{
    short a_x, a_y, a_z;
    short m_x, m_y, m_z;
    //Timer t;
    //int usec1, usec2;
    
    //t.reset();
    //t.start();

    //usec1 = t.read_us();
    read_reg_short(addr_acc, OUT_X_A, &a_x);
    read_reg_short(addr_acc, OUT_Y_A, &a_y);
    read_reg_short(addr_acc, OUT_Z_A, &a_z);
    read_reg_short(addr_mag, OUT_X_M, &m_x);
    read_reg_short(addr_mag, OUT_Y_M, &m_y);
    read_reg_short(addr_mag, OUT_Z_M, &m_z);
    //usec2 = t.read_us();
    
    //if (debug) debug->printf("%d %d %d\n", usec1, usec2, usec2-usec1);

    // Perform simple lowpass filtering
    // Intended to stabilize heading despite
    // device vibration such as on a UGV
    _filt_ax += a_x - (_filt_ax >> FILTER_SHIFT);
    _filt_ay += a_y - (_filt_ay >> FILTER_SHIFT);
    _filt_az += a_z - (_filt_az >> FILTER_SHIFT);

    a.x = (float) (_filt_ax >> FILTER_SHIFT);
    a.y = (float) (_filt_ay >> FILTER_SHIFT);
    a.z = (float) (_filt_az >> FILTER_SHIFT);
    
    // offset and scale
    m.x = (m_x + _offset_x) * _scale_x;
    m.y = (m_y + _offset_y) * _scale_y;
    m.z = (m_z + _offset_z) * _scale_z;
}


// Returns the number of degrees from the -Y axis that it
// is pointing.
float LSM303DLH::heading()
{
    return heading((vector){0,-1,0});
}

float LSM303DLH::heading(vector from)
{
    vector a, m;

    this->read(a, m);
    
    ////////////////////////////////////////////////
    // compute heading       
    ////////////////////////////////////////////////

    vector temp_a = a;
    // normalize
    vector_normalize(&temp_a);
    //vector_normalize(&m);

    // compute E and N
    vector E;
    vector N;
    vector_cross(&m,&temp_a,&E);
    vector_normalize(&E);
    vector_cross(&temp_a,&E,&N);
    
    // compute heading
    float heading = atan2(vector_dot(&E,&from), vector_dot(&N,&from)) * 180/M_PI;
    if (heading < 0) heading += 360;
    
    return heading;
}

void LSM303DLH::frequency(int hz)
{
    _compass.frequency(hz);
}
