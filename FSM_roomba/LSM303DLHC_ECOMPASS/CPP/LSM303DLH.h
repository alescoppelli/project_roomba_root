#include "mbed.h"
#include "vector.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/** Tilt-compensated compass interface Library for the STMicro LSM303DLH 3-axis magnetometer, 3-axis acceleromter
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
 *
 * @code
 * #include "mbed.h"
 * #include "LSM303DLH.h"
 *
 * Serial debug(USBTX,USBRX);
 * LSM303DLH compass(p28, p27);
 *
 * int main() {
 *   float hdg;
 *   debug.format(8,Serial::None,1);
 *   debug.baud(115200);
 *   debug.printf("LSM303DLH Test\x0d\x0a");
 *   compass.setOffset(29.50, -0.50, 4.00); // example calibration
 *   compass.setScale(1.00, 1.03, 1.21);    // example calibration
 *   while(1) {
 *     hdg = compass.heading();
 *     debug.printf("Heading: %.2f\n", hdg);
 *     wait(0.1);
 *   }
 * }
 * @endcode
 */
class LSM303DLH {
    public:
        /** Create a new interface for an LSM303DLH
         *
         * @param sda is the pin for the I2C SDA line
         * @param scl is the pin for the I2C SCL line
         */
        LSM303DLH(PinName sda, PinName scl);

        /** sets the x, y, and z offset corrections for hard iron calibration
         * 
         * Calibration details here:
         *  http://mbed.org/users/shimniok/notebook/quick-and-dirty-3d-compass-calibration/
         *
         * If you gather raw magnetometer data and find, for example, x is offset
         * by hard iron by -20 then pass +20 to this member function to correct
         * for hard iron.
         *
         * @param x is the offset correction for the x axis
         * @param y is the offset correction for the y axis
         * @param z is the offset correction for the z axis
         */
        void setOffset(float x, float y, float z);
        
        /** sets the scale factor for the x, y, and z axes
         *
         * Calibratio details here:
         *  http://mbed.org/users/shimniok/notebook/quick-and-dirty-3d-compass-calibration/
         *
         * Sensitivity of the three axes is never perfectly identical and this
         * function can help to correct differences in sensitivity.  You're
         * supplying a multipler such that x, y and z will be normalized to the
         * same max/min values
         */
        void setScale(float x, float y, float z);

        /** read the raw accelerometer and compass values
         *
         * @param a is the accelerometer 3d vector, written by the function
         * @param m is the magnetometer 3d vector, written by the function
         */
        void read(vector &a, vector &m);
        
        /** returns the magnetic heading with respect to the y axis
         *
         */
        float heading(void);
        
        /** returns the heading with respect to the specified vector
         *
         */
        float heading(vector from);
    
        /** sets the I2C bus frequency
         *
         * @param frequency is the I2C bus/clock frequency, either standard (100000) or fast (400000)
         */
        void frequency(int hz);
    
    private:
        I2C _compass;
        float _offset_x;
        float _offset_y;
        float _offset_z;
        float _scale_x;
        float _scale_y;
        float _scale_z;
        long _filt_ax;
        long _filt_ay;
        long _filt_az;
       
        bool write_reg(int addr_i2c,int addr_reg, char v);
        bool read_reg(int addr_i2c,int addr_reg, char *v);
        bool read_reg_short(int addr_i2c,int addr_reg, short *v);
};
