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

    

    return MP_OBJ_FROM_PTR(self);
}

//  ----------------Class methods ----------------
//Class method 'add_state'
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


//Class method 'exist_state'
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


//Class method 'add_event'
STATIC mp_obj_t roomba_add_event(size_t n_args, const mp_obj_t* args ) {

    int ret;
    int founded=0;

    if( n_args != 4 ){
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
       mp_print_str(MP_PYTHON_PRINTER, "Wrong numbers pf parameters !");
       mp_print_str(MP_PYTHON_PRINTER, "\n"); 
      return mp_const_false;
    }
   
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    GET_STR_DATA_LEN(args[1], str_event,      str_event_len);
    GET_STR_DATA_LEN(args[2], str_from_state, str_from_state_len);
    GET_STR_DATA_LEN(args[3], str_to_state,   str_to_state_len);
    


    //PROTOTYPE TOOL FUNCTIONS
    //
     founded=0;
     for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
         ret=strcmp((const char*)str_from_state,(const char*)self->states[i]);
         if( ret == 0 ){
              founded = 1;
         }
     }
     if(founded == 0){
          mp_print_str(MP_PYTHON_PRINTER, "\n"); 
          mp_print_str(MP_PYTHON_PRINTER, "The state ");
          mp_print_str(MP_PYTHON_PRINTER, " -- ");
          mp_print_str(MP_PYTHON_PRINTER,(const char*)str_from_state );
          mp_print_str(MP_PYTHON_PRINTER, " -- ");
          mp_print_str(MP_PYTHON_PRINTER, " doesn't exist! ");
     }
    ////////////////////
    //PROTOTYPE TOOL FUNCTIONS
    //
     founded=0;
     for(int16_t i=0; i<self->position_free_slot_array_states;i++){ 
         ret=strcmp((const char*)str_to_state,(const char*)self->states[i]);
         if( ret == 0 ){
              founded = 1;
         }
     }
     if(founded == 0){
          mp_print_str(MP_PYTHON_PRINTER, "\n"); 
          mp_print_str(MP_PYTHON_PRINTER, "The state ");
          mp_print_str(MP_PYTHON_PRINTER, " -- ");
          mp_print_str(MP_PYTHON_PRINTER,(const char*)str_to_state );
          mp_print_str(MP_PYTHON_PRINTER, " -- ");
          mp_print_str(MP_PYTHON_PRINTER, " doesn't exist! ");
     }
    ////////////////////

     
    event_t* e = (event_t*)m_malloc(sizeof(event_t) );
    strcpy(e->name, (const char*)str_event);
    strcpy(e->from_state, (const char*)str_from_state);
    strcpy(e->to_state, (const char*)str_to_state);
    self->events[self->position_free_slot_array_events]=e;
    
  
    self->position_free_slot_array_events+=1;
    return mp_const_none;
}


MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(roomba_add_event_obj,4,4, roomba_add_event);

//typedef struct _event_t {
//      char name[MAX_LEN_STR_EVENT_OR_STATE];
//      char from_state[MAX_LEN_STR_EVENT_OR_STATE];
//      char to_state[MAX_LEN_STR_EVENT_OR_STATE];
//} event_t;
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


//char  current_state[MAX_LEN_STR_EVENT_OR_STATE];
//Class method 'set_initial_current_state'
STATIC mp_obj_t roomba_set_initial_current_state(mp_obj_t self_in, mp_obj_t str_initial_current_state_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(str_initial_current_state_in, str_initial_current_state,  str_initial_current_state_len);

    strcpy(self->current_state, (const char*)str_initial_current_state);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(roomba_set_initial_current_state_obj, roomba_set_initial_current_state);


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



//UTILITY FUNCTION 
//IT USED BY 
//STATIC mp_obj_t roomba_sending_event(mp_obj_t self_in, mp_obj_t str_sending_event_in )
//TO SEND SERIAL COMMAND/EVENT
void send_serial_event(roomba_roomba_obj_t *self, char* event){
   uint8_t data;
   int    errcode=0;
   
   if(!strcmp(event,"START")){
        data=128;
        if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            uart_tx_data(self->serial, &data, 1, &errcode);
        }
   }else if( !strcmp(event,"CONTROL")   ){
        data=130;
        if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            uart_tx_data(self->serial, &data, 1, &errcode);
        }
   
   }else if( !strcmp(event,"SAFE")   ){
        data=131;
        if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            uart_tx_data(self->serial, &data, 1, &errcode);
        }
   
   }else if( !strcmp(event,"FULL")   ){
        data=132;
        if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            uart_tx_data(self->serial, &data, 1, &errcode);
        }
   
   }else if( !strcmp(event,"POWER")   ){
        data=133;
        if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            uart_tx_data(self->serial, &data, 1, &errcode);
        }
   else{
   
   }
}

//typedef struct _event_t {
//      char name[MAX_LEN_STR_EVENT_OR_STATE];
//      char from_state[MAX_LEN_STR_EVENT_OR_STATE];
//      char to_state[MAX_LEN_STR_EVENT_OR_STATE];
//} event_t;
//char  current_state[MAX_LEN_STR_EVENT_OR_STATE];
//    event_t* events[NUMBER_OF_EVENTS];
//Class method 'sending_event'
STATIC mp_obj_t roomba_sending_event(mp_obj_t self_in, mp_obj_t str_sending_event_in ) {
    int ret;
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
                 send_serial_event(self, self->events[i]->name);

                 strcpy(self->current_state, (const char*)self->events[i]->to_state);
                 break;
             } 
         }else{

         }


    }



    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(roomba_sending_event_obj, roomba_sending_event);




// src - a pointer to the data to send (16-bit aligned for 9-bit chars)
// num_chars - number of characters to send (9-bit chars count for 2 bytes from src)
// *errcode - returns 0 for success, MP_Exxx on error
// returns the number of characters sent (valid even if there was an error)
//size_t uart_tx_data(pyb_uart_obj_t *self, const void *src_in, size_t num_chars, int *errcode) {
//bool uart_tx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
//Class method 'send_by_serial'
//STATIC mp_obj_t roomba_send_by_serial(mp_obj_t self_in, char* data, size_t num_chars ) {
STATIC mp_obj_t roomba_send_by_serial(mp_obj_t self_in, mp_obj_t data_array_in, mp_obj_t num_chars_in ) {
    roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //Bisogna convertire 'data_array_in' e 'num_chars_in'
    //
    //
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t item, iterable =mp_getiter(data_array_in, &iter_buf);
    int    num_chars = mp_obj_get_int(num_chars_in);
    int    errcode=0;
    size_t num_chars_sended=0;
    uint8_t data;
    
    while( (item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION   ){
       num_chars=1;
       data = mp_obj_get_int(item);
       if( uart_tx_wait( self->serial, TIMEOUT_TX_MILLISECONDS) ){
            //num_chars_sended=uart_tx_data(self->serial, (const void *) data, num_chars, &errcode);
            //num_chars_sended=uart_tx_data(self->serial, (const void *) item, num_chars, &errcode);
            num_chars_sended=uart_tx_data(self->serial, &data, num_chars, &errcode);
            //mp_printf(MP_PYTHON_PRINTER," %d  ", item); NON VA BENE DEVE STAMPARE BYTE 
       if( errcode != 0){
             if( errcode == MP_ETIMEDOUT){
                 mp_print_str(MP_PYTHON_PRINTER, "\n"); 
                 mp_print_str(MP_PYTHON_PRINTER, "Serial sending error: connection timed out ");
                 mp_print_str(MP_PYTHON_PRINTER, "\n"); 
                 mp_print_str(MP_PYTHON_PRINTER, "Char sended:  ");
                 mp_printf(MP_PYTHON_PRINTER," %d  ", num_chars_sended);
                 mp_print_str(MP_PYTHON_PRINTER, "  \n");
             }else{
                 mp_print_str(MP_PYTHON_PRINTER, "\n"); 
                 mp_print_str(MP_PYTHON_PRINTER, "Generic serial sending error ");
                 mp_print_str(MP_PYTHON_PRINTER, "\n"); 
                 mp_print_str(MP_PYTHON_PRINTER, "Char sended:  ");
                 mp_printf(MP_PYTHON_PRINTER," %d ", num_chars_sended);
                 mp_print_str(MP_PYTHON_PRINTER, "  \n");
            }
      }
      
    }       
    return mp_const_none;
}


MP_DEFINE_CONST_FUN_OBJ_3(roomba_send_by_serial_obj, roomba_send_by_serial);





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
    //roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
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
    //roomba_roomba_obj_t *self = MP_OBJ_TO_PTR(self_in);
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
    //{ MP_ROM_QSTR(MP_QSTR_mysum), MP_ROM_PTR(&roomba_sum_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_state), MP_ROM_PTR(&roomba_add_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_view_states), MP_ROM_PTR(&roomba_view_states_obj) },
    { MP_ROM_QSTR(MP_QSTR_exist_state), MP_ROM_PTR(&roomba_exist_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_event), MP_ROM_PTR(&roomba_add_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_view_events), MP_ROM_PTR(&roomba_view_events_obj) },
    { MP_ROM_QSTR(MP_QSTR_sending_event), MP_ROM_PTR(&roomba_sending_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_by_serial),MP_ROM_PTR(&roomba_send_by_serial_obj) },  
    //{ MP_ROM_QSTR(MP_QSTR_exist_event), MP_ROM_PTR(&roomba_exist_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_initial_current_state), MP_ROM_PTR(&roomba_set_initial_current_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_view_current_state), MP_ROM_PTR(&roomba_view_current_state_obj) },
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
    //roomba_roomba_obj_t *class_instance = MP_OBJ_TO_PTR(o_in);
    //return mp_obj_new_int(class_instance->a + class_instance->b);
    return mp_const_none;
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
 
