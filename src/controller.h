#ifndef CONTROLLER_H
#define CONTROLLER_H
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

void CONTROLLER_portReceive(int port, unsigned char val);

int CONTROLLER_portRead(int port);

void CONTROLLER_setInput(int control, int state);

void CONTROLLER_swap(void);

void CONTROLLER_consoleInput(int control, int state);

int CONTROLLER_cursorPos(void);

int CONTROLLER_cursorDown(void);

int CONTROLLER_swapped(void);

extern unsigned char ControllerEnabled;
extern unsigned char ControllerSwapped;
extern int cursorX;
extern int cursorDown;
extern uint8_t CONTROLLER_State[3];

#endif
