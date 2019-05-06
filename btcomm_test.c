/* EV3 API
 *  Copyright (C) 2018-2019 Francisco Estrada and Lioudmila Tishkina
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Initial testing of a bare-bones BlueTooth communication
// library for the EV3 - Thank you Lego for changing everything
// from the NXT to the EV3!

#include "btcomm.h"

int main(int argc, char *argv[]) {
  char test_msg[8] = {0x06, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x01};
  char reply[1024];
  int tone_data[50][3];

  // Reset tone data information
  for (int i = 0; i < 50; i++) {
    tone_data[i][0] = -1;
    tone_data[i][1] = -1;
    tone_data[i][2] = -1;
  }

  tone_data[0][0] = 262;
  tone_data[0][1] = 250;
  tone_data[0][2] = 1;
  tone_data[1][0] = 330;
  tone_data[1][1] = 250;
  tone_data[1][2] = 25;
  tone_data[2][0] = 392;
  tone_data[2][1] = 250;
  tone_data[2][2] = 50;
  tone_data[3][0] = 523;
  tone_data[3][1] = 250;
  tone_data[3][2] = 63;

  memset(&reply[0], 0, 1024);

// just uncomment your bot's hex key to compile for your bot, and comment the
// other ones out.
#ifndef HEXKEY
#define HEXKEY "00:16:53:56:55:D9"  // <--- SET UP YOUR EV3's HEX ID here
#endif

  BT_open(HEXKEY);

  // name must not contain spaces or special characters
  // max name length is 12 characters
  BT_setEV3name("R2D2");

  BT_play_tone_sequence(tone_data);

  BT_close();
  fprintf(stderr, "Done!\n");
}
