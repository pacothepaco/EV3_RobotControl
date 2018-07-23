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
#define RGB_Normalization_Constant  1020.0   // Constant used to normalize RGB values to a range of [0,255]
					     // If you are having trouble with RGB values looking weird, 
					     // get under the hood of the RGB sensor read function, print out
					     // the returned RGB from the EV3 block (which will NOT be in
					     // [0, 255], and see if the values being returned *for a variety
					     // of colours under the lighting conditions you will be working on*
					     // are either much smaller than this constant, or larger.
					     // Adjust the constant so that you can reliably obtain values in
					     // [0,255]. CAVEAT -> read values may be dependent on battery power!
					     
#define __BT_debug			// Uncomment to trigger printing of BT messages for debug purposes

int message_id_counter=1;		// <-- This is a global message_id counter, used to keep track of
					//     messages sent to the EV3
int *socket_id;				// <-- Socked identifier for your EV3

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

 message_id_counter++; 
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
 unsigned char cmd_string[11]={0x09,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA3,   0x00,    0x00,       0x00};
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

 message_id_counter++; 
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
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
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

 message_id_counter++; 
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
 //         BT_turn(MOTOR_A, -50, MOTOR_B, -100);    <-- Turn toward the right while driving backward
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
 unsigned char cmd_string[20]={0x12,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,      0x81,0x00,    0xA4,     0x00,     0x00, 0x81,0x00,  0xA6,    0x00,   0x00};
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

 message_id_counter++;

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
 unsigned char cmd_string[22]={0x00,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0x00,  0x00,   0x00,     0x81,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,  0x00,0x00,0x00,     0x00};
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

 message_id_counter++;

 return(0);
}

int BT_timed_motor_port_start_v2(char port_id, char power, int time){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // This is a quick call provided for convenience - it sets the motor to the specified power
 // for the specified amount of time without ramp-up or ramp-down.
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

 //BT_motor_port_start(port_id, power);

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
  fprintf(stderr,"BT_wait(): Command successful\n");
  return(reply[5]!=0);
 }
 else{
  fprintf(stderr,"BT_wait(): Command failed\n");
  return(-1);
 }

 message_id_counter++;

 return(0);
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
 // Reads the value from the colour sensor returning an RGB colour triplet.
 // Notice that this should prove more informative than the indexed colour, however, you'll then
 // have to determine what colour the bot is actually looking at.
 //
 // Several ways exist to do this, you can define reference RGB values and compute difference
 // between what the sensor read and the reference, or you can get fancier and use a different
 // colour space such as HSV (if you want to do that, read up a bit on this - it's easily 
 // found).
 // 
 // Return values are in R[0, 255], G[0, 255] and B[0, 255].
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

 unsigned char cmd_string[17]={0x00,0x00, 0x00,0x00, 0x00,  0x0C,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00, 0x00, 0x00 };
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
  fprintf(stderr,"BT_colour_sensor_RGB(): Command successful\n");
#ifdef __BT_debug
 fprintf(stderr,"BT_read_colour_sensor_RGB response string:\n");
 for(int i=0; i<17; i++)
 {
  fprintf(stderr,"%X, ",reply[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

  R|=(uint32_t)reply[5];
  R|=(uint32_t)reply[6]<<8;
  G|=(uint32_t)reply[9];
  G|=(uint32_t)reply[10]<<8;
  B|=(uint32_t)reply[13];
  B|=(uint32_t)reply[14]<<8;
  normalized=(double)R/RGB_Normalization_Constant*255.0;
  RGB[0]=(int)normalized;
  normalized=(double)G/RGB_Normalization_Constant*255.0;
  RGB[1]=(int)normalized;
  normalized=(double)B/RGB_Normalization_Constant*255.0;
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
  fprintf(stderr,"BT_ultrasonic_sensor(): Command successful\n");
}
 else{
  fprintf(stderr,"BT_ultrasonic_sensor: Command failed\n");
  return(-1);
 }
 return (reply[5]);
}

int BT_clear_gyro_sensor(char sensor_port){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Clears the sensor data of the gyro sensor - this should reset the sensor to 0 degrees.
 // Note that the sensor is initialized when you first power up the kit, so whatever orientation
 // the bot has at that point, becomes 0 degrees. This function allows you to reset the sensor
 // during operation. This will be needed to set specific reference directions to zero, and to
 // handle sensor drift.
 //
 // Ports are identified as PORT_1, PORT_2, etc
 // 
 // Inputs: port identifier of gyro sensor port
 //
 // Returns: 0 on success
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 unsigned char clr_string[10]={0x00,0x00, 0x00,0x00, 0x00,  0x01,0x00,  0x00,    0x00,       0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|

 void *p;
 unsigned char *cp;
 char reply[1024];
 memset(&reply[0],0,1024);

 if (sensor_port>8)
 {
  fprintf(stderr,"BT_clear_gyro_sensor: Invalid port id value\n");
  return(-1);
 }

 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 clr_string[0]=LC0(8);
 clr_string[2]=*cp;
 clr_string[3]=*(cp+1);
 clr_string[7]=opINPUT_DEVICE;
 clr_string[8]=LC0(CLR_ALL);
 clr_string[10]=sensor_port;

#ifdef __BT_debug
 fprintf(stderr,"BT_clear_gyro_sensor command string\n");
 for(int i=0; i<11; i++)
 {
  fprintf(stderr,"%X, ",clr_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&clr_string[0],10);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_clear_gyro_sensor(): Command successful\n");
 }
 else{
  fprintf(stderr,"BT_clear_gyro_sensor: Command failed\n");
  return(-1);
 }
 return (0);
}

int BT_read_gyro_sensor(char sensor_port, int angle_speed[2]){
 ////////////////////////////////////////////////////////////////////////////////////////////////
 //
 // Reads the values for angle and speed of angle change and fills in the provided array.
 //
 // Ports are identified as PORT_1, PORT_2, etc
 //
 // Inputs: port identifier of gyro sensor port
 //
 // Returns: 0 on success
 //          -1 if EV3 returned an error response
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 char reply[1024];
 memset(&reply[0],0,1024);
 unsigned char *cp;

 unsigned char cmd_string[16]={0x00,0x00, 0x00,0x00, 0x00,  0x02,0x00,  0x00,    0x00,       0x00,    0x00,  0x00,  0x00,   0x00,     0x00, 0x00};
 //                          |length-2| | cnt_id | |type| | header |   |cmd|  |sensor cmd | |layer|  |port| |type| |mode| |data set| |global var addr|


 if (sensor_port>8)
 {
  fprintf(stderr,"BT_read_gyro_sensor: Invalid port id value\n");
  return(-1);
 }

 cmd_string[0]=LC0(14);
 // Set message count id
 p=(void *)&message_id_counter;
 cp=(unsigned char *)p;
 cmd_string[2]=*cp;
 cmd_string[3]=*(cp+1);

 cmd_string[7]=opINPUT_DEVICE;
 cmd_string[8]=LC0(READY_RAW);
 cmd_string[10]=sensor_port;
 cmd_string[11]=LC0(32); //type
 cmd_string[12]=LC0(3);//mode
 cmd_string[13]=LC0(0x02); //data set
 cmd_string[14]=GV0(0x00); //global var
 cmd_string[15]=GV0(0x02);

#ifdef __BT_debug
 fprintf(stderr,"BT_read_gyro_sensor command string\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(*socket_id,&cmd_string[0],16);
 read(*socket_id,&reply[0],1023);

 message_id_counter++;

 if (reply[4]==0x02){
  fprintf(stderr,"BT_read_gyro_sensor(): Command successful\n");
  fprintf(stderr, "angle: %d, speed: %d\n", reply[5], reply[6]);
#ifdef __BT_debug
 fprintf(stderr,"BT_read_gyro_sensor response string:\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",reply[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 }
 else{
  fprintf(stderr,"BT_read_gyro_sensor: Command failed\n");
  return(-1);
 }

 return (0);
}

