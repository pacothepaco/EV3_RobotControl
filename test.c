// Initial testing of a bare-bones BlueTooth communication
// library for the EV3 - Thank you Lego for changing everything
// from the NXT to the EV3!

#include <stdio.h>
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

int message_id_counter=1;		// <-- Global message id counter
#define __BT_debug

// Hex identifiers for the 4 motor ports
#define MOTOR_A 0x01
#define MOTOR_B 0x02
#define MOTOR_C 0x04
#define MOTOR_D 0x08

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This library was developed using for reference the excellent tutorials by Christoph Gaukel at
// http://ev3directcommands.blogspot.ca/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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


int BT_open(const char *device_id)
{
 // Open a connection to the device identified by the
 // provided hex ID string.
 //
 // Returns an int identifier for an open socket to the
 // device
 //
 // Derived from bluetooth.c by Don Neumann
  
 int rv;
 struct sockaddr_rc addr = { 0 };
 int s, status, socket_id;
 char dest[18];
   
 fprintf(stderr,"Request to connect to device %s\n",device_id);
 
 socket_id = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
 // set the connection parameters (who to connect to)
 addr.rc_family = AF_BLUETOOTH;
 addr.rc_channel = (uint8_t) 1;
 str2ba(device_id, &addr.rc_bdaddr );

 status = connect(socket_id, (struct sockaddr *)&addr, sizeof(addr));
 if( status == 0 ) {
	printf("Connection to %s established.\n", device_id);
 }
 if( status < 0 ) {
       perror("Connection attempt failed ");
       return(-1);
 }
 return(socket_id);
}

int BT_close(int socket_id)
{
 // Close the communication socket to the EV3, input is the
 // socket identifier obtained upon opening the connection.
 fprintf(stderr,"Request to close connection to device at socket id %d\n",socket_id);
 close(socket_id);
}

int BT_setEV3name(const char *name, int socket_id)
{
 // This function can be used to name your EV3. The input name should be a proper
 // zero-terminated string
 char cmd_string[1024];
 char cmd_prefix[11]={0x00,0x00,    0x00,0x00,    0x00,    0x00,0x00,    0xD4,     0x08,   0x84,              0x00};
 //                   |length-2|    | cnt_id |    |type|   | header |    |ComSet|  |Op|    |String prefix|   
 char reply[1024];
 int len;
 void *lp;
 unsigned char *cp;

 memset(&cmd_string[0],0,1024);
 // Check input string fits within our buffer, then pre-format the command sequence 
 len=strlen(name);
 if (len>24)
 {
  fprintf(stderr,"BT_setEV3name(): The input name string is too long - 24 characters max, no white spaces or special characters\n");
  return(-1);
 }
 memcpy(&cmd_string[0],&cmd_prefix[0],10*sizeof(unsigned char));
 strncpy(&cmd_string[10],name,1013);
 cmd_string[1024]=0x00;

 // Update message length, and update sequence counter (length and cnt_id fields) 
 len+=9;
 lp=(void *)&len;
 cp=(unsigned char *)lp;		// <- magic!
 cmd_string[0]=*cp;
 cmd_string[1]=*(cp+1);

 lp=(void *)&message_id_counter;
 cp=(unsigned char *)lp;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 
#ifdef __BT_debug 
 fprintf(stderr,"Set name command:\n");
 for(int i=0; i<len+2; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif  

 write(socket_id,&cmd_string[0],len+2);
 read(socket_id,&reply[0],1023);

#ifdef __BT_debug
 fprintf(stderr,"Set name reply:\n");
 for (int i=0; i<5; i++)
   fprintf(stderr,"%x ",reply[i]);
 fprintf(stderr,"\n");
#endif

 if (reply[4]==0x02)
  fprintf(stderr,"BT_setEV3name(): Command successful\n");
 else
  fprintf(stderr,"BT_setEV3name(): Command failed, name must not contain spaces or special characters\n");
 
 message_id_counter++;
}

int BT_play_tone_sequence(const int tone_data[50][3], int socket_id)
{
 // This function takes in an array specifying a sequence of tones to
 // be played by the EV3 - the array has a fixed size, up to 50 notes
 // can be sent at a time. For each tone, tone_data[i][0] contains the frequency
 // in 20 - 20,000 Hz, and tone_data[i][1] is the duration in milliseconds
 // in 1 - 5000. 
 // A value of -1 for either entry in tone_data signifies the end of the
 // tones to be played.
 // Finally, tone_data[i][2] is the volume for the tone in [0, 63]
 //
 // This function does not check the array is properly sized/formatted,
 // so be sure to provide sound input.
 //
 // On success, returns 1
 // On error, returns -1
  
 int len;
 int freq;
 int dur;
 int vol;
 void *p;
 char *cmd_str_p;
 unsigned char *cp;
 char cmd_string[1024];
 char cmd_prefix[8]={0x00,0x00,    0x00,0x00,    0x80,    0x00,0x00,    0x00};
 //                  |length-2|    | cnt_id |    |type|   | header |    

 memset(&cmd_string[0],0,1024);
 strcpy(&cmd_string[0],&cmd_prefix[0]);
 len=5;
 
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 
 // Pre-check tone information
 for (int i=0; i<50; i++)
 {
  if (tone_data[i][0]==-1||tone_data[i][1]==-1) break;
  if (tone_data[i][0]<20||tone_data[i][0]>20000) {fprintf(stderr,"BT_play_tone_sequence():Tone range must be in 20Hz-20KHz\n");return(-1);}
  if (tone_data[i][1]<1||tone_data[i][1]>5000) {fprintf(stderr,"BT_play_tone_sequence():Tone duration must be in 1-5000ms\n");return(-1);}
  if (tone_data[i][2]<0||tone_data[i][2]>63) {fprintf(stderr,"BT_play_tone_sequence():Volume must be in 0-63\n");return(-1);}
 }

 cmd_str_p=&cmd_string[7];
 for (int i=0; i<50; i++)
 {
  if (tone_data[i][0]==-1||tone_data[i][1]==-1) break;
  freq=tone_data[i][0];
  dur=tone_data[i][1];
  vol=tone_data[i][2];
  *(cmd_str_p++)=0x94;				// <--- Sound output command
  *(cmd_str_p++)=0x01;				// <--- Output tone mode
  *(cmd_str_p++)=(unsigned char)vol;		// <--- Set volume for this note
  *(cmd_str_p++)=0x82;				// <--- 3-bytes specifying tone frequency (see format notes above)
  p=(void *)&freq;
  cp=(unsigned char *)p;
  *(cmd_str_p++)=*cp;
  *(cmd_str_p++)=*(cp+1);
  *(cmd_str_p++)=0x82;				// <--- 3-bytes specifying tone duration (see format notes above)
  p=(void *)&dur;
  cp=(unsigned char *)p;
  *(cmd_str_p++)=*cp;
  *(cmd_str_p++)=*(cp+1);
  *(cmd_str_p++)=0x96;				// <--- Command to request wait for this tone to end before next is played
  len+=10;    
 }
 
 // Update message length field
 p=(void *)&len;
 cp=(unsigned char *)p;
 cmd_string[0]=*cp;
 cmd_string[1]=*(cp+1);

#ifdef __BT_debug 
 fprintf(stderr,"Tone output command string:\n");
 for(int i=0; i<len+2; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif  

 write(socket_id,&cmd_string[0],len+2);

 message_id_counter++; 
 return(1);
}

int BT_motor_port_start(char port_ids, char power, int socket_id)
{
 // This is the function that controls the output power to the motor
 // ports. Each motor port is identified by a hex value and multiple
 // ports can be set at once. For example
 //
 // BT_motor_port_power(MOTOR_A, 100);   <-- start motor at port A, 100% power
 // BT_motor_port_power(MOTOR_A|MOTOR_C, 50);  <-- start motors at port A and port C, 50% power
 //
 // Power must be in [-100, 100]
 // 
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 //  to fully stop the motors you need to use the appropriate BT command.
  
 void *p;
 unsigned char *cp;
 char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                  |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_motor_port_start: Power must be in [-100, 100]\n");
  return(-1);
 }
 
 if (port_ids>15)
 {
  fprintf(stderr,"BT_motor_port_start: Invalid port id value\n");
  return(-1);
 }
 
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[9]=port_ids;
 cmd_string[11]=power;
 cmd_string[14]=port_ids;

#ifdef __BT_debug 
 fprintf(stderr,"BT_motor_port_start command string:\n");
 for(int i=0; i<15; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif  
 
 write(socket_id,&cmd_string[0],15);

 message_id_counter++; 
 return(1); 
}

int BT_motor_port_stop(char port_ids, int brake_mode, int socket_id)
{
 // Stops the motors at the specified port ids. Note this does
 // not change output power settings!
 //
 // brake_mode is either 0 or 1.
 //   0 -> allow motor to come to a stop by itself
 //   1 -> active brake, use power to stop motor

 void *p;
 unsigned char *cp;
 char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
 //                  |length-2| | cnt_id | |type| | header |  |stop|   |layer|  |port ids|  |brake|
 
 if (port_ids>15)
 {
  fprintf(stderr,"BT_motor_port_stop: Invalid port id value\n");
  return(-1);
 }
 if (brake_mode!=0&&brake_mode!=1)
 {
  fprintf(stderr,"BT_motor_port_start: brake mode must be either 0 or 1\n");
  return(-1);
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
  
 cmd_string[9]=port_ids;
 cmd_string[10]=brake_mode;
 
#ifdef __BT_debug 
 fprintf(stderr,"BT_motor_port_stop command string:\n");
 for(int i=0; i<11; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif  
 
 write(socket_id,&cmd_string[0],11);

 message_id_counter++; 
 return(1); 
}

int main(int argc, char *argv[])
{
 int EV3_socket;
 char test_msg[8]={0x06,0x00,0x2A,0x00,0x00,0x00,0x00,0x01};
 char reply[1024];
 int tone_data[50][3];
 
 // Reset tone data information
 for (int i=0;i<50; i++) 
 {
   tone_data[i][0]=-1;
   tone_data[i][1]=-1;
   tone_data[i][2]=-1;
 }
 
 tone_data[0][0]=262;
 tone_data[0][1]=250;
 tone_data[0][2]=1;
 tone_data[1][0]=330;
 tone_data[1][1]=250;
 tone_data[1][2]=25;
 tone_data[2][0]=392;
 tone_data[2][1]=250;
 tone_data[2][2]=50;
 tone_data[3][0]=523;
 tone_data[3][1]=250;
 tone_data[3][2]=63;
 
 memset(&reply[0],0,1024);
 
 //just uncomment your bot's hex key to compile for your bot, and comment the other ones out.
 #ifndef HEXKEY
 	#define HEXKEY "00:16:53:3F:71:F0"	// <--- SET UP YOUR NXT's HEX ID here
 #endif

 EV3_socket=BT_open(HEXKEY);
 fprintf(stderr,"EV3 socket identifier: %d\n",EV3_socket);

 fprintf(stderr,"Sending message:\n");
 for (int i=0; i<8; i++)
   fprintf(stderr,"%x ",test_msg[i]);
 fprintf(stderr,"\n");

 write(EV3_socket,&test_msg[0],8);

 read(EV3_socket,&reply[0],5);
 fprintf(stderr,"Reply message:\n");
 for (int i=0; i<5; i++)
   fprintf(stderr,"%x ",reply[i]);
 fprintf(stderr,"\n");

 BT_setEV3name("Paco-Bot-Net",EV3_socket);

 BT_play_tone_sequence(tone_data,EV3_socket);

 BT_motor_port_start(MOTOR_D,10,EV3_socket);
 BT_motor_port_start(MOTOR_C,-10,EV3_socket);
 gets(&reply[0]);
 BT_motor_port_stop(MOTOR_C|MOTOR_D,1,EV3_socket);
 
 BT_close(EV3_socket);
 fprintf(stderr,"Done!\n"); 
}
