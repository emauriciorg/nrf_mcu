/** 
******************************************************************************
* \file    command_list.h
* \brief   command list for CLI.
******************************************************************************
*/
#ifndef COMMAND_LIST
#define COMMAND_LIST

/*turn on leds commands */
#define cmd_turn   0x7b

#define cmd_sub_off   0x78
#define cmd_sub_on    0xA4
#define cmd_sub_red   0x73
#define cmd_sub_blue  0x43
#define cmd_sub_green 0x55

/*gpio commands : "gpio lo pinnumber", "gpio lo 8"*/
#define cmd_gpio     0x4d 
#define cmd_gpio_lo  0xA3
#define cmd_gpio_hi  0x93

#define cmd_sample   0xd4

#define cmd_send     0x26


#define cmd_cip     0x285
#define cmd_dcip     0x34

//random key generation 
#define cmd_sub_random_key  0x89


#define cmd_ble	     0x269

#define cmd_rgb      0x266

#define cmd_help     0x45   
#define cmd_clear    0x237

/*TWI DEBUG*/
#define cmd_write_twi 0x88
#define cmd_read_twi  0x74
#define cmd_twi_read_saved_buffer 0x64
#define cmd_twiapp	0x10f

#define cmd_accinit  0x3da
#define cmd_cmdtest  0x3d
#define cmd_hex2dec  0x287
#endif
