#include "../inc/fsm_slave.h"

#include <stdio.h>

unsigned int slave_h1(void);
unsigned int slave_h2(void);
unsigned int slave_h3(void);
unsigned int slave_h4(void);


sm_declare_update_events(slave);

static unsigned int updateEvents(void);


sm_declare_states (slave , msIDLE      ,         
		           led_RED     ,      
		           led_BLUE    ,
		           led_GREEN   ,    
		           Error)      ;

sm_declare_events  (slave, btn_1       ,
		 	   btn_2       ,
		 	   btn_12)     ;

sm_declare_handler (slave)             ;



sm_declare_look_up_table(slave, {  //evt1       evt2          evt3
			        { slave_h1 ,  slave_h2  ,  slave_h3 },  //state1
				{ slave_h3 ,  slave_h1  ,  slave_h2 },  //state2
				{ slave_h3 ,  slave_h2  ,  slave_h1 },  //state3
				{ slave_h1 ,  slave_h2  ,  slave_h3 },  //state4
				{0,0,0}}	
			);


sm_declare_update_events(slave){
}				

sm_declare_parser(slave){
}


/****state machine custom functions*****/

unsigned int slave_h1(void){
	printf("state h1\n");
	return led_RED;
}
unsigned int slave_h2(void)
{	printf("state h2\n");
 	return led_BLUE;
}

unsigned int slave_h3(void){	
	printf("state h3\n");
	return led_GREEN;
}

unsigned int slave_h4(void){	
	printf("state h4\n");
	return msIDLE;
}


