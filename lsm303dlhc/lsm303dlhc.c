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

//From ports/stm32/machine_i2c.c
typedef struct _machine_hard_i2c_obj_t {
    mp_obj_base_t base;
    i2c_t *i2c;
    mp_hal_pin_obj_t scl;
    mp_hal_pin_obj_t sda;
} machine_hard_i2c_obj_t;



/******************************************************************************/
/* MicroPython bindings                                                      */

typedef struct _accelerometer_lsm303dlhc_obj_t {
    mp_obj_base_t base;
    //pyb_i2c_obj_t*  i2c;
    //machine_i2c_obj_t* i2c;
    //mp_machine_soft_i2c_obj_t*  i2c;
    //struct _machine_hard_i2c_obj_t* i2c;
    //struct machine_hard_i2c_obj_t* i2c;
    //machine_soft_i2c_obj_t* i2c;
    struct _machine_hard_i2c_obj_t* i2c;
    //struct machine_hard_i2c_obj_t* i2c;
   //pyb_i2c_obj_t* i2c; 

    union _accel_x{
     uint8_t low;
     uint8_t high;
     int16_t value;
    }accel_x;

    union _accel_y{
     uint8_t low;
     uint8_t high;
     int16_t value;
    }accel_y;
    
    union _accel_z{
     uint8_t low;
     uint8_t high;
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

//////DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN = const(0x77)
//////CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE = const(0x08)
//
//from machine import I2C
//
//i2c = I2C(1) 
//i2c.scan()
//
//i2c.writeto_mem(25,32,b'\x77')
//i2c.writeto_mem(25,35,b'\x08')
//
//i2c.readfrom_mem(25,40,1)
//i2c.readfrom_mem(25,41,1)
//

STATIC mp_obj_t lsm303dlhc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //uint8_t data[2];
    //mp_arg_check_num(n_args, n_kw, 2, 2, true);
    //mp_arg_check_num(n_args, n_kw, 0, 0, true);
    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    accelerometer_lsm303dlhc_obj_t *self = m_new_obj(accelerometer_lsm303dlhc_obj_t);
    self->base.type = &accelerometer_lsm303dlhc_type;
    
    //if (mp_obj_get_type(args[0]) == &mp_machine_soft_i2c_type) {
    if (mp_obj_get_type(args[0]) == &machine_hard_i2c_type) {
    //if (mp_obj_get_type(args[0]) == &pyb_i2c_type) {
         self->i2c=args[0];
    }else{
         mp_print_str(MP_PYTHON_PRINTER, "The argumet is not a I2C type.");
    }
 
    // pyb_i2c_mem_write(self->i2c,0x77,25,32);
   
    //self->i2c->machine_i2c_writeto_mem(25,35,0x08);
   
    self->i2c->writeto_mem(25,35,0x08);

    //i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);
    //mp_hal_delay_ms(30);
    
    //data[0] = CTRL_REG1_A ;
    //data[1] = DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN ;
    //i2c_writeto(I2C1,ACCEL_ADDR , data, 2, true);
    //mp_printf(MP_PYTHON_PRINTER, "Indirizzo I2C=%d, REGISTRO=%d, CONFIG_BYTE=%d  ", ACCEL_ADDR,  CTRL_REG1_A, DATA_RATE_400_HZ_NORMAL_MODE_X_EN_Y_EN_Z_EN);
    //mp_hal_delay_ms(30);
    
    
    //data[0] = CTRL_REG4_A ;
    //data[1] = CONTINUOS_UPDATE_LITTLE_ENDIAN_2_G_HIGH_RESOLUTION_SPI_4_WIRE ;
    //i2c_writeto(I2C1, ACCEL_ADDR, data, 2, true);
    //mp_printf(MP_PYTHON_PRINTER, "Indirizzo I2C=%d, REGISTRO=%d   ", ACCEL_ADDR,  CTRL_REG4_A);
    //mp_hal_delay_ms(30);

    self->accel_x.value=0;
    self->accel_y.value=0;
    self->accel_z.value=0;
   

    return MP_OBJ_FROM_PTR(self);
}

//
//
//
//
//
//STATIC mp_obj_t read_axis(int axis) {
//    uint8_t data[1] = { axis };
//    i2c_writeto(I2C1, ACCEL_ADDR, data, 1, false);
//    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
//    return mp_obj_new_int(ACCEL_AXIS_SIGNED_VALUE(data[0]));
//}
//
/// \method x()
/// Get the x-axis value.
//STATIC mp_obj_t pyb_accel_x(mp_obj_t self_in) {
//    return read_axis(ACCEL_REG_X);
//}
//STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_accel_x_obj, pyb_accel_x);

//  ----------------Class methods ----------------
//Class method 'add_state'
STATIC mp_obj_t lsm303dlhc_read_axes(mp_obj_t self_in ) {
    uint8_t data[2] ;
    accelerometer_lsm303dlhc_obj_t *self = MP_OBJ_TO_PTR(self_in);

    data[1] = OUT_X_L_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_x.low = data[0];
 
    data[1] = OUT_X_H_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_x.high = data[0];

    mp_printf(MP_PYTHON_PRINTER, "X_L=%d, X_H=%d, X=%d  ", self->accel_x.low,  self->accel_x.high, self->accel_x.value);

    data[1] = OUT_Y_L_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_y.low = data[0];
 
    data[1] = OUT_Y_H_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_y.high = data[0];

    mp_printf(MP_PYTHON_PRINTER, "Y_L=%d, Y_H=%d, Y=%d  ", self->accel_y.low,  self->accel_y.high, self->accel_y.value);

    data[1] = OUT_Z_L_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_z.low = data[0];
 
    data[1] = OUT_Z_H_A;
    i2c_readfrom(I2C1, ACCEL_ADDR, data, 1, true);
    self->accel_z.high = data[0];

    mp_printf(MP_PYTHON_PRINTER, "Z_L=%d, Z_H=%d, Z=%d  ", self->accel_z.low,  self->accel_z.high, self->accel_z.value);

    mp_obj_t tuple[NUM_AXIS];
    tuple[0] = mp_obj_new_int(self->accel_x.value);
    tuple[1] = mp_obj_new_int(self->accel_y.value);
    tuple[2] = mp_obj_new_int(self->accel_z.value);
    
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(lsm303dlhc_read_axes_obj, lsm303dlhc_read_axes);

STATIC const mp_rom_map_elem_t lsm303dlhc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_axes), MP_ROM_PTR(&lsm303dlhc_read_axes_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_mysum), MP_ROM_PTR(&roomba_sum_obj) },
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


 
