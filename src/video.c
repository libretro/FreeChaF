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

pixel_t VIDEO_Buffer_rgb[8192]; // 128x64
uint8_t VIDEO_Buffer_raw[8192]; // 128x64
static const pixel_t colors[8] =
  {
	  vRGB(0x10, 0x10, 0x10),
	  vRGB(0xFD, 0xFD, 0xFD),
	  vRGB(0x53, 0x31, 0xFF),
	  vRGB(0x5D, 0xCC, 0x02),
	  vRGB(0xF3, 0x3F, 0x4B),
	  vRGB(0xE0, 0xE0, 0xE0),
	  vRGB(0xA6, 0xFF, 0x91),
	  vRGB(0xD0, 0xCE, 0xFF)
  };
static const uint8_t palette[16] = {0,1,1,1, 7,2,4,3, 6,2,4,3, 5,2,4,3}; // bk wh wh wh, bl B G R, gr B G R, gy B G R...

uint8_t VIDEO_ARM = 0;
uint8_t VIDEO_X = 0;
uint8_t VIDEO_Y = 0;
uint8_t VIDEO_Color = 2; 

void VIDEO_drawFrame(void)
{
	int row;
	int col;

	for(row=0; row<64; row++)
	{
		// The last three columns in the video buffer are special.
		// 127 - unknown
		// 126 - bit 1 = palette bit 1
		// 125 - bit 1 = palette bit 0 (or with 126 bit 0)
		// (palette is shifted by two and added to 'color'
		//  to find palette index which holds the color's index)
		
		uint8_t pal = ((VIDEO_Buffer_raw[(row<<7)+125]&2)>>1) | (VIDEO_Buffer_raw[(row<<7)+126]&3);
		pal = (pal<<2) & 0xC;
		
		for(col=0; col<128; col++)
		{
			uint8_t color = (VIDEO_Buffer_raw[(row<<7)+col]) & 0x3;
			VIDEO_Buffer_rgb[(row<<7)+col] = colors[palette[pal|color]&0x7];
		}
	}

}

void VIDEO_portReceive(uint8_t port, uint8_t val)
{
	switch(port)
	{
		case 0: // ARM 
			val &= 0x60;
			if(val==0x40 && VIDEO_ARM==0x60) // Strobed
			{
				// Write to display buffer
				VIDEO_Buffer_raw[(VIDEO_Y<<7)+VIDEO_X] = VIDEO_Color;
			}
			VIDEO_ARM = val;
		break;

		case 1: // Set Color (bits 6 and 7) 
			VIDEO_Color = ((val ^ 0xFF)>>6)&3;
			break;
		case 4: // X coordinate, inverted (bits 0-6)
			VIDEO_X = (val ^ 0xFF) & 0x7F;
			break;
		case 5: // Y coordinate, inverted (bits 0-5)
			VIDEO_Y = (val ^ 0xFF) & 0x3F;
			break;
	}
}
