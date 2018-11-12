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

#include <string.h>
#include "osd.h"

unsigned int DisplayWidth = 0;
unsigned int DisplayHeight = 0;
unsigned int DisplayColor[] = {0, 0xFFFFFF};
unsigned int DisplaySize = 0;
unsigned int *Frame;

void OSD_setDisplay(unsigned int frame[], unsigned int width, unsigned int height)
{
	Frame = frame;
	DisplayWidth = width;
	DisplayHeight = height;
	DisplaySize = width*height;
}

void OSD_setColor(unsigned int color)
{
	DisplayColor[1] = color;
}

void OSD_setBackground(unsigned int color)
{
	DisplayColor[0] = color;
}

// Utility functions
void OSD_HLine(int x, int y, int len)
{
	if(x<0 || y<0 || (y*DisplayWidth+x+len)>DisplaySize) { return; }
	
	int offset = (y*DisplayWidth)+x;
	int i;
	for(i=0; i<=len; i++)
	{
		Frame[offset] = DisplayColor[1];
		offset = offset + 1;
	}
}
void OSD_VLine(int x, int y, int len)
{
	if(x<0 || y<0 || ((y+len)*DisplayWidth+x)>DisplaySize) { return; }
	
	int offset = (y*DisplayWidth)+x;
	int i;
	for(i=0; i<=len; i++)
	{
		Frame[offset] = DisplayColor[1];
		offset = offset + DisplayWidth;
	}
}

void OSD_Box(int x1, int y1, int width, int height)
{
	OSD_HLine(x1, y1, width);
	OSD_HLine(x1, y1+height, width);
	OSD_VLine(x1, y1, height);
	OSD_VLine(x1+width, y1, height);
}

void OSD_FillBox(int x1, int y1, int width, int height)
{
	int i;
	for(i=0; i<height; i++)
	{
		OSD_HLine(x1, y1+i, width);
	}
}

int letters[590] = // 32 - 90 59x10
{
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // space
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // !
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // "
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // #
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // $
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // %
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // &
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // '
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // (
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // )
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // *
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // +
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 8,  // ,  
	0, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0, 0,  // -
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0, 0,  // .
	0, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0, 0,  // /
	0, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  // 0
	0, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x3E, 0, 0,  // 1
	0, 0x3C, 0x42, 0x02, 0x04, 0x18, 0x20, 0x7E, 0, 0,  // 2
	0, 0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x3C, 0, 0,  // 3
	0, 0x40, 0x44, 0x44, 0x7E, 0x04, 0x04, 0x04, 0, 0,  // 4
	0, 0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x7C, 0, 0,  // 5
	0, 0x3E, 0x40, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0, 0,  // 6
	0, 0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0, 0,  // 7
	0, 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0, 0,  // 8
	0, 0x3C, 0x42, 0x42, 0x3E, 0x02, 0x02, 0x3C, 0, 0,  // 9
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // :
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // ;
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // <
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // =
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // >
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // ?
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  // @
	0, 0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0, 0,  // A
	0, 0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0, 0,  // B
	0, 0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0, 0,  // C
	0, 0x7C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7C, 0, 0,  // D
	0, 0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0, 0,  // E
	0, 0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40, 0, 0,  // F
	0, 0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C, 0, 0,  // G
	0, 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0, 0,  // H
	0, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7C, 0, 0,  // I
	0, 0x7E, 0x04, 0x04, 0x04, 0x04, 0x44, 0x38, 0, 0,  // J
	0, 0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0, 0,  // K
	0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0, 0,  // L
	0, 0x42, 0x66, 0x5A, 0x42, 0x42, 0x42, 0x42, 0, 0,  // M
	0, 0x42, 0x62, 0x52, 0x52, 0x4A, 0x46, 0x42, 0, 0,  // N
	0, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  // O
	0, 0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40, 0, 0,  // P
	0, 0x3C, 0x42, 0x42, 0x42, 0x4A, 0x46, 0x3D, 0, 0,  // Q
	0, 0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x42, 0, 0,  // R
	0, 0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C, 0, 0,  // S
	0, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0, 0,  // T
	0, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  // U
	0, 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0, 0,  // V
	0, 0x42, 0x42, 0x42, 0x42, 0x5A, 0x66, 0x42, 0, 0,  // W
	0, 0x42, 0x42, 0x24, 0x18, 0x24, 0x42, 0x42, 0, 0,  // X
	0, 0x42, 0x42, 0x24, 0x18, 0x08, 0x08, 0x08, 0, 0,  // Y
	0, 0x7E, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7E, 0, 0  // Z
};

void OSD_drawLetter(int x, int y, int c)
{
	unsigned int t = DisplayColor[0];
	int i, j;
	int offset = (DisplayWidth*y)+x;
	c = (c-32) * 10;

	for(i=0; i<10; i++)
	{
		for(j=0; j<8; j++)
		{
			DisplayColor[0] = Frame[offset+j];
			Frame[offset+j] = DisplayColor[((letters[c]>>(7-j))&0x01)];
		}
		offset+=DisplayWidth;
		c++;
	}
	DisplayColor[0] = t;
}

void OSD_drawText(int x, int y, const char *text)
{
	int len = strlen(text);
	int i = 0;
	int c = 32;
	while(i<len)
	{
		c = text[i];
		if(c<32) { break; }
		if(c>90) { c = 32; }
		i++;
		OSD_drawLetter(x, y, c);
		x+=8;
	}
}

void OSD_drawTextBoxed(int x, int y, const char *text)
{
	unsigned int t1 = DisplayColor[1];

	int len = (strlen(text)*8)+1;

	DisplayColor[1] = DisplayColor[0];
	OSD_FillBox(x, y, len, 10);

	DisplayColor[1] = t1;
	OSD_Box(x, y, len, 10);

	OSD_drawText(x+1, y+1, text);
}

void OSD_drawTextCenterBoxed(int y, const char *text)
{
	int len = (strlen(text)*8)+1;
	int x = (DisplayWidth-len) / 2;

	if(x>=0)
	{
		OSD_drawTextBoxed(x, y, text);
	}
}

// ChannelF 

// controller swaps
void OSD_drawP1P2()
{
	OSD_drawTextBoxed(DisplayWidth-17, DisplayHeight-13, "P2P1");
}

void OSD_drawP2P1()
{
	OSD_drawTextBoxed(DisplayWidth-17, DisplayHeight-13, "P1P2");
}

// console buttons
void OSD_drawConsole(int pos, int down)
{
	unsigned int t0 = DisplayColor[0];
	unsigned int t1 = DisplayColor[1];

	int x = (DisplayWidth-98)/2;
	int y = (DisplayHeight-50);

	DisplayColor[1] = 0x000000;
	OSD_FillBox(x, y, 98, 21);
	DisplayColor[1] = 0xFFFFFF;
	OSD_Box(x, y, 98, 21);

	x += 3;
	y += 3;
	DisplayColor[1] = 0xFFFF00;
	OSD_FillBox(x, y, 16, 16);
	DisplayColor[1] = 0x000000;
	OSD_drawLetter(x+4, y+4, 'R');
	
	int i;
	for(i=0; i<4; i++)
	{
		x+=19;
		DisplayColor[1] = 0xCCCCCC;
		OSD_FillBox(x, y, 16, 16);
		DisplayColor[1] = 0x000000;
		OSD_drawLetter(x+4, y+4, 48+(i+1));
	}
	
	// draw cursor
	x = x-76;
	DisplayColor[1] = 0x00FF00;
	OSD_Box(x+(19*pos)-1, y-1, 17, 17);
	if(down)
	{
		OSD_Box(x+(19*pos), y, 15, 15);
	}

	DisplayColor[0] = 0x000000;
	DisplayColor[1] = 0xFFFFFF;

	switch(pos)
	{
		case 0: OSD_drawTextCenterBoxed(DisplayHeight-26, "RESET"); break;
		case 1:
			OSD_drawTextCenterBoxed(DisplayHeight-26, "TIME");
			OSD_drawTextCenterBoxed(DisplayHeight-16, "2 MIN / HOCKEY");
		break;
		case 2:
			OSD_drawTextCenterBoxed(DisplayHeight-26, "MODE");
			OSD_drawTextCenterBoxed(DisplayHeight-16, "5 MIN / TENNIS");
		break;
		case 3:
			OSD_drawTextCenterBoxed(DisplayHeight-26, "HOLD");
			OSD_drawTextCenterBoxed(DisplayHeight-16, "10 MIN / GAME 3");
		break;
		case 4:
			OSD_drawTextCenterBoxed(DisplayHeight-26, "START");
			OSD_drawTextCenterBoxed(DisplayHeight-16, "20 MIN / GAME 4");
		break;
	}

	DisplayColor[0] = t0;
	DisplayColor[1] = t1;
}