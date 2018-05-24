// Initial testing of a bare-bones BlueTooth communication
// library for the EV3 - Thank you Lego for changing everything
// from the NXT to the EV3!

#include "btcomm.h"

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
 	#define HEXKEY "00:16:53:56:55:D9"	// <--- SET UP YOUR EV3's HEX ID here
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

 // name must not contain spaces or special characters
 // max name length is 12 characters
 BT_setEV3name("Ghost_Buster",EV3_socket);

 BT_play_tone_sequence(tone_data,EV3_socket);

 BT_motor_port_start(MOTOR_D,20,EV3_socket);
 BT_motor_port_start(MOTOR_C,-20,EV3_socket);
 fgets(&reply[0], 1020, stdin);
 //BT_motor_port_stop(MOTOR_C|MOTOR_D,1,EV3_socket);
 BT_all_stop(0, EV3_socket); 

 BT_close(EV3_socket);
 fprintf(stderr,"Done!\n"); 
}
