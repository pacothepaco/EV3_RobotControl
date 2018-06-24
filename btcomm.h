/***********************************************************************************************************************
 * 
 * 	BT Communications library for Lego EV3 - This provides a bare-bones communication interface between your
 * 	program and the EV3 block. Commands are executed immediately, where appropriate the response from the
 * 	block is processed. 
 * 
 * 	Initial release: Sep. 2018
 * 	By: F.J.Estrada
 * 
 * ********************************************************************************************************************/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This library was developed using for reference the excellent tutorials by Christoph Gaukel at
// http://ev3directcommands.blogspot.ca/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __btcomm_header
#define __btcomm_header

#include "stdio.h"
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include "bytecodes.h"

extern int message_id_counter;		// <-- Global message id counter
#define __BT_debug			// Uncomment to trigger printing of BT debug messages

// Hex identifiers for the 4 motor ports
#define MOTOR_A 0x01
#define MOTOR_B 0x02
#define MOTOR_C 0x04
#define MOTOR_D 0x08

//Hex identifiers of the four input ports
#define PORT_1 0x00
#define PORT_2 0x01
#define PORT_3 0x02
#define PORT_4 0x03
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int BT_open(const char *device_id);
int BT_close();
int BT_setEV3name(const char *name);
int BT_play_tone_sequence(const int tone_data[50][3]);
int BT_motor_port_start(char port_ids, char power);
int BT_motor_port_stop(char port_ids, int brake_mode);
int BT_all_stop(int brake_mode);
int BT_drive(char lport, char rport, char power);
int BT_turn(char lport, char lpower,  char rport, char rpower);
int BT_read_touch_sensor(char sensor_port);
int BT_read_colour_sensor(char sensor_port);
int BT_read_colour_sensor_RGB(char sensor_port, int RGB[3]); 
int BT_read_ultrasonic_sensor(char sensor_port);
int BT_clear_gyro_sensor(char sensor_port);
int BT_read_gyro_sensor(char sensor_port, int angle_speed[2]);
int BT_timed_motor_port_start(char port_id, char power, int ramp_up_time, int run_time, int ramp_down_time);
int BT_timed_motor_port_start_v2(char port_id, char power, int time);

#endif
