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


bool read_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t* data ){
    
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;
    uint8_t buf[2];

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
     data[0] = buf[0];

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
     data[1] = buf[0];

 return ret;
}

bool convert_to_m_s_s(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, double* accel){
    bool ret = true;
    uint8_t buf[2];

    read_register_helper(hi2c, accel_i2c_slave_reg, buf );



 return ret;
}

bool read_x_axes_helper(I2C_HandleTypeDef* hi2c, int16_t* data ){
    bool ret = true;
    union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;

    ret=read_register_helper(hi2c, OUT_X_L_A , &accel.low );

    *data = accel.value;

 return ret;
}




bool read_register_and_convert_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, int16_t* data ){
    bool ret = true;
    union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;

    ret=read_register_helper(hi2c, accel_i2c_slave_reg, &accel.low );

    *data = accel.value;

 return ret;
}

bool read_axes_xyz(I2C_HandleTypeDef* hi2c,int16_t* x, int16_t* y, int16_t* z){
   bool ret = true;

   read_register_and_convert_helper(hi2c, OUT_X_A, x );
   read_register_and_convert_helper(hi2c, OUT_Y_A, y );
   read_register_and_convert_helper(hi2c, OUT_Z_A, z );

 return ret;
} 

bool read_axes_filtered_helper(I2C_HandleTypeDef* hi2c, int16_t* x, int16_t* y, int16_t* z ){

    uint8_t HOW_MANY = 7;
    int16_t X[HOW_MANY];
    int16_t Y[HOW_MANY];
    int16_t Z[HOW_MANY];
    int16_t tmp;

    bool ret = true;

    for(uint8_t i=0; i < HOW_MANY; i++ ){
           read_axes_xyz(hi2c,&X[i],&Y[i],&Z[i]); 
    }
    
    tmp =0;
    for(uint8_t i=0; i < HOW_MANY; i++ ){
           tmp += X[i];  
    }
    *x = tmp/HOW_MANY;

    tmp =0;
    for(uint8_t i=0; i < HOW_MANY; i++ ){
           tmp += Y[i];  
    }
    *y = tmp/HOW_MANY;

    tmp =0;
    for(uint8_t i=0; i < HOW_MANY; i++ ){
           tmp += Z[i];  
    }
    *z = tmp/HOW_MANY;



 return ret;
}

//float freq = mp_obj_get_float_to_f(freq_in);
bool statistic_helper(I2C_HandleTypeDef* hi2c,double* mx,double* my,double* mz,double* sdx,double* sdy,double* sdz){
   bool ret = true;
   uint16_t HOW_MANY_SAMPLES=530;
   union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;

    double sum_x,sum_y,sum_z;
    double mean_x,mean_y,mean_z;
    double diff_x_mean_sq,diff_y_mean_sq,diff_z_mean_sq;
    double standard_deviation_x,standard_deviation_y,standard_deviation_z;
    double temp;
    sum_x = sum_y = sum_z = 0.0;
    diff_x_mean_sq = diff_y_mean_sq = diff_z_mean_sq = 0.0;

    for(uint16_t i=0; i<HOW_MANY_SAMPLES; i++){
        read_register_helper(hi2c, OUT_X_A, &accel.low );
        sum_x += (double) accel.value;
        //START DEBUG
        //mp_printf(MP_PYTHON_PRINTER, "Read value V=%d\n", accel.value);
       //END DEBUG

        read_register_helper(hi2c, OUT_Y_A, &accel.low );
        sum_y += (double) accel.value;
        //START DEBUG
        //mp_printf(MP_PYTHON_PRINTER, "Read value V=%d\n", accel.value);
       //END DEBUG


        read_register_helper(hi2c, OUT_Z_A, &accel.low );
        sum_z += (double) accel.value;   
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

        read_register_helper(hi2c, OUT_X_A, &accel.low );
        temp = (double) accel.value - mean_x;
        temp *= temp;  
        diff_x_mean_sq += temp;


        read_register_helper(hi2c, OUT_Y_A, &accel.low );
        temp = (double) accel.value - mean_y;
        temp *= temp;  
        diff_y_mean_sq += temp;



        read_register_helper(hi2c, OUT_Z_A, &accel.low );
        temp = (double) accel.value - mean_z;
        temp *= temp;  
        diff_z_mean_sq += temp;

    }	

    //standard_deviation_x = sqrt(diff_x_mean_sq/HOW_MANY_SAMPLES);
    //standard_deviation_y = sqrt(diff_y_mean_sq/HOW_MANY_SAMPLES);
    //standard_deviation_z = sqrt(diff_z_mean_sq/HOW_MANY_SAMPLES);


    standard_deviation_x = (diff_x_mean_sq/HOW_MANY_SAMPLES);
    standard_deviation_y = (diff_y_mean_sq/HOW_MANY_SAMPLES);
    standard_deviation_z = (diff_z_mean_sq/HOW_MANY_SAMPLES);


    *sdx = standard_deviation_x;
    *sdy = standard_deviation_y;
    *sdz = standard_deviation_z;

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

    union _accel_x{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel_x;

    union _accel_y{
     struct{
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel_y;
    
    union _accel_z{
    struct{
       uint8_t low;
       uint8_t high;
    };   
     int16_t value;
    }accel_z;
         
} accelerometer_lsm303dlhc_obj_t;

const mp_obj_type_t accelerometer_lsm303dlhc_type;



STATIC void lsm303dlhc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
 
    mp_print_str(print, "Accelerometer lsm303dlhc ");
    mp_printf(print, " X= %d, Y= %d, Z=%d  ", self->accel_x.value,  self->accel_y.value, self->accel_z.value);

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
 

    self->accel_x.value=0;
    self->accel_y.value=0;
    self->accel_z.value=0;
   

    return MP_OBJ_FROM_PTR(self);
}


//  ----------------Class methods ----------------
//Class method 'write_register'
///STATIC mp_obj_t pyb_i2c_mem_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
///Original function in pyb_i2c.c
/// \method  write_register(self_in, data_t, data_len_t, i2c_addr_t, i2c_reg_t, timeout_t=5000, mem_size_t=8)
///
/// Write to the memory of an I2C device:
///
///   - `self_in`     object 
///   - `data_t`      can be an integer or a buffer to write from
//////  - `data_len_t`  data length
///   - `i2c_addr_t`  is the I2C device address
///   - `i2c_reg_t`   is the memory location within the I2C device
//////   - `timeout_t`   is the timeout in milliseconds to wait for the write (default 1000)
//////   - `mem_size_t`  selects width of memaddr: 8 or 16 bits (default 8 )
///
/// Returns 0.
/// This is only valid in master mode.
//STATIC mp_obj_t lsm303dlhc_write_register(mp_obj_t self_in, mp_obj_t data_t, mp_obj_t data_len_t, mp_obj_t i2c_addr_t, mp_obj_t i2c_reg_t, mp_obj_t timeout_t, mp_obj_t mem_size_t ) {
STATIC mp_obj_t lsm303dlhc_write_register(size_t n_args, const mp_obj_t* args ) {
    accelerometer_lsm303dlhc_obj_t *self=NULL;
    mp_buffer_info_t bufinfo;
    uint8_t data[1];  
    uint8_t  accel_i2c_slave_addr=0;
    uint8_t  accel_i2c_slave_reg=0;
    uint16_t timeout=1000;
    uint8_t  reg_size=8;
    
    
    if( n_args == 0 ){
      mp_print_str(MP_PYTHON_PRINTER, " No arguments supplied \n");
    }else if(n_args == 4){
       self        = MP_OBJ_TO_PTR(args[0]);
       pyb_buf_get_for_send(args[1],&bufinfo,data);
       accel_i2c_slave_addr = mp_obj_get_int(args[2]);
       accel_i2c_slave_reg  = mp_obj_get_int(args[3]);
       timeout  = 1000;
       reg_size = 8;
     }else{
      mp_print_str(MP_PYTHON_PRINTER, " Wrong number of arguments!\n");
    }
    //printf("WRITE: MASTER I2C address before =%d\n", i2c_addr );
    accel_i2c_slave_addr = accel_i2c_slave_addr << 1;
    //printf("WRITE: MASTER I2C address after =%d\n", i2c_addr );
    
    for(int i = 0; i < 10; i++){
         HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(self->i2c->i2c,accel_i2c_slave_addr,10,200);
         if ( status == HAL_OK){    
              mp_print_str(MP_PYTHON_PRINTER, "WRITE: accelerometer ready!\n");
              break;
         }
    }
    //I2C_HandleTypeDef* 
    //    HAL_I2C_Mem_Write(I2C1, addr, mem_addr, mem_addr_size, data, data_len, timeout);
    HAL_StatusTypeDef status;
                             
    status=HAL_I2C_Mem_Write(self->i2c->i2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size,  bufinfo.buf, bufinfo.len, timeout);
    if (status != HAL_OK) {
       mp_print_str(MP_PYTHON_PRINTER, "Warning: error writing accelerometer  ");  
    }
    ///INIT DEBUG
    printf("Wrote on I2C->REG %d->%d the value %d\n",mp_obj_get_int(args[2]),mp_obj_get_int(args[3]), mp_obj_get_int(args[1])   );
    ///END DEBUG
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lsm303dlhc_write_register_obj,0,4, lsm303dlhc_write_register);


/// \method read_register(data, data_len, addr, memaddr)
///
/// Read to the memory of an I2C device:
///
///   
///   - `data_len`  data length
///   - `i2c_addr`      is the I2C device address
///   - `i2c_reg`  is the memory location within the I2C device

///i2c.mem_read(2,25,40)
/// Returns 0.
/// This is only valid in master mode.
STATIC mp_obj_t lsm303dlhc_read_register(size_t n_args, const mp_obj_t *args) {
    accelerometer_lsm303dlhc_obj_t *self=NULL;
    //vstr_t    vstr;
    //mp_obj_t  o_ret;
    uint8_t   data_to_read;
    uint8_t   buffer[8];
    uint8_t   accel_i2c_slave_addr  = 0;
    uint8_t   accel_i2c_slave_reg   = 0;
    uint16_t  timeout   = 1000;
    uint8_t   reg_size  = 8;

    if( n_args == 0 ){
      mp_print_str(MP_PYTHON_PRINTER, " No arguments supplied \n");
    }else if(n_args == 4){
       mp_print_str(MP_PYTHON_PRINTER, " Ok: correct  number of arguments!\n");
     }else{
      mp_print_str(MP_PYTHON_PRINTER, " Wrong number of arguments!\n");
    }

    self     = MP_OBJ_TO_PTR(args[0]);
    //o_ret    = pyb_buf_get_for_recv(args[1],&vstr); 
    data_to_read  = mp_obj_get_int(args[1]);    
    accel_i2c_slave_addr = mp_obj_get_int(args[2]);
    accel_i2c_slave_reg  = mp_obj_get_int(args[3]);
    //printf("READ: MASTER I2C address before =%d\n", i2c_addr );
    printf("READ: SLAVE  I2C address  =%d\n", accel_i2c_slave_addr );
    printf("READ: SLAVE  I2C reg address  =%d\n", accel_i2c_slave_reg );
    printf("READ: SLAVE  I2C num sample  =%d\n", data_to_read );

    VSTR_FIXED(vstr,data_to_read);
    accel_i2c_slave_addr = accel_i2c_slave_addr << 1;
    //printf("READ: MASTER I2C address after  =%d\n", i2c_addr );
     //mp_printf(MP_PYTHON_PRINTER, "Z_L=%d, Z_H=%d, Z=%d  ", self->accel_z.low,  self->accel_z.high, self->accel_z.value);
    
    for(int i = 0; i < 10; i++){
         HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(self->i2c->i2c,accel_i2c_slave_addr,10,200);
         if ( status == HAL_OK){    
              mp_print_str(MP_PYTHON_PRINTER, "READ: accelerometer ready!\n");
              break;
         }
    }

    
    HAL_StatusTypeDef status;
   
    //status=HAL_I2C_Mem_Read(self->i2c->i2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size, (uint8_t*)vstr.buf, vstr.len, timeout);
 status=HAL_I2C_Mem_Read(self->i2c->i2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size, buffer, data_to_read, timeout);
    if (status != HAL_OK) {
       mp_print_str(MP_PYTHON_PRINTER, "Warning: error reading accelerometer  ");  
    }

    //if( o_ret != MP_OBJ_NULL){
    //     printf("READ: return o_ret\n" );
    //     return o_ret;
    //} else {
    //   printf("READ: return a new string from vstr\n" );
    //return mp_obj_new_str_from_vstr(&mp_type_bytes,&vstr);
    //}
   if(data_to_read == 2){
      self->accel_x.low = buffer[0];
      self->accel_x.high = buffer[1];
      //printf("Read on DATA_X_LOW=%d  DATA_X_HIGH=%d \n",mp_obj_new_int(data[0]),mp_obj_new_int(data[1])   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",self->accel_x.low,self->accel_x.high   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",buffer[0],buffer[1]  );
      printf("BUFFER[%d,%d,%d,%d,%d,%d]  \n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]  );
      //printf("Read on X_VALUE=%d\n",self->accel_x.value  );
      return mp_obj_new_int(self->accel_x.value ); 
    }  

   if(data_to_read == 4){
      self->accel_x.low = buffer[0];
      self->accel_x.high = buffer[1];
      self->accel_y.low = buffer[2];
      self->accel_y.high = buffer[3];

      //printf("Read on DATA_X_LOW=%d  DATA_X_HIGH=%d \n",mp_obj_new_int(data[0]),mp_obj_new_int(data[1])   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",self->accel_x.low,self->accel_x.high   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",buffer[0],buffer[1]  );
      printf("BUFFER[%d,%d,%d,%d,%d,%d]  \n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]  );
      //printf("Read on X_VALUE=%d\n",self->accel_x.value  );
        mp_obj_t tuple[2];
       tuple[0] = mp_obj_new_int(self->accel_x.value);
       tuple[1] = mp_obj_new_int(self->accel_y.value);
       return mp_obj_new_tuple(2, tuple); 
    }  

   if(data_to_read == 6){
      self->accel_x.low = buffer[0];
      self->accel_x.high = buffer[1];
      self->accel_y.low = buffer[2];
      self->accel_y.high = buffer[3];
      self->accel_z.low = buffer[4];
      self->accel_z.high = buffer[5];


      //printf("Read on DATA_X_LOW=%d  DATA_X_HIGH=%d \n",mp_obj_new_int(data[0]),mp_obj_new_int(data[1])   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",self->accel_x.low,self->accel_x.high   );
      //printf("Read on X_LOW=%d  X_HIGH=%d \n",buffer[0],buffer[1]  );
      printf("BUFFER[%d,%d,%d,%d,%d,%d]  \n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]  );
      //printf("Read on X_VALUE=%d\n",self->accel_x.value  );
        mp_obj_t tuple[3];
       tuple[0] = mp_obj_new_int(self->accel_x.value);
       tuple[1] = mp_obj_new_int(self->accel_y.value);
       tuple[2] = mp_obj_new_int(self->accel_z.value);
       return mp_obj_new_tuple(3, tuple); 
    }  


    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lsm303dlhc_read_register_obj,0,4, lsm303dlhc_read_register);


//Class method 'set'
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
    union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;

    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);


    ret=read_register_helper(self->i2c->i2c, OUT_X_A, &accel.low );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X reading \n");
      return mp_const_false;
    }
    x=accel.value;


    ret=read_register_helper(self->i2c->i2c, OUT_Y_A, &accel.low );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y reading \n");
      return mp_const_false;
    }
    y=accel.value;


    ret=read_register_helper(self->i2c->i2c, OUT_Z_A, &accel.low );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z reading \n");
      return mp_const_false;
    }
    z=accel.value;


    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_int(x);
    tuple[1] = mp_obj_new_int(y);
    tuple[2] = mp_obj_new_int(z);
    
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_obj, lsm303dlhc_read_axes);


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



//  --------------------------------
//Class method 'read_axes_filtered'
STATIC mp_obj_t lsm303dlhc_read_axes_filtered(mp_obj_t self_in ) {
    int16_t x;
    int16_t y;
    int16_t z;
    //bool ret = false;

    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    read_axes_filtered_helper(self->i2c->i2c, &x, &y, &z );


    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_int(x);
    tuple[1] = mp_obj_new_int(y);
    tuple[2] = mp_obj_new_int(z);
    
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_filtered_obj, lsm303dlhc_read_axes_filtered);



STATIC const mp_rom_map_elem_t lsm303dlhc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_axes), MP_ROM_PTR(&lsm303dlhc_read_axes_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_axes_filtered), MP_ROM_PTR(&lsm303dlhc_read_axes_filtered_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_register), MP_ROM_PTR(&lsm303dlhc_write_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_register), MP_ROM_PTR(&lsm303dlhc_read_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_setup), MP_ROM_PTR(&lsm303dlhc_setup_obj) },
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


