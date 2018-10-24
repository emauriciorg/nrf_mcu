#ifndef _FSM_DEFINITION_H_
#define _FSM_DEFINITION_H_


#define sm_declare_states(name, ...)	     \
typedef enum {				     \
	__VA_ARGS__,			     \
	name##_STATE_COUNT,		     \
	name##_STATE_INVALID                 \
} name##_state_e



#define sm_declare_events(name, ...)         \
typedef enum {			             \
	__VA_ARGS__,		             \
	name##_COUNT_EVENT,                  \
	name##_INVALID_EVENT                 \
} name##_event_e


#define sm_declare_handler(name)             \
 struct name##_handler {                     \
  unsigned int   (*handler)(void);                   \
 };


#define sm_declare_look_up_table(name, ... ) \
struct name##_handler                        \
       name##_look_up_table[ name##_STATE_COUNT ][ name##_COUNT_EVENT ]=__VA_ARGS__;


/*macro type : var Declartions */

#define sm_declare_next_event_var(name)\
static name##_event_e name##_new_event;\
static name##_event_e name##_next_event;


#define sm_declare_state_vars(name)\
static name##_state_e name##_new_state;\
static name##_state_e name##_next_state;\


/*
*macro type : run function
**/
#define sm_run(name)\
	unsigned int name##_state_machine_parser();


/*macro type : func Declartions */

#define sm_declare_parser(name)\
void name##_state_machine_parser(void)


#define sm_declare_update_events(name)\
static unsigned int updateEvents(void)



/*macro type : Definition */
#define sm_define_parser(name)\
void name##_state_machine_parser(void){\
	name##_new_event = updateEvents ();\
	if (( name##_next_state  <     name##_COUNT_EVENT )\
		&& (  name##_new_event   <     name##_COUNT_EVENT ) \
		&& (  name##_look_up_table [ name##_next_state ][  name##_new_event ].handler ))\
	      name##_next_state =   (name##_state_e) (*MasterLooKUpTable [name##_next_state ][  name##_new_event ].handler)();\
}

/*
#define sm_set_next_event_var(name,val)						\
	name##_next_event = val
#define sm_get_next_event_var(name,val)						\
	val = name##_next_event
*/
#endif
