/*
	This file is part of FreeChaF.

	FreeChaF is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FreeChaF is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeChaF.  If not, see http://www.gnu.org/licenses/
*/

#include "video.h"

//int VIDEO_Buffer[8192]; // 128x64
unsigned int buffer[8192]; // 128x64
unsigned int colors[8] = {0x101010, 0xFDFDFD, 0x5331FF, 0x5DCC02, 0xF33F4B, 0xE0E0E0, 0xA6FF91, 0xD0CEFF};
unsigned int palette[16] = {0,1,1,1, 7,2,4,3, 6,2,4,3, 5,2,4,3}; // bk wh wh wh, bl B G R, gr B G R, gy B G R...

int ARM = 0;
int X = 0;
int Y = 0;
int Color = 2; 

void VIDEO_drawFrame()
{
	int row;
	int col;
	int color;
	int pal;

	for(row=0; row<64; row++)
	{
		// The last three columns in the video buffer are special.
		// 127 - unknown
		// 126 - bit 1 = palette bit 1
		// 125 - bit 1 = palette bit 0 (or with 126 bit 0)
		// (palette is shifted by two and added to 'color'
		//  to find palette index which holds the color's index)
		
		pal = ((buffer[(row<<7)+125]&2)>>1) | (buffer[(row<<7)+126]&3);
		pal = (pal<<2) & 0xC;
		
		for(col=0; col<128; col++)
		{
			color = (buffer[(row<<7)+col]) & 0x3;
			VIDEO_Buffer[(row<<7)+col] = colors[palette[pal|color]&0x7];
		}
	}

}

void VIDEO_portReceive(int port, int val)
{
	switch(port)
	{
		case 0: // ARM 
			val &= 0x60;
			if(val==0x40 && ARM==0x60) // Strobed
			{
				// Write to display buffer
				buffer[(Y<<7)+X] = Color;
			}
			ARM = val;
		break;

		case 1: // Set Color (bits 6 and 7) 
			Color = ((val ^ 0xFF)>>6)&3;
			break;
		case 4: // X coordinate, inverted (bits 0-6)
			X = (val ^ 0xFF) & 0x7F;
			break;
		case 5: // Y coordinate, inverted (bits 0-5)
			Y = (val ^ 0xFF) & 0x3F;
			break;
	}
}