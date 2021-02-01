#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#define DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN   ((uint8_t)0x77)
#define DATA_RATE_10_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN    ((uint8_t)0x27)
#define DATA_RATE_100_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN   ((uint8_t)0x57)
#define CONTINUOS_UPDATE_LSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_DISABLE_SPI_4_WIRE   ((uint8_t)0x00)
#define CONTINUOS_UPDATE_LSB_LOWER_ADDR_2_G_HIGH_RESOLUTION_ENABLE_SPI_4_WIRE   ((uint8_t)0x08)
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
#define OUT_X_A    ((uint8_t)0x28)
#define OUT_Y_A    ((uint8_t)0x2A)
#define OUT_Z_A    (uint8_t)(0x2C)

bool write_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t data ){

    //uint16_t   accel_i2c_slave_addr = ACCEL_ADDR;
    //uint16_t   reg_size = 8;
    //uint16_t   how_many_data = 1;
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;
    uint8_t buffer[2];
    //accel_i2c_slave_addr = accel_i2c_slave_addr << 1;   
    // ORing 0x80 auto-increments the register for each read  
    //accel_i2c_slave_reg |= 0x80;       

    //status=HAL_I2C_Mem_Write(hi2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size,  &data, how_many_data, timeout);
    //if (status != HAL_OK) {
    //   ret = false;  
    //}
    buffer[0] = (uint8_t) accel_i2c_slave_reg;
    buffer[1] = data;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buffer, 2, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "WRITE_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 


 return ret;
}

/*
bool read_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t* data ){

    uint16_t   accel_i2c_slave_addr = ACCEL_ADDR;
    uint16_t   reg_size = 8;
    uint16_t   how_many_data = 2;
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;

    accel_i2c_slave_addr = accel_i2c_slave_addr << 1;
    status=HAL_I2C_Mem_Read(hi2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size,  data, how_many_data, timeout);
    if (status != HAL_OK) {
       ret = false;  
    }

 return ret;
}
*/

bool read_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t* data ){
    
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;
    uint8_t buf[2];

    //data[0] = accel_i2c_slave_reg;
    buf[0] = accel_i2c_slave_reg;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 

      // Read 2 bytes from the accelerometer  register
      status = HAL_I2C_Master_Receive(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
      if ( status != HAL_OK ) {
        mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Receive error\n");
        ret = false;
      }
     data[0] = buf[0];

    //data[0] = accel_i2c_slave_reg;
    buf[0] = accel_i2c_slave_reg+1;
    status = HAL_I2C_Master_Transmit(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
    if ( status != HAL_OK ) {
      mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Transmit error\n");
      ret = false;
    } 

      // Read 2 bytes from the accelerometer  register
      status = HAL_I2C_Master_Receive(hi2c, ACCEL_ADDR<<1, buf, 1, timeout);
      if ( status != HAL_OK ) {
        mp_print_str(MP_PYTHON_PRINTER, "READ_REGISTER_HELPER: Master_Receive error\n");
        ret = false;
      }
     data[1] = buf[0];



 return ret;
}



bool read_register_and_convert_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, int16_t* data ){

    uint16_t   accel_i2c_slave_addr = ACCEL_ADDR;
    uint16_t   reg_size = 8;
    uint16_t   how_many_data = 1;
    uint16_t   timeout = 1000;
    HAL_StatusTypeDef status;
    bool ret = true;

    union _accel{
     struct {
        uint8_t low;
        uint8_t high;
     };   
     int16_t value;
    }accel;


    accel_i2c_slave_addr = accel_i2c_slave_addr << 1;
    status=HAL_I2C_Mem_Read(hi2c, accel_i2c_slave_addr, accel_i2c_slave_reg, reg_size,  &accel.low, how_many_data, timeout);
    if (status != HAL_OK) {
       ret = false;  
    }

    status=HAL_I2C_Mem_Read(hi2c, accel_i2c_slave_addr, accel_i2c_slave_reg+1, reg_size,  &accel.high, how_many_data, timeout);
    if (status != HAL_OK) {
       ret = false;  
    }


    *data = accel.value;

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


/// \function i2c_write(data, data_len, addr, memaddr, timeout=5000, addr_size=8)
///
/// Write to the memory of an I2C device:
///
///   - `data`      can be an integer or a buffer to write from
///   - `data_len`  data length
///   - `addr`      is the I2C device address
///   - `mem_addr`  is the memory location within the I2C device
///   - `timeout`   is the timeout in milliseconds to wait for the write (default 1000)
///   - `addr_size` selects width of memaddr: 8 or 16 bits (default 8 )
///
/// Returns 0.
/// This is only valid in master mode.
//int i2c_write_register(uint8_t* data, uint8_t data_len, uint8_t addr, uint8_t mem_addr, uint16_t timeout, uint8_t mem_addr_size ) {
//
//    addr = addr << 1;
//    I2C_HandleTypeDef*  =  ;
//    HAL_I2C_Mem_Write(I2C1, addr, mem_addr, mem_addr_size, data, data_len, timeout);
//
//
//  return 0;
//}

STATIC void lsm303dlhc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
 
    mp_print_str(print, "Accelerometer lsm303dlhc ");
    mp_printf(print, " X= %d, Y= %d, Z=%d  ", self->accel_x.value,  self->accel_y.value, self->accel_z.value);

}


STATIC mp_obj_t lsm303dlhc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    //uint8_t data[2];
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
    //int16_t x;
    //int16_t y;
    //int16_t z;
    bool ret = false;
    //
    //uint8_t xl,xh,yl,yh,zl,zh;
    uint8_t  X[2];
    uint8_t  Y[2];
    uint8_t  Z[2];


    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    ret=read_register_helper(self->i2c->i2c, OUT_X_A, X );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X reading \n");
      return mp_const_false;
    }

    ret=read_register_helper(self->i2c->i2c, OUT_Y_A, Y );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y reading \n");
      return mp_const_false;
    }

    ret=read_register_helper(self->i2c->i2c, OUT_Z_A, Z );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z reading \n");
      return mp_const_false;
    }





/*
    ret=read_register_helper(self->i2c->i2c, OUT_X_L_A, &xl );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X_LOW reading ");
      return mp_const_false;
    }
    ret=read_register_helper(self->i2c->i2c, OUT_X_H_A, &xh );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X_LOW reading ");
      return mp_const_false;
    }
    ret=read_register_helper(self->i2c->i2c, OUT_Y_L_A, &yl );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y_LOW reading ");
      return mp_const_false;
    }
    ret=read_register_helper(self->i2c->i2c, OUT_Y_H_A, &yh );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y_LOW reading ");
      return mp_const_false;
    }
    ret=read_register_helper(self->i2c->i2c, OUT_Z_L_A, &zl );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z_LOW reading ");
      return mp_const_false;
    }
    ret=read_register_helper(self->i2c->i2c, OUT_Z_H_A, &zh );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z_LOW reading ");
      return mp_const_false;
    }

*/

//bool read_register_helper(I2C_HandleTypeDef* hi2c, uint16_t accel_i2c_slave_reg, uint8_t* data ){

    /*
    ret=read_register_and_convert_helper(self->i2c->i2c, OUT_X_L_A, &x );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during X reading ");
      return mp_const_false;
    }

    ret=read_register_and_convert_helper(self->i2c->i2c, OUT_Y_L_A, &y );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Y reading ");
      return mp_const_false;
    }

    ret=read_register_and_convert_helper(self->i2c->i2c, OUT_Z_L_A, &z );
    if( ret == false){
      mp_printf(MP_PYTHON_PRINTER, "Error during Z reading ");
      return mp_const_false;
    }

    */


    mp_obj_t tuple[6];
    tuple[0] = mp_obj_new_int(X[0]);
    tuple[1] = mp_obj_new_int(X[1]);
    tuple[2] = mp_obj_new_int(Y[0]);
    tuple[3] = mp_obj_new_int(Y[1]);
    tuple[4] = mp_obj_new_int(Z[0]);
    tuple[5] = mp_obj_new_int(Z[1]);
    


    return mp_obj_new_tuple(6, tuple);


    //mp_obj_t tuple[NUM_AXIS];
    //tuple[0] = mp_obj_new_int(x);
    //tuple[1] = mp_obj_new_int(y);
    //tuple[2] = mp_obj_new_int(z);
    


    //return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_obj, lsm303dlhc_read_axes);

STATIC const mp_rom_map_elem_t lsm303dlhc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_axes), MP_ROM_PTR(&lsm303dlhc_read_axes_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_register), MP_ROM_PTR(&lsm303dlhc_write_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_register), MP_ROM_PTR(&lsm303dlhc_read_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_setup), MP_ROM_PTR(&lsm303dlhc_setup_obj) },

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


