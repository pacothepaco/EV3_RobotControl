// Initial testing of a bare-bones BlueTooth communication
// library for the EV3 - Thank you Lego for changing everything
// from the NXT to the EV3!

#include "btcomm.h"

int main(int argc, char *argv[])
{
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

 BT_open(HEXKEY);

 fprintf(stderr,"Sending message:\n");
 for (int i=0; i<8; i++)
   fprintf(stderr,"%x ",test_msg[i]);
 fprintf(stderr,"\n");

 //write(&test_msg[0],8);

 //read(&reply[0],5);
 //fprintf(stderr,"Reply message:\n");
 //for (int i=0; i<5; i++)
 //  fprintf(stderr,"%x ",reply[i]);
 //fprintf(stderr,"\n");

 // name must not contain spaces or special characters
 // max name length is 12 characters
 //BT_setEV3name("Legally_Pink");

 //BT_play_tone_sequence(tone_data);

 /*int readings[2];
 BT_clear_gyro_sensor(PORT_2);
 BT_read_gyro_sensor(PORT_2, readings);
 fgets(&reply[0], 1020, stdin);

 BT_motor_port_start(MOTOR_D,30);
 BT_motor_port_start(MOTOR_C,-30);
 fgets(&reply[0], 1020, stdin);

 BT_all_stop(1); 
 BT_read_gyro_sensor(PORT_2, readings);*/

 //BT_drive(MOTOR_C, MOTOR_D, -30);
 //fgets(&reply[0], 1020, stdin);
 //BT_all_stop(1);

 //BT_turn(MOTOR_C, 40, MOTOR_D, -30);
 fgets(&reply[0], 1020, stdin);
 //BT_all_stop(1);

 //int touch = BT_read_touch_sensor(PORT_3);
 //fprintf(stderr, "Touch sensor value: %d\n", touch);

 /*int RGB_array[3];
 int color=0;
 color=BT_read_colour_sensor(PORT_4);
 fprintf(stderr, "Color: %d\n", color);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);
 fgets(&reply[0], 1020, stdin);*/
/*
 color=BT_read_colour_sensor(PORT_4);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "Color: %d\n", color);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);

 fgets(&reply[0], 1020, stdin);

 color=BT_read_colour_sensor(PORT_4);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "Color: %d\n", color);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);
 fgets(&reply[0], 1020, stdin);

 color=BT_read_colour_sensor(PORT_4);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "Color: %d\n", color);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);
 fgets(&reply[0], 1020, stdin);

 color=BT_read_colour_sensor(PORT_4);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "Color: %d\n", color);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);
 fgets(&reply[0], 1020, stdin);

 color=BT_read_colour_sensor(PORT_4);
 fprintf(stderr, "Color: %d\n", color);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);
 fgets(&reply[0], 1020, stdin);*/

 /*int distance=BT_read_ultrasonic_sensor(PORT_1);
 fprintf(stderr, "distance: %d\n", distance);
 fgets(&reply[0], 1020, stdin);*/

 /*int gyro_readings[2];
 BT_read_gyro_sensor(PORT_2, gyro_readings);

 BT_motor_port_start(MOTOR_D,-30);
 BT_motor_port_start(MOTOR_C,30);
 fgets(&reply[0], 1020, stdin);
 BT_read_gyro_sensor(PORT_2, readings);
 BT_all_stop(1);
 fgets(&reply[0], 1020, stdin);

 color=BT_read_colour_sensor(PORT_4);
 BT_read_colour_sensor_RGB(PORT_4, RGB_array);
 fprintf(stderr, "Color: %d\n", color);
 fprintf(stderr, "R: %d, G: %d, B:%d\n", RGB_array[0], RGB_array[1], RGB_array[2]);

 fgets(&reply[0], 1020, stdin);

 BT_motor_port_start(MOTOR_D,30);
 BT_motor_port_start(MOTOR_C,-30);
 fgets(&reply[0], 1020, stdin);
 BT_read_gyro_sensor(PORT_2, readings);

 BT_all_stop(1);
 fgets(&reply[0], 1020, stdin);

 BT_motor_port_start(MOTOR_D,30);
 BT_motor_port_start(MOTOR_C,-30);
 fgets(&reply[0], 1020, stdin);
 BT_read_gyro_sensor(PORT_2, readings);
 BT_all_stop(0);
 fgets(&reply[0], 1020, stdin);*/

 BT_timed_motor_port_start(MOTOR_B, -20, 4000);
 //fgets(&reply[0], 1020, stdin);

 BT_motor_port_stop(MOTOR_B, 0);
 fgets(&reply[0], 1020, stdin);


 BT_close();
 fprintf(stderr,"Done!\n"); 
}
