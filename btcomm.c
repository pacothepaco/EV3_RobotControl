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
#include "btcomm.h"
int message_id_counter=1;
int *socket_id;

int BT_open(const char *device_id)
{
 //////////////////////////////////////////////////////////////////////////////////////////////////////
 // Open a socket to the specified Lego EV3 device specified by the provided hex ID string
 //
 // Input: The hex string identifier for the Lego EV3 block
 // Returns: 0 on success
 //          -1 otherwise 
 //
 // Derived from bluetooth.c by Don Neumann
 //////////////////////////////////////////////////////////////////////////////////////////////////////
  
 int rv;
 struct sockaddr_rc addr = { 0 };
 int s, status;
 char dest[18];
 socket_id=(int*)malloc(sizeof(int));   
 fprintf(stderr,"Request to connect to device %s\n",device_id);
 
 *socket_id = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
 // set the connection parameters (who to connect to)
 addr.rc_family = AF_BLUETOOTH;
 addr.rc_channel = (uint8_t) 1;
 str2ba(device_id, &addr.rc_bdaddr );

 status = connect(*socket_id, (struct sockaddr *)&addr, sizeof(addr));
 if( status == 0 ) {
	printf("Connection to %s established at socket: %d.\n", device_id, *socket_id);
 }
 if( status < 0 ) {
       perror("Connection attempt failed ");
       return(-1);
 }
 return 0;
}

int BT_close()
{
 /////////////////////////////////////////////////////////////////////////////////////////////////////
 // Close the communication socket to the EV3
 /////////////////////////////////////////////////////////////////////////////////////////////////////  
 fprintf(stderr,"Request to close connection to device at socket id %d\n",*socket_id);
 close(*socket_id);
 free(socket_id);
}

int BT_setEV3name(const char *name)
{
 /////////////////////////////////////////////////////////////////////////////////////////////////////
 // This function can be used to name your EV3. 
 // Inputs: A zero-terminated string containing the desired name, length <=  12 characters
 /////////////////////////////////////////////////////////////////////////////////////////////////////

 char cmd_string[1024];
 unsigned char cmd_prefix[11]={0x00,0x00,    0x00,0x00,    0x00,    0x00,0x00,    0xD4,     0x08,   0x84,              0x00};
 //                   |length-2|    | cnt_id |    |type|   | header |    |ComSet|  |Op|    |String prefix|   
 char reply[1024];
 int len;
 void *lp;
 unsigned char *cp;

 memset(&cmd_string[0],0,1024);
 // Check input string fits within our buffer, then pre-format the command sequence 
 len=strlen(name);
 if (len>12)
 {
  fprintf(stderr,"BT_setEV3name(): The input name string is too long - 12 characters max, no white spaces or special characters\n");
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

 write(*socket_id,&cmd_string[0],len+2);
 read(*socket_id,&reply[0],1023);

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

int BT_play_tone_sequence(const int tone_data[50][3])
{
 //////////////////////////////////////////////////////////////////////////////////////////////////
 // 
 // This function sends to the EV3 a list of notes, volumes, and durations for tones to
 // be played out by the block.
 //
 // Inputs: An array of size [50][3] with up to 50 notes
 //            for each entry: tone_data[i][0] contains a frequency in [20,20000]
 //                            tone_data[i][1] contains a duration in milliseconds [1,5000]
 //                            tone_data[i][2] contains the volume in [0,63]
 //            A value of -1 for any of the entries in tone_data[][] signals the end of the
 //             sequence.
 //
 // This function does not check the array is properly sized/formatted, so be sure to provide 
 //  properly formatted input.
 //
 // Returns:  1 on success
 //           0 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
  
 int len;
 int freq;
 int dur;
 int vol;
 void *p;
 unsigned char *cmd_str_p;
 unsigned char *cp;
 unsigned char cmd_string[1024];
 unsigned char cmd_prefix[8]={0x00,0x00,    0x00,0x00,    0x80,    0x00,0x00,    0x00};
 //                  |length-2|    | cnt_id |    |type|   | header |    

 memset(&cmd_string[0],0,1024);
 strcpy((char *)&cmd_string[0],(char *)&cmd_prefix[0]);
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
  if (tone_data[i][0]<20||tone_data[i][0]>20000) {fprintf(stderr,"BT_play_tone_sequence():Tone range must be in 20Hz-20KHz\n");return(0);}
  if (tone_data[i][1]<1||tone_data[i][1]>5000) {fprintf(stderr,"BT_play_tone_sequence():Tone duration must be in 1-5000ms\n");return(0);}
  if (tone_data[i][2]<0||tone_data[i][2]>63) {fprintf(stderr,"BT_play_tone_sequence():Volume must be in 0-63\n");return(0);}
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

 write(*socket_id,&cmd_string[0],len+2);

 message_id_counter++; 
 return(1);
}

int BT_motor_port_start(char port_ids, char power)
{
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This function sends a command to the specified motor ports to set the motor power to
 // the desired value.
 //
 // Motor ports are identified by hex values (defined at the top), but here we will use
 //  their associated names MOTOR_A through MOTOR_D. Multiple motors can be started with
 //  a single command by ORing their respective hex values, e.g.
 //
 // BT_motor_port_power(MOTOR_A, 100);   	<-- set motor at port A to 100% power
 // BT_motor_port_power(MOTOR_A|MOTOR_C, 50);   <-- set motors at port A and port C to 50% power
 //
 // Power must be in [-100, 100] - Forward and reverse
 // 
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 //  to fully stop the motors you need to use the appropriate BT command.
 //
 // Inputs: The port identifiers
 //         Desired power value in [-100,100]
 //
 // Returns: 1 on success
 //          0 otherwise  
 //////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 unsigned char *cp;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                  |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_motor_port_start: Power must be in [-100, 100]\n");
  return(0);
 }
 
 if (port_ids>15)
 {
  fprintf(stderr,"BT_motor_port_start: Invalid port id value\n");
  return(0);
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
 
 write(*socket_id,&cmd_string[0],15);

 message_id_counter++; 
 return(1); 
}

int BT_motor_port_stop(char port_ids, int brake_mode)
{
 //////////////////////////////////////////////////////////////////////////////////
 //
 // Stop the motor(s) at the specified ports. This does not change the output
 // power settings!
 //
 // Inputs: Port ids of the motors that should be stopped
 // 	    brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 // Returns: 1 on success
 //          0 otherwise
 //////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 unsigned char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
 //                  |length-2| | cnt_id | |type| | header |  |stop|   |layer|  |port ids|  |brake|
 
 if (port_ids>15)
 {
  fprintf(stderr,"BT_motor_port_stop: Invalid port id value\n");
  return(0);
 }
 if (brake_mode!=0&&brake_mode!=1)
 {
  fprintf(stderr,"BT_motor_port_start: brake mode must be either 0 or 1\n");
  return(0);
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
 
 write(*socket_id,&cmd_string[0],11);

 message_id_counter++; 
 return(1); 
}


int BT_all_stop(int brake_mode){
 //////////////////////////////////////////////////////////////////////////////////////////////////////
 // Applies breaks to all motors.
 //
 // Inputs: brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 // Returns: 1 on success
 //          0 otherwise
 //
 //////////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 unsigned char *cp;
 char port_ids = MOTOR_A|MOTOR_B|MOTOR_C|MOTOR_D;
 unsigned char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
 //                  |length-2| | cnt_id | |type| | header |  |stop|   |layer|  |port ids|  |brake|

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
  
 cmd_string[9]=port_ids;
 cmd_string[10]=brake_mode;

#ifdef __BT_debug
 fprintf(stderr,"BT_all_stop command string:\n");
 for(int i=0; i<11; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],11);

 message_id_counter++;
 return(1);

}

int BT_drive(char lport, char rport, char power){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This function sends a command to the left and right motor ports to set the motor power to
 // the desired value.
 //
 // Ports are identified as MOTOR_A, MOTOR_B, etc
 // Power must be in [-100, 100]
 //
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 // to fully stop the motors you need to use the appropriate BT command.
 //
 // Inputs: port identifier of left port
 //         port identifier of right port
 //         power for ports in [-100, 100]
 //
 // Returns: 1 on success
 //          0 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 
 void *p;
 unsigned char *cp;
 char ports;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                           |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_drive: Power must be in [-100, 100]\n");
  return(0);
 }

 if (lport>8 || rport>8)
 {
  fprintf(stderr,"BT_drive: Invalid port id value\n");
  return(0);
 }
 ports = lport|rport;

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[9]=ports;
 cmd_string[11]=power;
 cmd_string[14]=ports;


#ifdef __BT_debug 
 fprintf(stderr,"BT_drive command string:\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif  

 write(*socket_id,&cmd_string[0],15);

 message_id_counter++; 
 return(1);

}

int BT_turn(char lport, char lpower, char rport, char rpower){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This function sends a command to the left and right motor ports to set the motor power to
 // the desired value.
 //
 // Ports are identified as MOTOR_A, MOTOR_B, etc
 // Power must be in [-100, 100]
 //
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 // to fully stop the motors you need to use the appropriate BT command.
 //
 // Inputs: port identifier of left port
 //         power for left port in [-100, 100]
 //         port identifier of right port
 //         power for right port in [-100, 100]
 //
 // Returns: 1 on success
 //          0 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 unsigned char cmd_string[20]={0x12,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,      0x81,0x00,    0xA4,     0x00,     0x00, 0x81,0x00,  0xA6,    0x00,   0x00};
 //                          |length-2| | cnt_id | |type| | header |  |set power| |layer|  |lport id|  |power|  |set power| |layer| |rport id| |power|     |start|  |layer| |port ids|

 if (lpower>100||lpower<-100||rpower>100||lpower<-100)
 {
  fprintf(stderr,"BT_drive: Power must be in [-100, 100]\n");
  return(0);
 }

 if (lport>8 || rport>8)
 {
  fprintf(stderr,"BT_drive: Invalid port id value\n");
  return(0);
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 //set up power and port for left motor
 cmd_string[9]=lport;
 cmd_string[11]=lpower;

 //set up power and port for right motor
 cmd_string[14] = rport;
 cmd_string[16] = rpower;

 cmd_string[19] = lport|rport;

#ifdef __BT_debug
 fprintf(stderr,"BT_turn command string:\n");
 for(int i=0; i<20; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],20);

 message_id_counter++;

 return(1);

}

int BT_read_touch_sensor(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the value from the touch sensor.
 // 
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 //
 // Inputs: port identifier of touch sensor port
 //
 // Returns: 1 if touch sensor is pushed 
 //          0 if touch sensor is not pushed
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 char reply[1024];
 unsigned char *cp;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00 };
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|

 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_touch_sensor: Invalid port id value\n");
  return(0);
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=LC0(READY_PCT);
 cmd_string[10]=sensor_port;
 cmd_string[11]=LC0(0x10); //type
 cmd_string[13]=LC0(0x01); //data set
 cmd_string[14]=GV0(0x00); //global var

#ifdef __BT_debug
 fprintf(stderr,"BT_read_touch_sensor command string:\n");
 for(int i=0; i<15; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],15);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_touch_sensor(): Command successful\n");
  return(reply[5]!=0);
 }
 else{
  fprintf(stderr,"BT_touch_sensor(): Command failed\n");
  return(-1);
 }
}

int BT_read_colour_sensor(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the value from the colour sensor.
 //
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 //
 // Inputs: port identifier of colour sensor port
 //
 // Returns: A colour value as an int (see table below).
 //
 // Value Colour
 //  0    No colour read
 //  1    Black
 //  2    Blue
 //  3    Green
 //  4    Yellow
 //  5    Red
 //  6    White
 //  7    Brown
 //          
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 char reply[1024];
 unsigned char *cp;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00 };
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|
 //unsigned char cmd_string[12]={0x0A,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00};


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_colour_sensor: Invalid port id value\n");
  return(0);
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=LC0(READY_RAW);
 cmd_string[10]=sensor_port;
 cmd_string[11]=LC0(29); //type
 cmd_string[12]=LC0(0x02); //mode
 cmd_string[13]=LC0(0x01); //data set
 cmd_string[14]=GV0(0x00); //global var

#ifdef __BT_debug
 fprintf(stderr,"BT_read_colour_sensor command string:\n");
 for(int i=0; i<15; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],15);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_colour_sensor(): Command successful\n");
 }
 else{
  fprintf(stderr,"BT_colour_sensor(): Command failed\n");
 }
 return reply[5];
}

int BT_read_colour_sensor_RGB(char sensor_port, int RGB[3]){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the value from the colour sensor and fills in the passed in RGB array with RGB values,
 // where R[0, 255], G[0, 255] and B[0, 255].
 //
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 //
 // Inputs: port identifier of colour sensor port
 //
 // Returns:
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char reply[1024];
 unsigned char *cp;
 uint32_t R=0, G=0, B=0;

 unsigned char cmd_string[17]={0x00,0x00, 0x00,0x00, 0x00,  0x0C,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00, 0x00, 0x00 };
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_colour_sensor_RGB: Invalid port id value\n");
  return(0);
 }

 cmd_string[0]=LC0(15);
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=LC0(READY_RAW);
 cmd_string[10]=sensor_port;
 cmd_string[11]=LC0(29); //type
 cmd_string[12]=LC0(0x04); //mode
 cmd_string[13]=LC0(3); //data set
 cmd_string[14]=GV0(0x00); //global var
 cmd_string[15]=GV0(0x04);
 cmd_string[16]=GV0(0x08);

#ifdef __BT_debug
 fprintf(stderr,"BT_read_colour_sensor_RGB command string:\n");
 for(int i=0; i<17; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],17);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_colour_sensor_RGB(): Command successful\n");
  R|=(uint32_t)reply[5];
  R|=(uint32_t)reply[6]<<8;
  G|=(uint32_t)reply[9];
  G|=(uint32_t)reply[10]<<8;
  B|=(uint32_t)reply[13];
  B|=(uint32_t)reply[14]<<8;
  float normalized=(float)R/1020*255;
  RGB[0]=(int)normalized;
  normalized=(float)G/1020*255;
  RGB[1]=(int)normalized;
  normalized=(float)B/1020*255;
  RGB[2]=(int)normalized;
 }
 else{
  fprintf(stderr,"BT_colour_sensor_RGB(): Command failed\n");
  return(-1);
 }
 return (0);
}

int BT_read_ultrasonic_sensor(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the value from ultrasonic sensor and returns distance in cm to object in front of sensor. 
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 // Inputs: port identifier of ultrasonic sensor port
 //
 // Returns: distance in cm
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char reply[1024];
 unsigned char *cp;

 unsigned char cmd_string[15]={0x00,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_ultrasonic_sensor: Invalid port id value\n");
  return(0);
 }

 cmd_string[0]=LC0(13);
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=LC0(READY_RAW);
 cmd_string[10]=sensor_port;
 cmd_string[11]=LC0(30); //type
 cmd_string[13]=LC0(0x01); //data set
 cmd_string[14]=GV0(0x00); //global var

#ifdef __BT_debug
 fprintf(stderr,"BT_read_ultrasonic_sensor command string\n");
 for(int i=0; i<15; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],15);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;
if (reply[4]==0x02){
  fprintf(stderr,"BT_ultrasonic_sensor(): Command successful\n");
}
 else{
  fprintf(stderr,"BT_ultrasonic_sensor: Command failed\n");
  return(-1);
 }
 return (reply[5]);
}
