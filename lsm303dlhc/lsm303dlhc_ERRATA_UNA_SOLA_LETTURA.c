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
#define DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN   (0x77)
#define CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE   (0x08)
#define I2C_TIMEOUT_MS (50)
#define ACCEL_ADDR   (0x19)
#define CTRL_REG1_A  (0x20)
#define CTRL_REG4_A  (0x23)
#define OUT_X_L_A    (0x28)
#define OUT_X_H_A    (0x29)
#define OUT_Y_L_A    (0x2A)
#define OUT_Y_H_A    (0x2B)
#define OUT_Z_L_A    (0x2C)
#define OUT_Z_H_A    (0x2D)



//typedef struct _pyb_i2c_obj_t {
//    mp_obj_base_t base;
//    I2C_HandleTypeDef *i2c;
//    const dma_descr_t *tx_dma_descr;
//    const dma_descr_t *rx_dma_descr;
//    bool *use_dma;
//} pyb_i2c_obj_t;

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
   //if (mp_obj_get_type(args[0]) == &pyb_i2c_obj_t) {
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
    uint8_t data_to_read;
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





    //if(data_to_read == 2){
    //   self->accel_x.low = data[0];
    //   self->accel_x.high = data[1];
    //   //printf("Read on DATA_X_LOW=%d  DATA_X_HIGH=%d \n",mp_obj_new_int(data[0]),mp_obj_new_int(data[1])   );
    //   printf("Read on X_LOW=%d  X_HIGH=%d \n",self->accel_x.low,self->accel_x.high   );
    //   printf("Read on X_VALUE=%d\n",self->accel_x.value  );
    //   return mp_obj_new_int(self->accel_x.value ); 
    //}else if(data_to_read == 4){
    //   self->accel_x.low = data[0];
    //   self->accel_x.high = data[1];
    //   self->accel_y.low = data[2];
    //   self->accel_y.high = data[3];
    //   printf("Read on X_LOW=%d  X_HIGH=%d  Y_LOW=%d  Y_HIGH=%d \n",data[0],data[1],data[2],data[3]  );
    //   mp_obj_t tuple[2];
    //   tuple[0] = mp_obj_new_int(self->accel_x.value);
    //   tuple[1] = mp_obj_new_int(self->accel_y.value);
    //   return mp_obj_new_tuple(2, tuple);
    //}else if(data_to_read == 6){
    //   self->accel_x.low = data[0];
    //   self->accel_x.high = data[1];
    //   self->accel_y.low = data[2];
    //   self->accel_y.high = data[3];  
    //   self->accel_y.low = data[4];
    //   self->accel_y.high = data[5];
    //   mp_obj_t tuple[3];
    //   tuple[0] = mp_obj_new_int(self->accel_x.value);
    //   tuple[1] = mp_obj_new_int(self->accel_y.value);
    //   tuple[2] = mp_obj_new_int(self->accel_z.value);
    //   return mp_obj_new_tuple(3, tuple);  
    //
    //}else{
    //  mp_print_str(MP_PYTHON_PRINTER, "You can read only 2 or 4 or 6 samples \n");
    //}
    
    ///INIT DEBUG
    //printf("Read on I2C->REG %d -> %d \n",mp_obj_get_int(args[2]),mp_obj_get_int(args[3])   );
    ///END DEBUG
    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lsm303dlhc_read_register_obj,0,4, lsm303dlhc_read_register);


////Class method 'set'
//STATIC mp_obj_t lsm303dlhc_set(mp_obj_t self_in ) {
//    uint8_t data[2] ;
//
//    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);
//    
//    data[0]=DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN;
//    lsm303dlhc_write_register(self,data,1,25,32);
//    
//    data[0]=CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE ;
//    lsm303dlhc_write_register(self,data,1,25,32);   
//
//    return mp_const_none;
//}
//MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_set_obj, lsm303dlhc_set);
//  --------------------------------
//Class method 'read_axes'
STATIC mp_obj_t lsm303dlhc_read_axes(mp_obj_t self_in ) {
    //uint8_t data[2] ;

    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    //data[0] = OUT_X_L_A ;
    //i2c_writeto(I2C1, ACCEL_ADDR, data, 1, false);
    //i2c_readfrom(I2C1,ACCEL_ADDR,data, 2, true );
    //self->accel_x.low = data[0];
    //self->accel_x.high = data[1]; 

    
    //data[0] = OUT_Y_L_A ;
    //i2c_writeto(I2C1, ACCEL_ADDR, data, 1, false);
    //i2c_readfrom(I2C1,ACCEL_ADDR,data, 2, true );
    //self->accel_y.low = data[0];
    //self->accel_y.high = data[1];


    //data[0] = OUT_Z_L_A ;
    //i2c_writeto(I2C1, ACCEL_ADDR, data, 1, false);
    //i2c_readfrom(I2C1,ACCEL_ADDR,data, 2, true );
    //self->accel_z.low = data[0];
    //self->accel_z.high = data[1]; 


    //mp_printf(MP_PYTHON_PRINTER, "Z_L=%d, Z_H=%d, Z=%d  ", self->accel_z.low,  self->accel_z.high, self->accel_z.value);

    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_int(self->accel_x.value);
    tuple[1] = mp_obj_new_int(self->accel_y.value);
    tuple[2] = mp_obj_new_int(self->accel_z.value);
    
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_obj, lsm303dlhc_read_axes);

STATIC const mp_rom_map_elem_t lsm303dlhc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_axes), MP_ROM_PTR(&lsm303dlhc_read_axes_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_register), MP_ROM_PTR(&lsm303dlhc_write_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_register), MP_ROM_PTR(&lsm303dlhc_read_register_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_add_state), MP_ROM_PTR(&roomba_add_state_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_view_states), MP_ROM_PTR(&roomba_view_states_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_exist_state), MP_ROM_PTR(&roomba_exist_state_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_add_event), MP_ROM_PTR(&roomba_add_event_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_view_events), MP_ROM_PTR(&roomba_view_events_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_sending_event), MP_ROM_PTR(&roomba_sending_event_obj) },

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
// Utility functions
//


//
//
// Module functions
//
//
//STATIC mp_obj_t roomba_add(const mp_obj_t o_in) {
//    //roomba_roomba_obj_t *class_instance = MP_OBJ_TO_PTR(o_in);
//    //return mp_obj_new_int(class_instance->a + class_instance->b);
//    return mp_const_none;
//}
//
//MP_DEFINE_CONST_FUN_OBJ_1(roomba_add_obj, roomba_add);
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


 
