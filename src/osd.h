#ifndef OSD_H
#define OSD_H
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

// On-Screen Display //
#include "video.h"

void OSD_setDisplay(pixel_t frame[], unsigned int width, unsigned int height);

void OSD_setColor(pixel_t color);

void OSD_setBackground(pixel_t color);

void OSD_HLine(int x, int y, int len);

void OSD_VLine(int x, int y, int len);

void OSD_Box(int x1, int y1, int width, int height);

void OSD_FillBox(int x1, int y1, int width, int height);

void OSD_drawLetter(int x, int y, int c);

void OSD_drawText(int x, int y, const char *text);

void OSD_drawTextBoxed(int x, int y, const char *text);

void OSD_drawTextCenterBoxed(int y, const char *text);

void OSD_drawPaused(void);

void OSD_drawP1P2(void);

void OSD_drawP2P1(void);

void OSD_drawConsole(int pos, int down);

#endif
