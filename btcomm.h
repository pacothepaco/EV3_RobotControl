/***********************************************************************************************************************
 * 
 * 	BT Communications library for Lego EV3 - This provides a bare-bones communication interface between your
 * 	program and the EV3 block. Commands are executed immediately, where appropriate the response from the
 * 	block is processed. 
 * 
 * 	Have a look at the headers below, as they provide a quick glimpse into what functions are provided. You
 * 	have API calls for controlling motor ports, as well as for polling attached sensors. Look at the 
 * 	corresponding .c code file to understand how the API works in case you need to create new API calls
 * 	not provided here.
 * 	
 * 	You *CAN* modify this file and expand the API, but please be sure to clearly mark what you have changed
 * 	so it an be evaluated by your TA and instructor.
 * 
 *      This library is free software, distributed under the GPL license. Please refer to the include license
 *      file for details.
 * 
 * 	Initial release: Sep. 2018
 * 	By: L. Tishkina
 *          F. Estrada
 * ********************************************************************************************************************/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This library was developed using for reference the excellent tutorials by Christoph Gaukel at
// http://ev3directcommands.blogspot.ca/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __btcomm_header
#define __btcomm_header

// Standard UNIX libraries, should already be in your Linux box
#include "stdio.h"
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>


// Bluetooth libraries - make sure they are installed in your machine
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "bytecodes.h"			// <-- This is provided by Lego, from the EV3 development kit,
#include "c_com.h"  			//     and is distributed under GPL. Please see the license
					           //     file included with this distribution for details.

extern int message_id_counter;		// <-- Global message id counter

// Hex identifiers for the 4 motor ports (defined by Lego)
#define MOTOR_A 0x01
#define MOTOR_B 0x02
#define MOTOR_C 0x04
#define MOTOR_D 0x08

//Hex identifiers of the four input ports (Also defined by Lego)
#define PORT_1 0x00
#define PORT_2 0x01
#define PORT_3 0x02
#define PORT_4 0x03

#define EV3_COLOUR 29
#define EV3_INFRARED 33
#define EV3_GYRO 32
#define PARTITION_SIZE 1017

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Command string encoding://   Prefix format:  |0x00:0x00|   |0x00:0x00|   |0x00|   |0x00:0x00|   |.... payload ....|
//                   				|length-2|    | cnt_id |    |type|   | header |    
//
// The length field is the total length of the command string *not including* the length field (so, string length - 2)
// the next two bytes are a message id counter, used to keep track of replies to messages requiring a reply from the EV3
// then follows a 1-byte type field: 0x00 -> Direct command with reply
//                                   0x80 -> Direct command with no reply
// The header specifies memory sizes - see the appropriate command sequences below
// All multi-byte values (header, counter id, etc.) must be stored in little endian order, that is, from left to right
// in the command string we store the lowest to highest order bytes.
//
// Command strings are limited to 1024 bytes (apparently). So all command strings here are 1024 in length.

// Notes on data encoding
// (from: http://ev3directcommands.blogspot.ca/2016/01/ev3-direct-commands-lesson-01-pre.html
//
// Sending data to the EV3 block:
// 
// The first byte is always a type identifier
// 0x81 - 1 byte signed integer (followed by the 1 byte value)
// 0x82 - 2 byte signed integer (followed by the 2 byte value in reverse order - little endian!)
// 0x83 - 4 byte signed integer (followed by the 4 byte value in reverse order)
// 
// Stuffing the bytes in the right order into the message byte string is crucial. If wrong
// values are being received at the EV3, check the ordering of bytes in the message payload!
//
// To help with the right placement of bits into variables (since all variables have to be converted to little endian) 
// there are helper methods defined in bytecodes.h file. 
// There are three types of variables - constants (LC0, LC1, etc), local variables (LV0, LV1, etc) and global variables (GV0, GV1, etc). 
// The first two letters specify the type of variable and the number specifies the size. For example, for a local variable that takes up 1 byt// e,  you would use LV0. 
// For variables that are longer than 1 byte, you need to call the right method to specify the type of variable and its size LC2_byte0,
// for example. And then there are methods such as LX_byte1 that will do the byte shifting for each subsequent byte. The byte shifting methods
// starting with LX can be used for any type of variable.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Set up a socket to communicate with your Lego EV3 kit
int BT_open(const char *device_id);

// Close open socket to your EV3 ending the communication with the bot
int BT_close();

// Change your Bot's name - the length should be up to 12 characters
int BT_setEV3name(const char *name);

// Play a sequence of musical notes of specified frequencies, durations, and volume
int BT_play_tone_sequence(const int tone_data[50][3]);

// Motor control section
int BT_motor_port_start(char port_ids, char power);			// General motor port control
int BT_motor_port_stop(char port_ids, int brake_mode);			// General motor port stop
int BT_all_stop(int brake_mode);					// Quick call to stop everything
int BT_drive(char lport, char rport, char power);			// Constant speed drive (equal speed both ports)
int BT_turn(char lport, char lpower,  char rport, char rpower);		// Individual control for two wheels for turning

// Timed functions will allow you to build carefully programmed motions. The motor is set to the specified power
// for the specified time, and then stopped. The more general version allows for smooth speed control by providing you
// with a delay between full stop and full speed (ramp up time), and from full speed back to full stop (ramp down).
int BT_timed_motor_port_start(char port_id, char power, int ramp_up_time, int run_time, int ramp_down_time);
int BT_timed_motor_port_start_v2(char port_id, char power, int time);

// Sensor operation section
// If no sensor is plugged into the sensor_port the readings will be 0 for that sensor. If the wrong sensor is
// plugged into the port then there will be values returned, but they will not correspond to the actual state of 
// the sensor.
void BT_sensor_get_format(char sensor_port);
void BT_sensor_set_type_mode(char sensor_port, char type, char mode);
void BT_get_type_mode(char sensor_port);
int BT_is_busy(char sensor_port);
int BT_read_touch_sensor(char sensor_port);				
int BT_read_colour_sensor(char sensor_port);				// Indexed colour read function
int BT_read_colour_sensor_RGB(char sensor_port, int RGB[3]); 		// Returns RGB instead of indexed colour
int BT_read_ultrasonic_sensor(char sensor_port);
int BT_clear_sensor(char sensor_port);				// Reset sensor to 0
int BT_read_gyro_sensor(char sensor_port);
void BT_get_type_mode(char sensor_port);
void BT_sensor_set_mode(char sensor_port, char mode);
int BT_check_if_busy(char sensor_port);
int BT_play_sound_file(const char *path, int volume);

// System command section
// Used for uploading files to the EV3 such as image and sound files in proper format. EV3 accepts .rgf image files and
// .rsf sound files.
int BT_list_files(char *path, char **contents);
int BT_upload_file(const char *path_dest, const char *path_src);

// UI commands section
// Used to interact with the display and LED lights around the buttons.
int BT_set_LED_colour(int colour);
int BT_draw_image_from_file(int colour, int x_0, int y_0, const char *file_path);
int BT_restore_previous_display(int no);
int BT_store_current_display(int no);
#endif
