#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "py/mphal.h"
#include <py/builtin.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/mperrno.h"
#include "ports/stm32/uart.h"
#include "extmod/machine_i2c.h"
#include "modmachine.h"
#include "i2c.h"
#include "pin.h"
#include "irq.h"
#include "bufhelper.h"
#include "dma.h"




#define LOW_FILTER_TAP_NUM 13
#define NUM_AXIS (3)
#define DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN                              ((uint8_t)0x77)
#define DATA_RATE_10_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN                               ((uint8_t)0x27)
#define DATA_RATE_100_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN                              ((uint8_t)0x57)
#define CONTINUOS_UPDATE_LSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_DISABLE_SPI_4_WIRE   ((uint8_t)0x00)
#define CONTINUOS_UPDATE_LSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_ENABLE_SPI_4_WIRE    ((uint8_t)0x08)
#define CONTINUOS_UPDATE_MSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_DISABLE_SPI_4_WIRE   ((uint8_t)0x40)
#define I2C_TIMEOUT_MS (50)
#define ACCEL_ADDR   ((uint8_t) 0x19)
#define CTRL_REG1_A  ((uint8_t)0x20)
#define CTRL_REG4_A  ((uint8_t)0x23)
#define OUT_X_L_A    ((uint8_t)0x28)
#define OUT_X_H_A    ((uint8_t)0x29)
#define OUT_Y_L_A    ((uint8_t)0x2A)
#define OUT_Y_H_A    ((uint8_t)0x2B)
#define OUT_Z_L_A    ((uint8_t)0x2C)
#define OUT_Z_H_A    ((uint8_t)0x2D)
#define OUT_X_A      ((uint8_t)0x28)
#define OUT_Y_A      ((uint8_t)0x2A)
#define OUT_Z_A      (uint8_t)(0x2C)
#define COEF_CONVER_X  ((double)-0.00056)
#define COEF_CONVER_Y  ((double)-0.00056) 
#define COEF_CONVER_Z  ((double)-0.00056)
#define ZERO_BAND_LOW  ((int16_t) -350 )
#define ZERO_BAND_HIGH ((int16_t) 350 )


static double filter_taps[LOW_FILTER_TAP_NUM] = {
  0.020132210722515607,
  0.014337588088026872,
  -0.06042518986827016,
  -0.11688176581198412,
  0.015390548525687591,
  0.30600043556088063,
  0.464289723357815,
  0.30600043556088063,
  0.015390548525687591,
  -0.11688176581198412,
  -0.06042518986827016,
  0.014337588088026872,
  0.020132210722515607
};

typedef struct {
  double history[LOW_FILTER_TAP_NUM];
  unsigned int last_index;
} low_filter;


void low_filter_init_helper(low_filter* f) {
  uint16_t  i;
  for(i = 0; i < LOW_FILTER_TAP_NUM; ++i){
    f->history[i] = 0;
  }
  f->last_index = 0;
}

void low_filter_put_helper(low_filter* f, double input) {
  f->history[f->last_index++] = input;
  if(f->last_index == LOW_FILTER_TAP_NUM)
    f->last_index = 0;
}

double low_filter_get_helper(low_filter* f) {
  double   acc = 0;
  uint16_t index = f->last_index, i;
  for(i = 0; i < LOW_FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : LOW_FILTER_TAP_NUM-1;
    acc += f->history[index] * filter_taps[i];
  };
  return acc;
}


bool write_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t data ){

    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;
    uint8_t buffer[2];
      
    // ORing 0x80 auto-increments the register for each read  
    //accel_i2c_slave_reg |= 0x80;       

    buffer[0] = (uint8_t) accel_i2c_slave_reg;
    buffer[1] = data;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buffer, 2, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "WRITE_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 

 return ret;
}


bool read_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, int16_t* data ){
    
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;
    uint8_t buf[2];
    union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;

    buf[0] = accel_i2c_slave_reg;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 

    status = HAL_I2C_Master_Receive(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
        mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Receive error\n");
        ret = false;
     }
     accel.low = buf[0];

    buf[0] = accel_i2c_slave_reg+1;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 
      
    status = HAL_I2C_Master_Receive(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
        mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Receive error\n");
        ret = false;
     }
     accel.high = buf[0];


     //Add a correction for little values
     if( (accel.value > ZERO_BAND_LOW)	 && (accel.value < ZERO_BAND_HIGH ) ){
       accel.value=0;
     }


     *data = accel.value;

 return ret;
}

bool convert_to_m_s_s_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, double* accel){
    bool ret = true;
    int16_t int_value;

    read_register_helper(hi2c, accel_i2c_slave_reg, &int_value );
    if ( accel_i2c_slave_reg ==  OUT_X_A ){
       *accel = (double) int_value *  COEF_CONVER_X; 
    }else if(accel_i2c_slave_reg ==  OUT_Y_A ){
        *accel = (double) int_value *  COEF_CONVER_Y;
    }else if(accel_i2c_slave_reg ==  OUT_Z_A ){
        *accel = (double) int_value *  COEF_CONVER_Z;
    }

 return ret;
}



//float freq = mp_obj_get_float_to_f(freq_in);
bool statistic_helper(I2C_HandleTypeDef* hi2c,double* mx,double* my,double* mz,double* vx,double* vy,double* vz){
   bool ret = true;
   uint16_t HOW_MANY_SAMPLES=3000;
   int16_t int_value; 

   double sum_x,sum_y,sum_z;
   double mean_x,mean_y,mean_z;
   double diff_x_mean_sq,diff_y_mean_sq,diff_z_mean_sq;
   double variance_x,variance_y,variance_z;
   double temp;
   
    sum_x = sum_y = sum_z = 0.0;
    diff_x_mean_sq = diff_y_mean_sq = diff_z_mean_sq = 0.0;

    for(uint16_t i=0; i<HOW_MANY_SAMPLES; i++){
        read_register_helper(hi2c, OUT_X_A, &int_value );
        sum_x += (double) int_value;
        //START DEBUG
        //mp_printf(MP_PYTHON_PRINTER, "Read value V=%d\n", accel.value);
       //END DEBUG

        read_register_helper(hi2c, OUT_Y_A, &int_value );
        sum_y += (double) int_value;
        //START DEBUG
        //mp_printf(MP_PYTHON_PRINTER, "Read value V=%d\n", accel.value);
       //END DEBUG


        read_register_helper(hi2c, OUT_Z_A, &int_value );
        sum_z += (double) int_value;   
        //START DEBUG
        //mp_printf(MP_PYTHON_PRINTER, "Read value V=%d\n", accel.value);
       //END DEBUG

    }	

    mean_x = sum_x/HOW_MANY_SAMPLES;
    mean_y = sum_y/HOW_MANY_SAMPLES;
    mean_z = sum_z/HOW_MANY_SAMPLES;  

    *mx = mean_x;
    *my = mean_y;
    *mz = mean_z;



    for(uint16_t i=0; i<HOW_MANY_SAMPLES; i++){

        read_register_helper(hi2c, OUT_X_A, &int_value );
        temp = (double) int_value - mean_x;
        temp *= temp;  
        diff_x_mean_sq += temp;


        read_register_helper(hi2c, OUT_Y_A, &int_value );
        temp = (double) int_value - mean_y;
        temp *= temp;  
        diff_y_mean_sq += temp;



        read_register_helper(hi2c, OUT_Z_A, &int_value );
        temp = (double) int_value - mean_z;
        temp *= temp;  
        diff_z_mean_sq += temp;

    }	

    //standard_deviation_x = sqrt(diff_x_mean_sq/HOW_MANY_SAMPLES);
    //standard_deviation_y = sqrt(diff_y_mean_sq/HOW_MANY_SAMPLES);
    //standard_deviation_z = sqrt(diff_z_mean_sq/HOW_MANY_SAMPLES);


    variance_x = (diff_x_mean_sq/HOW_MANY_SAMPLES);
    variance_y = (diff_y_mean_sq/HOW_MANY_SAMPLES);
    variance_z = (diff_z_mean_sq/HOW_MANY_SAMPLES);


    *vx = variance_x;
    *vy = variance_y;
    *vz = variance_z;

//START DEBUG
//mp_printf(MP_PYTHON_PRINTER, "X=%f, Y=%f, Z=%f  \n", mean_x,mean_y,mean_z);
//mp_printf(MP_PYTHON_PRINTER, "SDX=%f, SDY=%f, SDZ=%f  \n", standard_deviation_x,standard_deviation_y,standard_deviation_z);
//END DEBUG

return ret;
}


/******************************************************************************/
/* MicroPython bindings                                                      */

typedef struct _accelerometer_lsm303dlhc_obj_t {
    mp_obj_base_t base;
    pyb_i2c_obj_t* i2c;
    low_filter x_axes_low_pass;
    low_filter y_axes_low_pass;
    low_filter z_axes_low_pass;

} accelerometer_lsm303dlhc_obj_t;

const mp_obj_type_t accelerometer_lsm303dlhc_type;



STATIC void lsm303dlhc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    //accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
 
    mp_print_str(print, "Accelerometer lsm303dlhc ");
    ////mp_printf(print, " X= %d, Y= %d, Z=%d  ", self->accel_x.value,  self->accel_y.value, self->accel_z.value);

}


STATIC mp_obj_t lsm303dlhc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    accelerometer_lsm303dlhc_obj_t *self = m_new_obj(accelerometer_lsm303dlhc_obj_t);
    self->base.type = &accelerometer_lsm303dlhc_type;
    
    
    if (mp_obj_get_type(args[0]) == &pyb_i2c_type) {
         self->i2c=args[0];
    }else{
         mp_print_str(MP_PYTHON_PRINTER, "The argumet is not a I2C type.");
    }
 
    low_filter_init_helper(&self->x_axes_low_pass);
    low_filter_init_helper(&self->y_axes_low_pass);
    low_filter_init_helper(&self->z_axes_low_pass);
 

    return MP_OBJ_FROM_PTR(self);
}

//
//  ----------------Class methods ----------------
//


//Class method 'setup'
STATIC mp_obj_t lsm303dlhc_setup(mp_obj_t self_in ) {
    bool ret=false;
    uint8_t set_reg1 = DATA_RATE_100_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN;
    uint8_t set_reg4 = CONTINUOS_UPDATE_LSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_ENABLE_SPI_4_WIRE;
    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    
    ret=write_register_helper(self->i2c->i2c, CTRL_REG1_A, set_reg1 );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during CTRL_REG1_A  setting ");
      return mp_const_false;
    }
    ret=write_register_helper(self->i2c->i2c, CTRL_REG4_A, set_reg4 );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during CTRL_REG4_A  setting ");
      return mp_const_false;
    }


    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_setup_obj, lsm303dlhc_setup);
//mp_printf(MP_PYTHON_PRINTER, "Z_L=%d, Z_H=%d, Z=%d  ", self->accel_z.low,  self->accel_z.high, self->accel_z.value);


//  --------------------------------
//Class method 'read_axes'
STATIC mp_obj_t lsm303dlhc_read_axes(mp_obj_t self_in ) {
    int16_t x;
    int16_t y;
    int16_t z;
    bool ret = false;


    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);


    ret=read_register_helper(self->i2c->i2c, OUT_X_A, &x );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X reading \n");
      return mp_const_false;
    }

    ret=read_register_helper(self->i2c->i2c, OUT_Y_A, &y );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y reading \n");
      return mp_const_false;
    }

    ret=read_register_helper(self->i2c->i2c, OUT_Z_A, &z );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z reading \n");
      return mp_const_false;
    }


    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_int(x);
    tuple[1] = mp_obj_new_int(y);
    tuple[2] = mp_obj_new_int(z);
    
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_obj, lsm303dlhc_read_axes);


//  --------------------------------
//Class method '_accel'
STATIC mp_obj_t lsm303dlhc_read_accel_axes(mp_obj_t self_in ) {
    double ax;
    double ay;
    double az;
    bool ret = false;


    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    ret=convert_to_m_s_s_helper(self->i2c->i2c, OUT_X_A, &ax);
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X reading \n");
      return mp_const_false;
    }

    ret=convert_to_m_s_s_helper(self->i2c->i2c, OUT_Y_A, &ay);
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y reading \n");
      return mp_const_false;
    }

    ret=convert_to_m_s_s_helper(self->i2c->i2c, OUT_Z_A, &az);
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z reading \n");
      return mp_const_false;
    }

    //Add a low filter 
    low_filter_put_helper(&self->x_axes_low_pass, ax);
    low_filter_put_helper(&self->y_axes_low_pass, ay);
    low_filter_put_helper(&self->z_axes_low_pass, az);


    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_float((float)ax);
    tuple[1] = mp_obj_new_float((float)ay);
    tuple[2] = mp_obj_new_float((float)az);
    //tuple[0] = mp_obj_new_float(   (float) low_filter_get_helper(&self->x_axes_low_pass));
    //tuple[1] = mp_obj_new_float(   (float) low_filter_get_helper(&self->y_axes_low_pass));
    //tuple[2] = mp_obj_new_float(   (float) low_filter_get_helper(&self->z_axes_low_pass));



    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_accel_axes_obj, lsm303dlhc_read_accel_axes);





//  --------------------------------
//Class method 'statistic'
STATIC mp_obj_t lsm303dlhc_statistic(mp_obj_t self_in ) {
 accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
   double  mx,my,mz;
   double  sdx,sdy,sdz;
   uint16_t delay=4;


   mp_printf(MP_PYTHON_PRINTER, "Get your board and put them on a stable position\n");
   mp_hal_delay_ms(1000 * delay);

   mp_printf(MP_PYTHON_PRINTER, "Please don't move the board\n");
   mp_hal_delay_ms(1000 * delay);


   mp_printf(MP_PYTHON_PRINTER, "Starting the acquisition...");
   mp_hal_delay_ms(1000 * delay);

   statistic_helper(self->i2c->i2c,&mx,&my,&mz,&sdx,&sdy,&sdz);

   mp_printf(MP_PYTHON_PRINTER, "done\n");
   mp_hal_delay_ms(1000 * delay);

   mp_printf(MP_PYTHON_PRINTER, "MEAN AXES X = %f\n", mx);
   mp_printf(MP_PYTHON_PRINTER, "MEAN AXES Y = %f\n", my);
   mp_printf(MP_PYTHON_PRINTER, "MEAN AXES Z = %f\n", mz);


   mp_printf(MP_PYTHON_PRINTER, "STANDARD DEVIATION  AXES X = %f\n", sdx);
   mp_printf(MP_PYTHON_PRINTER, "STANDARD DEVIATION  AXES Y = %f\n", sdy);
   mp_printf(MP_PYTHON_PRINTER, "STANDARD DEVIATION  AXES Z = %f\n", sdz);



  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_statistic_obj, lsm303dlhc_statistic);



STATIC const mp_rom_map_elem_t lsm303dlhc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_axes), MP_ROM_PTR(&lsm303dlhc_read_axes_obj) },
    { MP_ROM_QSTR(MP_QSTR_accel),     MP_ROM_PTR(&lsm303dlhc_read_accel_axes_obj) },
    { MP_ROM_QSTR(MP_QSTR_setup),     MP_ROM_PTR(&lsm303dlhc_setup_obj) },
    { MP_ROM_QSTR(MP_QSTR_statistic), MP_ROM_PTR(&lsm303dlhc_statistic_obj) },
};


STATIC MP_DEFINE_CONST_DICT(lsm303dlhc_locals_dict, lsm303dlhc_locals_dict_table);

const mp_obj_type_t accelerometer_lsm303dlhc_type = {
    { &mp_type_type },
    .name = MP_QSTR_lsm303dlhc,
    .print = lsm303dlhc_print,
    .make_new = lsm303dlhc_make_new,
    .locals_dict = (mp_obj_dict_t*)&lsm303dlhc_locals_dict,
};


//
STATIC const mp_map_elem_t lsm303dlhc_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_lsm303dlhc) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_lsm303dlhc), (mp_obj_t)&accelerometer_lsm303dlhc_type },	
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_add), (mp_obj_t)&roomba_add_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_lsm303dlhc_globals,
    lsm303dlhc_globals_table
);

const mp_obj_module_t lsm303dlhc_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lsm303dlhc_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lsm303dlhc, lsm303dlhc_user_cmodule, MODULE_LSM303DLHC_ENABLED);


