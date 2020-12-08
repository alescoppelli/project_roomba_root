#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <py/builtin.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/mperrno.h"
#include "ports/stm32/uart.h"


#define  ROOMBA_NAME_LEN            50
#define  ROOMBA_MODEL_LEN           50
#define  NUMBER_OF_STATES           15
#define  NUMBER_OF_EVENTS           15
#define  NUMBER_OF_TRANSACTIONS     15
#define  MAX_LEN_STR_EVENT_OR_STATE 30
#define  TIMEOUT_TX_MILLISECONDS    50

typedef struct _event_t {
      char name[MAX_LEN_STR_EVENT_OR_STATE];
      char from_state[MAX_LEN_STR_EVENT_OR_STATE];
      char to_state[MAX_LEN_STR_EVENT_OR_STATE];
} event_t;


typedef struct _roomba_roomba_obj_t {
    mp_obj_base_t base;
    int16_t position_free_slot_array_states;
    int16_t position_free_slot_array_events;
    int16_t position_free_slot_array_transactions;

    char* transactions[NUMBER_OF_TRANSACTIONS];    
    char* states[NUMBER_OF_STATES];
    event_t* events[NUMBER_OF_EVENTS];
    char  name[ROOMBA_NAME_LEN];
    char  model[ROOMBA_MODEL_LEN];
    char  current_state[MAX_LEN_STR_EVENT_OR_STATE];
    pyb_uart_obj_t* serial;
} roomba_roomba_obj_t;

const mp_obj_type_t roomba_roomba_type;

//
//START FUNCTIONS UTILITY
//
char* make_state(char* state  ){
      size_t num_chars=sizeof(&state);
      char*  str_ptr = (char*)m_malloc(sizeof(char) * num_chars+1);
      strcpy(str_ptr,(const char*) state);
      return str_ptr;
}

event_t* make_event(char* event, char* from_state, char* to_state){

    event_t* evt_ptr = (event_t*)m_malloc(sizeof(event_t) );
    strcpy(evt_ptr->name, (const char*) event);
    strcpy(evt_ptr->from_state, (const char*) from_state);
    strcpy(evt_ptr->to_state, (const char*) to_state);

    return evt_ptr;
}
uint8_t from_str_event_to_code_event(char* event){
     uint8_t data=0;
        
   if(!strcmp(event,"START")){
        data=128;
   }else if( !strcmp(event,"CONTROL")   ){
        data=130;
   }else if( !strcmp(event,"SAFE")   ){
        data=131;
   }else if( !strcmp(event,"FULL")   ){
        data=132;
   }else if( !strcmp(event,"POWER")   ){
        data=133;
   }else{
   
   }

  return data;
}

//
//END FUNCTIONS UTILITY
//
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
    //GET_STR_DATA_LEN(args[1], str_model, str_model_len);
 
    strncpy(self->name,(const char*)str_name,str_name_len);
    //strncpy(self->model,(const  char*)str_model,str_model_len);
    if (mp_obj_get_type(args[1]) == &pyb_uart_type) {
         self->serial=args[1];
    }else{
         mp_print_str(MP_PYTHON_PRINTER, "The argumet is not a serial line type.");
    }
 
    self->position_free_slot_array_states=0; 
    self->position_free_slot_array_events=0; 
    self->position_free_slot_array_transactions=0;

    
    self->states[self->position_free_slot_array_states]=make_state("ASLEEP");   
    self->position_free_slot_array_states+=1;
    
    self->states[self->position_free_slot_array_states]=make_state("ON");   
    self->position_free_slot_array_states+=1;
    
    self->states[self->position_free_slot_array_states]=make_state("PASSIVE");   
    self->position_free_slot_array_states+=1;

    self->states[self->position_free_slot_array_states]=make_state("SAFE");   
    self->position_free_slot_array_states+=1;

    self->states[self->position_free_slot_array_states]=make_state("FULL");   
    self->position_free_slot_array_states+=1;

    strcpy(self->current_state, "ASLEEP");
    
 
    self->events[self->position_free_slot_array_events]=make_event("WAKE_UP","ASLEEP","ON");
    self->position_free_slot_array_events+=1;
    
    self->events[self->position_free_slot_array_events]=make_event("START","ON","PASSIVE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("STOP","PASSIVE","ON" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("SENSORS","PASSIVE","PASSIVE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("CONTROL","PASSIVE","SAFE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("SAFETY_FAULT","SAFE","PASSIVE" );
    self->position_free_slot_array_events+=1;      


    self->events[self->position_free_slot_array_events]=make_event("DRIVE","SAFE","SAFE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("SENSORS","SAFE","SAFE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("SAFE","FULL","SAFE" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event( "FULL","SAFE","FULL");
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("SENSORS","FULL","FULL" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("DRIVE","FULL","FULL" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("POWER","SAFE","ASLEEP" );
    self->position_free_slot_array_events+=1;      

    self->events[self->position_free_slot_array_events]=make_event("POWER","FULL","ASLEEP" );
    self->position_free_slot_array_events+=1;      

    return MP_OBJ_FROM_PTR(self);
}


//  ----------------Class methods ----------------
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

//Class method 'view_events'
STATIC mp_obj_t roomba_view_events(mp_obj_t self_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if(self->position_free_slot_array_events == 0){
       mp_print_str(MP_PYTHON_PRINTER, "You haven't added any event yet.");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 

    }else{
       mp_print_str(MP_PYTHON_PRINTER, "List of events ( and associated states):  ");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
      
       for(int16_t i=0; i<self->position_free_slot_array_events;i++){ 
           mp_print_str(MP_PYTHON_PRINTER, " Event:  ");
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->events[i]->name );
           mp_print_str(MP_PYTHON_PRINTER, " ( ");
           mp_print_str(MP_PYTHON_PRINTER, " From:  ");
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->events[i]->from_state );
           mp_print_str(MP_PYTHON_PRINTER, " To:  ");
           mp_print_str(MP_PYTHON_PRINTER,(const char*)self->events[i]->to_state );
           mp_print_str(MP_PYTHON_PRINTER, " ) ");
           mp_print_str(MP_PYTHON_PRINTER, "\n");
       }
      mp_print_str(MP_PYTHON_PRINTER, "\n");
    } 
        
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(roomba_view_events_obj, roomba_view_events);

//Class method 'sending_event'
STATIC mp_obj_t roomba_sending_event(mp_obj_t self_in, mp_obj_t str_sending_event_in ) {
    int ret;
    int errcode;
    int event_founded=0;

    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_sending_event_in, str_sending_event,  str_sending_event_len);

    if(self->position_free_slot_array_states == 0){
       mp_print_str(MP_PYTHON_PRINTER, "You haven't  any status !");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
       return mp_const_none;   
    }
  
    if(self->position_free_slot_array_events == 0){
       mp_print_str(MP_PYTHON_PRINTER, "You haven't  any event !");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
       return mp_const_none;
    }
  
    for(int16_t i=0; i<self->position_free_slot_array_events;i++){ 
         event_founded = 0;
         uint8_t data;
         ret=strcmp((const char*)str_sending_event,(const char*)self->events[i]->name);
         if( ret == 0 ){
              event_founded = 1;
              //START DEBUG
              mp_print_str(MP_PYTHON_PRINTER, "\n");
              mp_print_str(MP_PYTHON_PRINTER, "Founded event: ");
              mp_print_str(MP_PYTHON_PRINTER,(const char*)self->events[i]->name );
              mp_print_str(MP_PYTHON_PRINTER, "\n");
              //END DEBUG

         }
         if( event_founded == 1){
             ret=strcmp((const char*)self->current_state,(const char*)self->events[i]->from_state);
             if( ret == 0){
                //START DEBUG
                mp_print_str(MP_PYTHON_PRINTER, "\n");
                mp_print_str(MP_PYTHON_PRINTER, "Founded from state : ");
                mp_print_str(MP_PYTHON_PRINTER,(const char*)self->events[i]->from_state );
                mp_print_str(MP_PYTHON_PRINTER, "\n");
                //END DEBUG
                //QUI BISOGNA SPEDIRE SULLA SERIALE IL COMANDO ASSOCIATO
                //IL MANUALE INDICA DI ASPETTARE 20mS TRA COMANDO E COMANDO
                 //send_serial_event(self, self->events[i]->name);
                 if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
                    //uart_tx_data(self->serial, &data, 1, &errcode);
                    data=from_str_event_to_code_event(self->events[i]->name);        
                    uart_tx_data(self->serial, &data, 1, &errcode);   
                    //SAREBBE OPPORTUNO METTERE UNA WAIT DI 20 mS       
                 }

                 strcpy(self->current_state, (const char*)self->events[i]->to_state);
                 break;
             } 
         }else{
             mp_print_str(MP_PYTHON_PRINTER, "SENDING EVENT: WTF !!!");
         }


    }



    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(roomba_sending_event_obj, roomba_sending_event);


//Class method 'view_current_state'
STATIC mp_obj_t roomba_view_current_state(mp_obj_t self_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_print_str(MP_PYTHON_PRINTER, "\n"); 
    mp_print_str(MP_PYTHON_PRINTER, "The current state is ");
    mp_print_str(MP_PYTHON_PRINTER,(const char*)self->current_state );
    mp_print_str(MP_PYTHON_PRINTER, "  \n");

    return mp_const_none;
}


MP_DEFINE_CONST_FUN_OBJ_1(roomba_view_current_state_obj, roomba_view_current_state);


STATIC const mp_rom_map_elem_t roomba_locals_dict_table[] = {
     { MP_ROM_QSTR(MP_QSTR_view_states), MP_ROM_PTR(&roomba_view_states_obj) },
     { MP_ROM_QSTR(MP_QSTR_view_events), MP_ROM_PTR(&roomba_view_events_obj) },
     { MP_ROM_QSTR(MP_QSTR_sending_event), MP_ROM_PTR(&roomba_sending_event_obj) },
     { MP_ROM_QSTR(MP_QSTR_view_current_state), MP_ROM_PTR(&roomba_view_current_state_obj) },
   
};


STATIC MP_DEFINE_CONST_DICT(roomba_locals_dict, roomba_locals_dict_table);

const mp_obj_type_t roomba_roomba_type = {
    { &mp_type_type },
    .name = MP_QSTR_roomba,
    .print = roomba_print,
    .make_new = roomba_make_new,
    .locals_dict = (mp_obj_dict_t*)&roomba_locals_dict,
};





STATIC const mp_map_elem_t roomba_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_roomba) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_roomba), (mp_obj_t)&roomba_roomba_type },	
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

