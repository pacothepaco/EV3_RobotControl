/***********************************************************************************************************************
 * 
 * 	BT Communications library for Lego EV3 - This provides a bare-bones communication interface between your
 * 	program and the EV3 block. Commands are executed immediately, where appropriate the response from the
 * 	block is processed. 
 *
 *      Please read the corresponding .h file for a quick overview of what functions are provided. You *CAN* modify
 * 	the code here, and expand the API as needed for your work, but be sure to clearly indicate what code was
 * 	added so your TA and instructor can appropriately evaluate your work.
 *
 *      This library is free software, distributed under the GPL license. Please see the attached license file for
 *      details.
 * 
 * 	Initial release: Sep. 2018
 * 	By: L. Tishkina
 * 	    F. Estrada
 * 
 * ********************************************************************************************************************/
#include "btcomm.h"
					     
//#define __BT_debug			// Uncomment to trigger printing of BT messages for debug purposes

// These are required to keep track of messages and help set state
int message_id_counter=1;		// <-- This is a global message_id counter, used to keep track of
                                //     messages sent to the EV3
int *socket_id;				    // <-- Socked identifier for your EV3
int ref_angle=0;                // <-- Reference angle, once set it makes the current measurement from gyro equal to 0 degrees

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
 memset(&reply[0],0,1024);
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
 // Returns:  0 on success
 //           -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
  
 int len;
 int freq;
 int dur;
 int vol;
 void *p;
 char reply[1024];
 unsigned char *cmd_str_p;
 unsigned char *cp;
 unsigned char cmd_string[1024];
 unsigned char cmd_prefix[8]={0x00,0x00,    0x00,0x00,    0x80,    0x00,0x00,    0x00};
 //                           |length-2|    | cnt_id |    |type|   | header |    

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

 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 return(0);
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
 // Returins: 0 on success
 //          -1 otherwise  
 //////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 unsigned char *cp;
 char reply[1024];
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                          |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

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
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_drive command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_drive command(): Command failed\n");
  return(-1);
 }
 return(0); 
}


int BT_motor_port_stop(char port_ids, int brake_mode)
{
 //////////////////////////////////////////////////////////////////////////////////
 // Stop the motor(s) at the specified ports. This does not change the output
 // power settings!
 //
 // Inputs: Port ids of the motors that should be stopped
 // 	    brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 char reply[1024];
 unsigned char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
 //                           |length-2| | cnt_id | |type| | header |  |stop|   |layer|  |port ids|  |brake|
 
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
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_drive command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_drive command(): Command failed\n");
  return(-1);
 }

 return(0);
}


int BT_all_stop(int brake_mode){
 //////////////////////////////////////////////////////////////////////////////////////////////////////
 // Stops all motor ports - provided for convenience, of course you can do the same with the 
 // functions above.
 //
 // Inputs: brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 unsigned char *cp;
 char reply[1024];
 char port_ids = MOTOR_A|MOTOR_B|MOTOR_C|MOTOR_D;
 unsigned char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
 //                           |length-2| | cnt_id | |type| | header |  |stop|   |layer|  |port ids|  |brake|

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
 read(*socket_id,&reply[0],1023);
 message_id_counter++;

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_drive command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_drive command(): Command failed\n");
  return(-1);
 }

 return(0);
}


int BT_drive(char lport, char rport, char power){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 // This function sends a command to the left and right motor ports to set the motor power to
 // the desired value. You can drive forward or backward depending on the sign of the power
 // value.
 //
 // Please note that not all motors are created equal - over time, motor performance will vary
 // so you can expect that setting both motors to the same speed will result in a motion that
 // is not straight. You can adjust for this by creating a function that adjusts the power to
 // whichever motor is shown to be more powerful so as to bring it down to the level of the
 // least performing motor. 
 //
 // Ports are identified as MOTOR_A, MOTOR_B, etc. - see btcomm.h
 // Power must be in [-100, 100]
 //
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 // to fully stop the motors you need to use the appropriate BT command.
 //
 // Inputs: port identifier of left port
 //         port identifier of right port
 //         power for ports in [-100, 100]
 //
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 
 void *p;
 unsigned char *cp;
 char ports;
 char reply[1024];
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                           |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_drive: Power must be in [-100, 100]\n");
  return(-1);
 }

 if (lport>8 || rport>8)
 {
  fprintf(stderr,"BT_drive: Invalid port id value\n");
  return(-1);
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
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_drive command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_drive command(): Command failed\n");
  return(-1);
 }

 return(0);
}


int BT_turn(char lport, char lpower, char rport, char rpower){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This function sends a command to the left and right motor ports to set the motor power to
 // the desired value for the purpose of turning or spinning. 
 //
 // Ports are identified as MOTOR_A, MOTOR_B, etc
 // Power must be in [-100, 100]
 //
 // Example uses (assumes MOTOR_A is on the right wheel, MOTOR_B is on the left wheel):
 //	    BT_turn(MOTOR_A, 100, MOTOR_B, 90);      <-- Turn toward the left gently
 //	    BT_turn(MOTOR_A, 100, MOTOR_B, 50);      <-- Turn toward the left more sharply
 //	    BT_turn(MOTOR_A, 100, MOTOR_B, 0);       <-- Turn toward the left at the highest possible rate
 //     BT_turn(MOTOR_A, -50, MOTOR_B, -100);    <-- Turn toward the right while driving backward
 //	    BT_turn(MOTOR_A, 100, MOTOR_B, -100);    <-- Spin counter-clockwise at full speed
 //	    BT_turn(MOTOR_A, -50, MOTOR_B, 50);      <-- Spin clockwise at half speed
 //
 // Inputs: port identifier of left port
 //         power for left port in [-100, 100]
 //         port identifier of right port
 //         power for right port in [-100, 100]
 //
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 char reply[1024];
 unsigned char cmd_string[20]={0x12,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0xA4,      0x00,    0x00,      0x81,0x00,    0xA4,     0x00,     0x00, 0x81,0x00,  0xA6,    0x00,   0x00};
 //                          |length-2| | cnt_id | |type| | header |  |set power| |layer|  |lport id|  |power|  |set power| |layer| |rport id| |power|     |start|  |layer| |port ids|

 if (lpower>100||lpower<-100||rpower>100||lpower<-100)
 {
  fprintf(stderr,"BT_drive: Power must be in [-100, 100]\n");
  return(-1);
 }

 if (lport>8 || rport>8)
 {
  fprintf(stderr,"BT_drive: Invalid port id value\n");
  return(-1);
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
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_turn command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_turn command(): Command failed\n");
  return(-1);
 }

 return(0);
}


int BT_timed_motor_port_start(char port_id, char power, int ramp_up_time, int run_time, int ramp_down_time){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Provides timed operation of the motor ports. This allows you, for example, to create carefully timed
 // turns (e.g. when you want to achieve turning by a certain angle), or to implement actions such as
 // kicking a ball, etc.
 //
 // This function provides for smooth power control by allowing you to specify how long the motor will
 // take to spin up to full power, and how long it will take to wind down back to full stop.
 //
 // Other operations can take place during this call, but the order of execution is not guaranteed.
 //
 // Power must be in [-100, 100]
 //
 // Inputs: port identifier
 //         power for port in [-100, 100]
 //         time in ms
 //
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 char reply[1024];
 unsigned char cmd_string[22]={0x00,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0x00,  0x00,   0x00,     0x81,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,  0x00,0x00,0x00,     0x00};
 //                          |length-2| | cnt_id | |type|   |header|    |cmd| |layer| |port ids|  |power|      |ramp up|      |run|           |ramp down|      |brake|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_timed_motor_port_start: Power must be in [-100, 100]\n");
  return(-1);
 }

 if (port_id>8)
 {
  fprintf(stderr,"BT_timed_motor_port_start: Invalid port id value\n");
  return(-1);
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[0]=LC0(20);
 cmd_string[7]=opOUTPUT_TIME_POWER;
 cmd_string[9]=port_id;
 cmd_string[11]=power;
 cmd_string[12]=LC2_byte0(); //ramp up
 cmd_string[13]=LX_byte1(ramp_up_time);
 cmd_string[14]=LX_byte2(ramp_up_time);
 cmd_string[15]=LC2_byte0(); //run
 cmd_string[16]=LX_byte1(run_time);
 cmd_string[17]=LX_byte2(run_time);
 cmd_string[18]=LC2_byte0(); //ramp down
 cmd_string[19]=LX_byte1(ramp_down_time);
 cmd_string[20]=LX_byte2(ramp_down_time);
 cmd_string[21]=0;

#ifdef __BT_debug
 fprintf(stderr,"BT_motor_port_start command string:\n");
 for(int i=0; i<22; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],22);
 read(*socket_id,&reply[0],1023);

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_motor_port_start command(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_motor_port_start command(): Command failed\n");
  return(-1);
 }

 if (reply[4]==0x02){
  fprintf(stderr,"BT_motor_port_start command(): Command successful\n");
  return(reply[5]!=0);
 }
 else{
  fprintf(stderr,"BT_motor_port_start command(): Command failed\n");
  return(-1);
 }

 message_id_counter++;

 return(0);
}


int BT_timed_motor_port_start_v2(char port_id, char power, int time){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This is a quick call provided for convenience - it sets the motor to the specified power
 // for the specified amount of time without ramp-up or ramp-down. This is a blocking call,
 // other operations will resume after this function finishes running.
 //
 // Power must be in [-100, 100]
 //
 // Note that starting a motor at 0% power is *not the same* as stopping the motor.
 // to fully stop the motors you need to use the appropriate BT command.
 //
 // Inputs: port identifier
 //         power for port in [-100, 100]
 //         time in ms
 //
 // Returns: 0 on success
 //          -1 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 char reply[1024];

 unsigned char cmd[26]= {0x00,0x00, 0x00,0x00, 0x00, 0x00,0x00,  0xA4,   0x00,  0x00, 0x81,0x00, 0xA6,  0x00,   0x00,   0x00, 0x00, 0x00,0x00, 0x00,       0x00,   0x00,      0xA3, 0x00,     0x00,   0x00};
 //                     |length-2| |cnt_id|   |type| |header| |set_pwr| |layer| |port| |power|  |start| |layer| |port|  |wait| |LC2|    |time| |var addr| |ready| |var addr| |stop| |layer| |port_id| |break|

 if (power>100||power<-100)
 {
  fprintf(stderr,"BT_timed_motor_port_start: Power must be in [-100, 100]\n");
  return(-1);
 }

 if (port_id>8)
 {
  fprintf(stderr,"BT_timed_motor_port_start: Invalid port id value\n");
  return(-1);
 }

 BT_motor_port_start(port_id, power);

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd[2]=*cp;
 cmd[3]=*(cp+1);

 cmd[0]=LC0(24);
 cmd[6]=LC0(10<<2); //size of local memory
 cmd[9]=port_id;
 cmd[11]=power;
 cmd[14]=port_id;

 cmd[15]=opTIMER_WAIT;
 cmd[16]=LC2_byte0();
 cmd[17]=LX_byte1(time);
 cmd[18]=LX_byte2(time);
 cmd[19]=LV0(0);

 cmd[20]=opTIMER_READY;
 cmd[21]=LV0(0);

 cmd[24]=port_id;
#ifdef __BT_debug
 fprintf(stderr,"BT_timed_motor_port_start timer ready command:\n");
 for(int i=0; i<26; i++)
 {
  fprintf(stderr,"%X, ",cmd[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd[0],26);
 read(*socket_id,&reply[0],1023);

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_motor_port_startv2(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_motor_port_startv2(): Command failed\n");
  return(-1);
 }

 message_id_counter++;

 return(0);
}


void BT_get_type_mode(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Displays on stderr the type and mode of sensor plugged into the specified sensor port. 
 // Can be useful for debugging sensor issues. Certain sensors switch to the wrong type or become 
 // unusable after changing modes (for example gyro sensor).
 //
 // Inputs: port identifier
 //
 //
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 char reply[1024];
 memset(reply,0,1024);
 unsigned char *cp;
 unsigned char cmd_string[13]={0x0B,0x00, 0x00,0x00, 0x00,  0x02,0x00,  0x00,    0x00,       0x00,    0x00,  0x00, 0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |global var addr|

 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_colour_sensor: Invalid port id value\n");
 }

 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=GET_TYPEMODE;
 cmd_string[10]=sensor_port;
 cmd_string[11]=GV0(0x00); //global var
 cmd_string[12]=GV0(0x01); //global var

 fprintf(stderr,"BT_get_type_mode command string:\n");
 for(int i=0; i<13; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");

 write(*socket_id,&cmd_string[0],13);
 read(*socket_id,&reply[0],1023);

 fprintf(stderr,"BT_get_type_mode response string:\n");
 for(int i=0; i<7; i++)
 {
  fprintf(stderr,"%X, ",reply[i]&0xff);
 }
 fprintf(stderr,"\n");

 printf("type: %d, mode: %d\n", reply[5], reply[6]);

 message_id_counter++;
}


int BT_read_touch_sensor(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 // Reads the value from the touch sensor.
 //
 // Ports are identified as PORT_1, PORT_2, etc
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
  return(-1);
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
#ifdef __BT_debug
  fprintf(stderr,"BT_touch_sensor(): Command successful\n");
#endif
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
 // Reads the value from the colour sensor using the indexed colour method provided by Lego. 
 //
 // Make sure you test and calibrate your sensor thoroughly - you can expect mis-reads, you can
 // expect the operation of the sensor will be dependent on ambient illumination, the rreflectivity
 // of the surface, and battery power. Your code will need to deal with all these external factors.
 //
 // Ports are identified as PORT_1, PORT_2, etc
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
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 char reply[1024];
 memset(&reply[0],0,1024);
 unsigned char *cp;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00 };
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|

 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_colour_sensor: Invalid port id value\n");
  return(-1);
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
#ifdef __BT_debug
  fprintf(stderr,"BT_colour_sensor(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_colour_sensor(): Command failed\n");
 }
 return reply[5];
}


int BT_read_colour_sensor_RGB(char sensor_port, int RGB[3]){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the value from the colour sensor returning an RGB colour triplet.
 // Notice that this should prove more informative than the indexed colour, however, you'll then
 // have to determine what colour the bot is actually looking at.
 //
 // Several ways exist to do this, you can define reference RGB values and compute difference
 // between what the sensor read and the reference, or you can get fancier and use a different
 // colour space such as HSV (if you want to do that, read up a bit on this - it's easily 
 // found).
 // Return values are in R[0, 1020], G[0, 1020] and B[0, 1020].
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 // Inputs: port identifier of colour sensor port, an INT array with 3 entries where the RGB
 //         triplet will be returned.
 //
 // Returns:
 //          -1 if EV3 returned an error response
 //           0 on success
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char reply[1024];
 memset(&reply[0],0,1024); 
 unsigned char *cp;
 uint32_t R=0, G=0, B=0;
 double normalized;

 unsigned char cmd_string[17]={0x00,0x00, 0x00,0x00, 0x00,  0x0C,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00, 0x00, 0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_colour_sensor_RGB: Invalid port id value\n");
  return(-1);
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
#ifdef __BT_debug
  fprintf(stderr,"BT_colour_sensor_RGB(): Command successful\n");
  fprintf(stderr,"BT_read_colour_sensor_RGB response string:\n");
  for(int i=0; i<17; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif

  R|=(uint32_t)reply[8];
  R<<=8;
  R|=(uint32_t)reply[7];
  R<<=8;
  R|=(uint32_t)reply[6];
  R<<=8;
  R|=(uint32_t)reply[5];

  G|=(uint32_t)reply[12];
  G<<=8;
  G|=(uint32_t)reply[11];
  G<<=8;
  G|=(uint32_t)reply[10];
  G<<=8;
  G|=(uint32_t)reply[9];

  B|=(uint32_t)reply[16];
  B<<=8;
  B|=(uint32_t)reply[15];
  B<<=8;
  B|=(uint32_t)reply[14];
  B<<=8;
  B|=(uint32_t)reply[13];


  RGB[0]=R;
  RGB[1]=G;
  RGB[2]=B;
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
 // Reads the value from ultrasonic sensor and returns distance in mm to any object in front of the sensor.
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 // Inputs: port identifier for the ultrasonic sensor port
 //
 // Returns: distance in mm
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char reply[1024];
 memset(&reply[0],0,1024);
 unsigned char *cp;

 unsigned char cmd_string[15]={0x00,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_ultrasonic_sensor: Invalid port id value\n");
  return(-1);
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
#ifdef __BT_debug
   fprintf(stderr,"BT_ultrasonic_sensor(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_ultrasonic_sensor: Command failed\n");
  return(-1);
 }
 return (reply[5]);
}

int BT_read_colour_RGBraw_NXT(char sensor_port, int *R, int *G, int *B, int *A)
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Reads the current RGB and ambient light raw values from the NXTread_colour_RGBraw_NXT color sensor.
    // The sensor will be showing a white light (all colour LEDs should be on).
    //
    // Ports are specified as PORT_1, PORT_2, etc.
    //
    // Inputs: The port id for the color sensor
    //         Pointers to the integer variables where the RGB+A data will be returned
    //         The RGB values returned *have had the ambient value subtracted already!*
    //         The NXT colour sensor returns 16-bit integers, so the range is NOT [0, 255] 
    //         and careful colour calibration will be required to determine what each of
    //         the colours in the map look like to the NXT sensor based on where it's placed
    //         on the bot, the ambient illumination conditions, etc.
    //         So - CALIBRATE!
    //
    //         This function exists because the EV3 colour sensor has a bad tendency to
    //         power down, then reset to reflectance mode every couploe seconds. Giving a
    //         very bad rate of useful reads. The NXT sensor can read continuosly and 
    //         fast without resetting or weird behaviour. So this function is recommended
    //         over the one using the EV3 sensor.
    //
    //
    // Returns: -1 if something went wrong
    //           1 on success
    ////////////////////////////////////////////////////////////////////////////////////////////
    
 void *p;
 unsigned char reply[1024];
 unsigned char *cp;
 int16_t r,g,b,a;
 int cmdlen;
 int replen;

 // Reset the reply message buffer
 memset(&reply[0],0,1024);
 r=g=b=0;
 
 // Pre-defined command sequence for reading the gyro sensor's angle and rate 
 unsigned char CMD_READ_COLOUR_RGBRAW[17]={0x0F,0x00,0x8E,0x00,0x00,0x0C,0x00,0x99,0x1C,0x00,0x00,0x1D,0x04,0x03,0x60,0x64,0x68};     // Pre-defined as per the ev3_dc library

 cmdlen=CMD_READ_COLOUR_RGBRAW[0]+2;
 unsigned char cmd_string[1024];
 memcpy(&cmd_string[0],&CMD_READ_COLOUR_RGBRAW[0],cmdlen);

 if (sensor_port>4)
 {
  fprintf(stderr,"BT_read_colour_RGBraw(): Invalid port id value\n");
  return(-1);
 }

 // Set message count id            // Message id count is kept in an int variable, but it has to be stuffed in little endian format, 
 p=(void *)&message_id_counter;     // so the code here stuffs the counter in the right order in the string, lowest byte first
 cp=(unsigned char *)p;
 cmd_string[2]=*(cp);
 cmd_string[3]=*(cp+1);
 cmd_string[10]=sensor_port;         // Sensor port
 cmd_string[11]=0x04;		         // Set type to NXT color sensor
 cmd_string[12]=0x05;		         // Set NXT sensor mode 5 RGB+A

 #ifdef __BT_debug
 fprintf(stderr,"BT_read_colour_RGBraw_NXT() command string\n");
 for(int i=0; i<cmdlen; i++)
 {
  fprintf(stderr,"%02X, ",cmd_string[i]);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],cmdlen);
 while(read(*socket_id,&reply[0],1023)==0);       // Wait for a reply!
 message_id_counter++;
 
 if (reply[4]==0x02){
  r=*((int16_t *)&reply[5]);                      // Unpack return data and copy to destination (int) variables
  g=*((int16_t *)&reply[7]);
  b=*((int16_t *)&reply[9]);
  a=*((int16_t *)&reply[11]);
  *R=(int)(r-a);
  *G=(int)(g-a);
  *B=(int)(b-a);
  *A=(int)a;
  
#ifdef __BT_debug
  fprintf(stderr,"BT_read_color_RGBraw_NXT(): Command successful\n");
  fprintf(stderr,"BT_read_color_RGBraw_NXT() response string:\n");
  replen=reply[0]+2;
  for(int i=0; i<replen; i++)
  {
   fprintf(stderr,"%02X, ",reply[i]);
  }
  fprintf(stderr,"\n");
  fprintf(stderr, "R=%d, G=%d, B=%d, A=%d\n", r,g,b,a);
#endif
 }
 else{
  fprintf(stderr,"BT_read_color_RGBraw(): Command failed\n");
  return(-1);
 }
 return(1); 
}


int BT_read_gyro(char sensor_port, int reset, int *angle, int *rate){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Updates the current angle and reate of change according to the gyro sensor on the robot. 
 // The angle is measured relative to the *last time the reference was set* - the reference is 
 // set by calling this function with the reset flag set to 1. So, to use the sensor, normally 
 // you would first reset the reference angle in this way, and then proceed to use the gyro sensor 
 // to measure the amount of turning relative to this reference.
 //
 // Like anything else, the sensor has drift, so some form of correction will be needed after
 // multiple successive readings. Using the sensor rate of change may be useful for this.
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 // Inputs: port identifier of gyro sensor port
 //         reset flag, if NOT ZERO, then the reference angle is set to the current angle
 //         pointers to variables where we will store the angle and rate
 //
 // Returns: 1 on success
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char reply[1024];
 unsigned char *cp;
 int ang=0;
 int rat=0;
 int cmdlen;
 int replen;

 // Reset the reply message buffer
 memset(&reply[0],0,1024);
 
 // Pre-defined command sequence for reading the gyro sensor's angle and rate 
 unsigned char CMD_READ_GYRO_ANGRATE[17]={0x0F,0x00,0x00,0x00,0x00,0x80,0x00,0x99,0x1C,0x00,0x01,0x81,0x20,0x03,0x02,0x60,0x64};

 cmdlen=CMD_READ_GYRO_ANGRATE[0]+2;
 unsigned char cmd_string[1024];
 memcpy(&cmd_string[0],&CMD_READ_GYRO_ANGRATE[0],cmdlen);

 if (sensor_port>4)
 {
  fprintf(stderr,"BT_read_gyro_sensor(): Invalid port id value\n");
  return(-1);
 }

 // Set message count id            // Message id count is kept in an int variable, but it has to be stuffed in little endian format, 
 p=(void *)&message_id_counter;     // so the code here stuffs the counter in the right order in the string, lowest byte first
 cp=(unsigned char *)p;
 cmd_string[2]=*(cp);
 cmd_string[3]=*(cp+1);
 cmd_string[10]=sensor_port;         // Port

#ifdef __BT_debug
 fprintf(stderr,"BT_read_gyro_sensor() command string\n");
 for(int i=0; i<cmdlen; i++)
 {
  fprintf(stderr,"%02X, ",cmd_string[i]);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],cmdlen);
 while(read(*socket_id,&reply[0],1023)==0);       // Wait for a reply!
 message_id_counter++;
 
 if (reply[4]==0x02){
  ang=*((int *)&reply[5]);
  if (reset>0) ref_angle=ang;
  ang=ang-ref_angle;
  rat=*((int *)&reply[9]);    
  
  *(angle)=ang;
  *(rate)=rat;
  
#ifdef __BT_debug
  fprintf(stderr,"BT_read_gyro_sensor(): Command successful\n");
  fprintf(stderr,"BT_read_gyro_sensor() response string:\n");
  replen=reply[0]+2;
  for(int i=0; i<13; i++)
  {
   fprintf(stderr,"%02X, ",reply[i]);
  }
  fprintf(stderr,"\n");
  fprintf(stderr, "angle: %d, rate=%d\n", angle, rate);
#endif
 }
 else{
  fprintf(stderr,"BT_read_gyro_sensor(): Command failed\n");
  return(-1);
 }
 return(1);
}


int BT_play_sound_file(const char *path, int volume){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Plays the sound at the specified path, the file path should not include the extension.
 //
 // Inputs: path - null-terminated path, with maximum length of 1012 bytes including the nullbyte
 //         volume - range [0, 100]
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 char reply[1024];
 memset(&reply[0],0,1024);
 unsigned char *cp;
 int msg_length=0;
 int path_len=0;
 path_len=strnlen(path, 1011);
 unsigned char cmd_string[1024];
 memset(cmd_string, 0, 1024);

 cmd_string[0]=LX_byte1(12+path_len+1-2); //length-2
 cmd_string[1]=LX_byte2(12+path_len+1-2); //length-2
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[4]=0; //command type - with reply
 cmd_string[5]=0; //global and local memory
 cmd_string[6]=0;
 cmd_string[7]=opSOUND;
 cmd_string[8]=PLAY;
 cmd_string[9]=LC1_byte0(); //Volume
 cmd_string[10]=LX_byte1(volume); //Volume
 cmd_string[11]=LCS;//file path
 for (int i=0; i<path_len; i++){
   cmd_string[i+12]=path[i];
 }

#ifdef __BT_debug
 fprintf(stderr,"BT_play_sound_file command string\n");
 for(int i=0; i<12+path_len+1; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],12+path_len+1);
 read(*socket_id,&reply[0],1023);
 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_play_sound_file(): Command successful\n");
#ifdef __BT_debug
 fprintf(stderr,"BT_play_sound_file response string:\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",reply[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif
 }
 else{
  fprintf(stderr,"BT_play_sound_file: Command failed\n");
  for(int i=0; i<16; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
  return(-1);
 }
 return (0);
}


//TODO: add the ability to read long directories that cannot be finished in one read
int BT_list_files(char *path, char **msg_reply){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the directory contents at the null-terminated path.
 //
 // Inputs: path - null-terminated path, with maximum length of 1012 bytes including the nullbyte
 //         msg_reply - memory will be allocated by list_files to hold the response,
 //         the response string contains subdirectories/files specified by path delimeted by '\n'
 //         the calling code is responsible for freeing the memory from msg_reply
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 int i;
 char reply[1024];
 memset(reply,0,1024);
 unsigned char *cp;
 unsigned int msg_length=0;
 int path_len=0;
 path_len=strnlen(path, 1011);
 unsigned char cmd_string[1024];
 memset(cmd_string,0,1024);

 cmd_string[0]=LX_byte1(8+path_len-2+1); //length-2
 cmd_string[1]=LX_byte2(8+path_len-2+1); //length-2
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[4]=SYSTEM_COMMAND_REPLY; //type
 cmd_string[5]=LIST_FILES; //system_cmd
 cmd_string[6]=LX_byte1(1012); //max bytes to read
 cmd_string[7]=LX_byte2(1012);
 for (i=0; i<path_len; i++){
   cmd_string[i+8]=path[i];
 }
 cmd_string[8+path_len]='\0';

#ifdef __BT_debug
 fprintf(stderr,"BT_list_files command string\n");
 for(i=0; i<8+path_len+1; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],8+path_len+1);
 read(*socket_id,&reply,1023);

 message_id_counter++;

 if (reply[4]==SYSTEM_REPLY){
  msg_length |= (unsigned char)reply[1];
  msg_length<<=8;
  msg_length |= (unsigned char)reply[0];
  msg_length += 2;
#ifdef __BT_debug
  fprintf(stderr,"BT_list_files(): Command successful\n");
  fprintf(stderr,"BT_list_files response string:\n");
  for(int i=0; i<msg_length; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif
  *msg_reply=(char *)calloc(msg_length-11, sizeof(char));
  if (*msg_reply == NULL){
    perror("calloc");
    return(-1);
  }

  if (reply[6] == SUCCESS || reply[6] == END_OF_FILE){
    strncpy(*msg_reply, &reply[12], msg_length-11);
  }
  else {
    return reply[6];
  }
  fprintf(stderr,"\n");
 }
 else{
  fprintf(stderr,"BT_list_files: Command failed\n");
  return(reply[4]);
 }
 return (reply[6]);
}


int BT_upload_file(char const *dest, char const *src){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Upload the file at src on the PC to dest on EV3 brick.
 //
 // Inputs: src - null-terminated path to file on PC, should be in correct format (.rsf sound files, 
 //         .rgf image files, etc).
 //         dest - null-terminated path to file on EV3 brick to download the file, relative paths
 //         are relative to /home/root/lms2012/sys. If the paths are absolute they should begin with
 //         /home/root/lms2012/apps, /home/root/lms2012/prjs or /home/root/lms2012/tools. At these paths
 //         the files should be placed inside a subfolder so that they will be visible in the EV3 display.
 //         The path will be truncated at 1011 bytes, not including the null-byte.
 //
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 FILE *fp;
 void *p;
 char buffer[PARTITION_SIZE];
 int i, size, remainder, n;
 char reply[1024];
 memset(&reply[0],0,1024);
 unsigned char *cp;
 const char *p1="/home/root/lms2012/apps";
 const char *p2="/home/root/lms2012/prjs";
 const char *p3="/home/root/lms2012/tools";

 int path_len=0;
 unsigned int msg_length=0;
 int handle;

 unsigned char cmd_string[1024];
 memset(&cmd_string[0],0,1024);
 struct stat st;

 if ((dest[0] == '/') && (strncmp(p1, dest, strlen(p1)) != 0) && (strncmp(p2, dest, strlen(p2)) != 0) && (strncmp(p3, dest, strlen(p3)) != 0)){
   fprintf(stderr, "Absolute destination path should begin with /home/root/lms2012/app, /home/root/lms2012/prjs or /home/root/lms2012/tools\n");
   return(-1);
 }

 path_len=strnlen(dest, 1011);

 stat(src, &st);
 size=st.st_size;

 cmd_string[0]=LX_byte1(10+path_len-2+1); //length-2
 cmd_string[1]=LX_byte2(10+path_len-2+1); //length-2
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[4]=SYSTEM_COMMAND_REPLY; //type
 cmd_string[5]=BEGIN_DOWNLOAD; //system_cmd
 cmd_string[6]=LX_byte1(size); //file size
 cmd_string[7]=LX_byte2(size);
 cmd_string[8]=LX_byte3(size);
 cmd_string[9]=LX_byte4(size);
 for (i=0; i<path_len; i++){
   cmd_string[i+10]=dest[i];
 }
 cmd_string[10+path_len]='\0';

#ifdef __BT_debug
 fprintf(stderr,"BT_upload_file command string\n");
 for(i=0; i<10+path_len+1; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],10+path_len+1); //this will return a handle to the file
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==SYSTEM_REPLY){
  msg_length = (unsigned char)reply[1];
  msg_length<<=8;
  msg_length |= (unsigned char)reply[0];
  msg_length += 2;
#ifdef __BT_debug
  fprintf(stderr,"BT_upload_file response string:\n");
  for(int i=0; i<msg_length; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif
  if (reply[6] == SUCCESS){
    fprintf(stderr,"BT_upload_file(): Command successful\n");
    handle=reply[8];
  }
  else {
    return reply[6];
  }
  fprintf(stderr,"\n");
 }
 else{
  fprintf(stderr,"BT_upload_file: Command failed\n");
  return(reply[4]);
 }

 if((fp = fopen(src, "rb")) == NULL) {
  perror(src);
  return(-1);
 }

 remainder = size > PARTITION_SIZE ? PARTITION_SIZE : size;
 while (remainder > 0){
   memset(&cmd_string[0],0,1024);

   n = fread(buffer, 1, remainder, fp);
   cmd_string[0]=LX_byte1(7+remainder-2); //length-2
   cmd_string[1]=LX_byte2(7+remainder-2); //length-2
   // Set message count id
   p=(void *)&message_id_counter;
   cp=(unsigned char *)p;
   cmd_string[2]=*cp;
   cmd_string[3]=*(cp+1);

   cmd_string[4]=SYSTEM_COMMAND_REPLY; //type
   cmd_string[5]=CONTINUE_DOWNLOAD; //system_cmd
   cmd_string[6]=LX_byte1(handle); //handle
   for (i=0; i<remainder; i++){
     cmd_string[i+7]=buffer[i];
   }

#ifdef __BT_debug
   fprintf(stderr,"BT_upload_file command string\n");
   for(i=0; i<7+remainder; i++)
   {
     fprintf(stderr,"%X, ",cmd_string[i]&0xff);
   }
   fprintf(stderr,"\n");
#endif

   write(*socket_id,&cmd_string[0],7+remainder);
   read(*socket_id,&reply[0],1023);

   message_id_counter++;

   if (reply[4]==SYSTEM_REPLY){
    msg_length = (unsigned char)reply[1];
    msg_length<<=8;
    msg_length |= (unsigned char)reply[0];
    msg_length += 2;
#ifdef __BT_debug
    fprintf(stderr,"BT_upload_file response string:\n");
    for(int i=0; i<msg_length; i++)
    {
      fprintf(stderr,"%X, ",reply[i]&0xff);
    }
    fprintf(stderr,"\n");
#endif
    if (reply[6] == SUCCESS){
#ifdef __BT_debug
      fprintf(stderr,"BT_upload_file(): Command successful\n");
#endif
    }
    else if (reply[6] == END_OF_FILE){
#ifdef __BT_debug
      fprintf(stderr,"BT_upload_file(): Command completed\n");
#endif
    }
    else {
      return reply[6];
    }
    fprintf(stderr,"\n");
   }
   else{
#ifdef __BT_debug
    fprintf(stderr,"BT_upload_file: Command failed\n");
#endif
    return(reply[4]);
   }
   size-=n;
   remainder = size > PARTITION_SIZE ? PARTITION_SIZE : size;
 }
 fclose(fp);
 return (reply[6]);
}


int BT_set_LED_colour(int colour){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Set the LED around the EV3 buttons to specified colour.
 //
 // Inputs: colour - one of the macros LED_BLACK, LED_GREEN, LED_RED, LED_ORANGE, LED_GREEN_FLASH,
 // LED_RED_FLASH, LED_ORANGE_FLASH, LED_GREEN_PULSE, LED_RED_PULSE, LED_ORANGE_PULSE
 //
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 unsigned char cmd_string[10]={0x00,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0x00,    0x00,      0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |ui cmd | |colour|

 void *p;
 unsigned char *cp;
 char reply[1024];
 memset(&reply[0],0,1024);

 if (colour != LED_BLACK && colour != LED_GREEN && colour != LED_RED && colour != LED_ORANGE && colour != LED_GREEN_FLASH && 
    colour != LED_RED_FLASH && colour != LED_ORANGE_FLASH && colour != LED_GREEN_PULSE && colour != LED_ORANGE_PULSE){
    fprintf(stderr,"BT_set_LED_colour: Invalid colour value\n");
    return(-1);
 }

 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[0]=LC0(8);
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 cmd_string[7]=opUI_WRITE;
 cmd_string[8]=LED;
 cmd_string[9]=colour;

#ifdef __BT_debug
 fprintf(stderr,"BT_set_LED_colour command string\n");
 for(int i=0; i<10; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],10);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

#ifdef __BT_debug
  fprintf(stderr,"BT_set_LED_colour(): response string\n");
  for(int i=0; i<5; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_set_LED_colour(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_set_LED_colour: Command failed\n");
  return(-1);
 }
 return (0);
}


int BT_draw_image_from_file(int colour, int x_0, int y_0, const char *file_path){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Display the image at specified file path on display starting at x_0, y_0. The file should be
 // a valid.rgf file.
 //
 // Inputs: colour - 0 = white, 1 = black
 //         x_0 - position of left edge [0-177]
 //         y_0 - position of top edge [0-127]
 //         file_path - file path without extension, max 1004 chars + nullbyte
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 void *p;
 unsigned char *cp;
 int i;
 char reply[1024];
 memset(&reply[0],0,1024);

 int msg_length=0;
 int path_len=0;
 path_len=strnlen(file_path, 1004);
 unsigned char cmd_string[1024];
 memset(&cmd_string[0],0,1024);

 if (x_0 < 0 || x_0 > 177){
    fprintf(stderr,"BT_draw_image_file: Invalid x_0 coordinate\n");
    return(-1);
 }

 if (y_0 < 0 || y_0 > 127){
    fprintf(stderr,"BT_draw_image_file: Invalid y_0 coordinate\n");
    return(-1);
 }

 if (colour != 0 && colour != 1){
    fprintf(stderr,"BT_draw_image_file: Invalid colour\n");
    return(-1);
 }

 cmd_string[0]=LX_byte1(20+path_len-2+1); //length-2
 cmd_string[1]=LX_byte2(20+path_len-2+1); //length-2

 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 cmd_string[7]=opUI_DRAW;
 cmd_string[8]=BMPFILE;
 cmd_string[9]=LC1_byte0(); //colour
 cmd_string[10]=LX_byte1(colour);
 cmd_string[11]=LC2_byte0();
 cmd_string[12]=LX_byte1(x_0);
 cmd_string[13]=LX_byte2(x_0);
 cmd_string[14]=LC2_byte0();
 cmd_string[15]=LX_byte1(y_0);
 cmd_string[16]=LX_byte2(y_0);
 cmd_string[17]=LCS;
 for (i=0; i<path_len; i++){
   cmd_string[i+18]=file_path[i];
 }
 cmd_string[18+path_len]='\0';
 cmd_string[19+path_len]=opUI_DRAW; //refreshes display to output image
 cmd_string[20+path_len]=UPDATE;

#ifdef __BT_debug
 fprintf(stderr,"BT_draw_image_from_file command string\n");
 for(int i=0; i<20+path_len+1; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],20+path_len+1);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

#ifdef __BT_debug
  fprintf(stderr,"BT_draw_image_from_file(): response string\n");
  for(int i=0; i<5; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_draw_image_file(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_draw_image_file: Command failed\n");
  return(-1);
 }
 return (0);
}


int BT_store_current_display(int no){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Store the current display, can be used to restore the display to the current state using
 // restore_previous_display.
 //
 // Inputs: 
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 unsigned char cmd_string[10]={0x00,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0x00,    0x00,      0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |ui cmd |    |no|

 void *p;
 unsigned char *cp;
 char reply[1024];
 memset(&reply[0],0,1024);

 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[0]=LC0(8);
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 cmd_string[7]=opUI_DRAW;
 cmd_string[8]=STORE;
 cmd_string[9]=no;

#ifdef __BT_debug
 fprintf(stderr,"BT_set_current_display command string\n");
 for(int i=0; i<10; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],10);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

#ifdef __BT_debug
  fprintf(stderr,"BT_set_current_display(): response string\n");
  for(int i=0; i<5; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_set_current_display(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_set_current_display: Command failed\n");
  return(-1);
 }
 return (0);
}


int BT_restore_previous_display(int no){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Restore the display to the one stored at no set by call to store_current_display.
 //
 // Inputs: no - number of stored display
 //
 //
 // Returns: success code on successfull execution
 //          error code on error
 //////////////////////////////////////////////////////////////////////////////////////////////////

 unsigned char cmd_string[12]={0x00,0x00, 0x00,0x00, 0x00,  0x00,0x00,  0x00,    0x00,      0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |ui cmd |    |no|

 void *p;
 unsigned char *cp;
 char reply[1024];
 memset(&reply[0],0,1024);

 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[0]=LC0(10);
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);
 cmd_string[7]=opUI_DRAW;
 cmd_string[8]=RESTORE;
 cmd_string[9]=no;
 cmd_string[10]=opUI_DRAW;;
 cmd_string[11]=UPDATE;

#ifdef __BT_debug
 fprintf(stderr,"BT_restore_previous_display command string\n");
 for(int i=0; i<12; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],12);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

#ifdef __BT_debug
  fprintf(stderr,"BT_restore_previous_display(): response string\n");
  for(int i=0; i<5; i++)
  {
   fprintf(stderr,"%X, ",reply[i]&0xff);
  }
  fprintf(stderr,"\n");
#endif

 if (reply[4]==0x02){
#ifdef __BT_debug
  fprintf(stderr,"BT_restore_previous_display(): Command successful\n");
#endif
 }
 else{
  fprintf(stderr,"BT_restore_previous_display: Command failed\n");
  return(-1);
 }
 return (0);
}
