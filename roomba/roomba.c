#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "py/malloc.h"
#include <py/builtin.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objmodule.h"

#define  ROOMBA_NAME_LEN         50
#define  ROOMBA_MODEL_LEN        50
#define  NUMBER_OF_STATES        15
#define  NUMBER_OF_TRANSACTIONS  15


typedef struct _roomba_roomba_obj_t {
    mp_obj_base_t base;
    int16_t a;
    int16_t b;
    int16_t position_free_slot_array_states;
    int16_t position_free_slot_array_transactions;

    char* transactions[NUMBER_OF_TRANSACTIONS];    
    char* states[NUMBER_OF_STATES];
    char  name[ROOMBA_NAME_LEN];
    char  model[ROOMBA_MODEL_LEN];
} roomba_roomba_obj_t;

const mp_obj_type_t roomba_roomba_type;

STATIC void roomba_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
 
    mp_print_str(print, "roomba(NAME: ");
    mp_obj_print_helper(print, mp_obj_new_str(self->name,strlen(self->name)), PRINT_REPR);
    mp_print_str(print, ", MODEL: ");
    mp_obj_print_helper(print, mp_obj_new_str(self->model,strlen(self->model)), PRINT_REPR);
    mp_print_str(print, ")");
}

STATIC mp_obj_t roomba_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 2, true);
    roomba_roomba_obj_t *self = m_new_obj(roomba_roomba_obj_t);
    self->base.type = &roomba_roomba_type;
 
    GET_STR_DATA_LEN(args[0], str_name,  str_name_len);
    GET_STR_DATA_LEN(args[1], str_model, str_model_len);
 
    strncpy(self->name,(const char*)str_name,str_name_len);
    strncpy(self->model,(const  char*)str_model,str_model_len);
    self->position_free_slot_array_states=0; 
    self->position_free_slot_array_transactions=0;

    return MP_OBJ_FROM_PTR(self);
}

//  ----------------Class methods ----------------
//        
//
// Class method 'sum'
STATIC mp_obj_t roomba_sum(mp_obj_t self_in) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->a + self->b);
}

MP_DEFINE_CONST_FUN_OBJ_1(roomba_sum_obj, roomba_sum);



//Class method 'add_states'
STATIC mp_obj_t roomba_add_state(mp_obj_t self_in, mp_obj_t str_state_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_state_in, str_state,  str_state_len);


    
    char* s = (char*)m_malloc(sizeof(char) * str_state_len+1);
    strcpy(s, (const char*)str_state);
    self->states[self->position_free_slot_array_states]=s;
        
    self->position_free_slot_array_states+=1;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(roomba_add_state_obj, roomba_add_state);

//Class method 'view_states'
STATIC mp_obj_t roomba_view_states(mp_obj_t self_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if(self->position_free_slot_array_states == 0){
       mp_print_str(MP_PYTHON_PRINTER, "You haven't added any status yet.");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 

    }else{
       mp_print_str(MP_PYTHON_PRINTER, "List of states:  ");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
      
       for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->states[i] );
           mp_print_str(MP_PYTHON_PRINTER, " -- ");
       }
      mp_print_str(MP_PYTHON_PRINTER, "\n");
    } 
        
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(roomba_view_states_obj, roomba_view_states);


//Class method 'exist_states'
STATIC mp_obj_t roomba_exist_state(mp_obj_t self_in, mp_obj_t str_state_in ) {
    int ret;
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_state_in, str_state,  str_state_len);
    
     for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
         ret=strcmp((const char*)str_state,(const char*)self->states[i]);
         if( ret == 0 )
              return mp_const_true;
      }
         
    return mp_const_false;
}

MP_DEFINE_CONST_FUN_OBJ_2(roomba_exist_state_obj, roomba_exist_state);


//from->to
//Class method 'add_transaction'
STATIC mp_obj_t roomba_add_transaction(mp_obj_t self_in, mp_obj_t str_from_in,mp_obj_t str_to_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_from_in, str_from,  str_from_len);
    GET_STR_DATA_LEN(str_to_in, str_to,  str_to_len);

    if( roomba_exist_state(self_in,str_from_in ) == mp_const_false ){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_from );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
       return mp_const_false;
    }

    if( roomba_exist_state(self_in,str_to_in ) == mp_const_false){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_to );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
       return mp_const_false;
    }

    
    char* f = (char*)m_malloc(sizeof(char) * str_from_len+1);
    strcpy(f, (const char*)str_from);
    self->transactions[self->position_free_slot_array_transactions]=f;
    self->position_free_slot_array_transactions+=1;

    char* t = (char*)m_malloc(sizeof(char) * str_to_len+1);
    strcpy(t, (const char*)str_to);
    self->transactions[self->position_free_slot_array_transactions]=t;
    self->position_free_slot_array_transactions+=1;


    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(roomba_add_transaction_obj, roomba_add_transaction);

//Class method 'view_transactions'
STATIC mp_obj_t roomba_view_transactions(mp_obj_t self_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if(self->position_free_slot_array_transactions == 0){
       mp_print_str(MP_PYTHON_PRINTER, "You haven't added any transaction yet.");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 

    }else{
       mp_print_str(MP_PYTHON_PRINTER, "List of transactions:  ");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
      
       for(int16_t i=0; i<self->position_free_slot_array_transactions;i=i+2){
           mp_print_str(MP_PYTHON_PRINTER, "From:  ");
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->transactions[i] );
           mp_print_str(MP_PYTHON_PRINTER, " To:  ");
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->transactions[i+1] );
           mp_print_str(MP_PYTHON_PRINTER, "\n");
       }
      mp_print_str(MP_PYTHON_PRINTER, "\n");
    } 
        
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(roomba_view_transactions_obj, roomba_view_transactions);





//Class method 'exist_transaction'
STATIC mp_obj_t roomba_exist_transaction(mp_obj_t self_in, mp_obj_t str_from_in,mp_obj_t str_to_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_from_in, str_from,  str_from_len);
    GET_STR_DATA_LEN(str_to_in, str_to,  str_to_len);

    if( roomba_exist_state(self_in,str_from_in ) == mp_const_false ){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_from );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
       return mp_const_false;
    }

    if( roomba_exist_state(self_in,str_to_in ) == mp_const_false){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_to );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
       return mp_const_false;
    }

    //Bisogna verificare le coppie di stati
    // for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
    //     ret=strcmp((const char*)str_state,(const char*)self->states[i]);
    //     if( ret == 0 )
    //          return mp_const_true;
    //  }
         
    return mp_const_false;
}

MP_DEFINE_CONST_FUN_OBJ_3(roomba_exist_transaction_obj, roomba_exist_transaction);







//robot.is_transaction_allowed("A","C") //-> False
//Class method 'is_transaction_allowed'
STATIC mp_obj_t roomba_is_transaction_allowed(mp_obj_t self_in, mp_obj_t str_from_in,mp_obj_t str_to_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_from_in, str_from,  str_from_len);
    GET_STR_DATA_LEN(str_to_in, str_to,  str_to_len);

    if( roomba_exist_state(self_in,str_from_in ) == mp_const_false ){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_from );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        return mp_const_false;
    }

    if( roomba_exist_state(self_in,str_to_in ) == mp_const_false){
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        mp_print_str(MP_PYTHON_PRINTER, "You didn't add this state: ");
        mp_print_str(MP_PYTHON_PRINTER,(const char*)str_to );
        mp_print_str(MP_PYTHON_PRINTER, "\n");
        return mp_const_false;
    }

    //Bisogna verificare che la transizione sia tra le possibili
    //e poi verificare se è permessa quindi in qualche maniera è in relazione
    //all'evento che la scatenerebbe
    // for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
    //     ret=strcmp((const char*)str_state,(const char*)self->states[i]);
    //     if( ret == 0 )
    //          return mp_const_true;
    //  }
         
    return mp_const_false;
}

MP_DEFINE_CONST_FUN_OBJ_3(roomba_is_transaction_allowed_obj, roomba_is_transaction_allowed);





STATIC const mp_rom_map_elem_t roomba_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mysum), MP_ROM_PTR(&roomba_sum_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_state), MP_ROM_PTR(&roomba_add_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_view_states), MP_ROM_PTR(&roomba_view_states_obj) },
    { MP_ROM_QSTR(MP_QSTR_exist_state), MP_ROM_PTR(&roomba_exist_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_transaction), MP_ROM_PTR(&roomba_add_transaction_obj) },
    { MP_ROM_QSTR(MP_QSTR_view_transactions), MP_ROM_PTR(&roomba_view_transactions_obj) },
    { MP_ROM_QSTR(MP_QSTR_exist_transaction), MP_ROM_PTR(&roomba_exist_transaction_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_transaction_allowed), MP_ROM_PTR(&roomba_is_transaction_allowed_obj) },

};


STATIC MP_DEFINE_CONST_DICT(roomba_locals_dict, roomba_locals_dict_table);

const mp_obj_type_t roomba_roomba_type = {
    { &mp_type_type },
    .name = MP_QSTR_roomba,
    .print = roomba_print,
    .make_new = roomba_make_new,
    .locals_dict = (mp_obj_dict_t*)&roomba_locals_dict,
};

//
// Utility functions
//


//
//
// Module functions
//
//
STATIC mp_obj_t roomba_add(const mp_obj_t o_in) {
    roomba_roomba_obj_t *class_instance = MP_OBJ_TO_PTR(o_in);
    return mp_obj_new_int(class_instance->a + class_instance->b);
}

MP_DEFINE_CONST_FUN_OBJ_1(roomba_add_obj, roomba_add);

STATIC const mp_map_elem_t roomba_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_roomba) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_roomba), (mp_obj_t)&roomba_roomba_type },	
    { MP_OBJ_NEW_QSTR(MP_QSTR_add), (mp_obj_t)&roomba_add_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_roomba_globals,
    roomba_globals_table
);

const mp_obj_module_t roomba_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_roomba_globals,
};

MP_REGISTER_MODULE(MP_QSTR_roomba, roomba_user_cmodule, MODULE_ROOMBA_ENABLED);


    //DEBUG
    //mp_print_str(MP_PYTHON_PRINTER,(const char*)str_state );
    //mp_printf(MP_PYTHON_PRINTER, "STRING=  %s\n", str_state);
    //mp_printf(MP_PYTHON_PRINTER, " LENGTH= %d\n", str_state_len);
    //-----   
 
