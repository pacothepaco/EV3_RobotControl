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

int BT_open(const char *device_id)
{
 //////////////////////////////////////////////////////////////////////////////////////////////////////
 // Open a socket to the specified Lego EV3 device specified by the provided hex ID string
 //
 // Input: The hex string identifier for the Lego EV3 block
 // Returns: An int identifier for an open socket to the device. Don't lose it!
 //
 // Derived from bluetooth.c by Don Neumann
 //////////////////////////////////////////////////////////////////////////////////////////////////////
  
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
 /////////////////////////////////////////////////////////////////////////////////////////////////////
 // Close the communication socket to the EV3
 // Input: socket identifier obtained upon opening the connection.
 /////////////////////////////////////////////////////////////////////////////////////////////////////  
 fprintf(stderr,"Request to close connection to device at socket id %d\n",socket_id);
 close(socket_id);
}

int BT_setEV3name(const char *name, int socket_id)
{
 /////////////////////////////////////////////////////////////////////////////////////////////////////
 // This function can be used to name your EV3. 
 // Inputs: A zero-terminated string containing the desired name, length <=  12 characters
 //         The socket-ID for the lego block
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

 write(socket_id,&cmd_string[0],len+2);

 message_id_counter++; 
 return(1);
}

int BT_motor_port_start(char port_ids, char power, int socket_id)
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
 //         The socket-ID for the Lego block
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
 
 write(socket_id,&cmd_string[0],15);

 message_id_counter++; 
 return(1); 
}

int BT_motor_port_stop(char port_ids, int brake_mode, int socket_id)
{
 //////////////////////////////////////////////////////////////////////////////////
 //
 // Stop the motor(s) at the specified ports. This does not change the output
 // power settings!
 //
 // Inputs: Port ids of the motors that should be stopped
 // 	    brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 //         socket_ID for the lego block
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
 
 write(socket_id,&cmd_string[0],11);

 message_id_counter++; 
 return(1); 
}


int BT_all_stop(int brake_mode, int socket_id){
 //////////////////////////////////////////////////////////////////////////////////////////////////////
 // Applies breaks to all motors.
 //
 // Inputs: brake_mode: 0 -> roll to stop, 1 -> active brake (uses battery power)
 //         socket_ID for the lego block
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

 write(socket_id,&cmd_string[0],11);

 message_id_counter++;
 return(1);

}

int BT_drive(char lport, char rport, int power, int socket_id){
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
 //         the socket-ID for the Lego block
 //
 // Returns: 1 on success
 //          0 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 
 void *p;
 unsigned char *cp;
 char ports;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                  |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

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

 write(socket_id,&cmd_string[0],15);

 message_id_counter++; 
 return(1);

}

int BT_turn(char lport, int lpower, char rport, int rpower, int socket_id){
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
 //         the socket-ID for the Lego block
 //
 // Returns: 1 on success
 //          0 otherwise
 //////////////////////////////////////////////////////////////////////////////////////////////////
 void *p;
 unsigned char *cp;
 unsigned char cmd_string[15]={0x0D,0x00, 0x00,0x00, 0x80,  0x00,0x00,  0xA4,      0x00,    0x00,       0x81,0x00,   0xA6,    0x00,   0x00};
 //                  |length-2| | cnt_id | |type| | header |  |set power| |layer|  |port ids|  |power|      |start|  |layer| |port id|

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

 cmd_string[9]=lport;
 cmd_string[11]=lpower;
 cmd_string[14]=lport;

#ifdef __BT_debug
 fprintf(stderr,"BT_turn command string:\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(socket_id,&cmd_string[0],15);

 message_id_counter++;

 cmd_string[9]=rport;
 cmd_string[11]=rpower;
 cmd_string[14]=rport;

#ifdef __BT_debug
 fprintf(stderr,"BT_turn command string:\n");
 for(int i=0; i<16; i++)
 {
  fprintf(stderr,"%X, ",cmd_string[i]&0xff);
 }
 fprintf(stderr,"\n");
#endif

 write(socket_id,&cmd_string[0],15);

 message_id_counter++;


 return(1);

}
