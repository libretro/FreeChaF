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

#include "controller.h"
#include "ports.h"
#include "channelf.h"

#include <stdio.h>

uint8_t CONTROLLER_State[] = {0,0,0};
unsigned char ControllerEnabled = 0;

#define Console 0
#define ControlA 1
#define ControlB 2

static const uint8_t ConsolePort = 0;
#define ControlAPort (ControllerSwapped ? 4 : 1)
#define ControlBPort (ControllerSwapped ? 1 : 4)
unsigned char ControllerSwapped = 0;

void setButton(int control, int button, int pressed)
{
   /*
   // Console 
   0 // TIME
   1 // MODE
   2 // HOLD
   3 // START

   // Controller
   0 // Right
   1 // Left
   2 // Back
   3 // Forward
   4 // rot left
   5 // rot right
   6 // pull
   7 // push
   */

   if(pressed)
      CONTROLLER_State[control] |= 1<<button;
   else
      CONTROLLER_State[control] &= (1<<button)^0xFF;
}

void CONTROLLER_setInput(int control, int state)
{
	if(control>=0 && control<=2)
		CONTROLLER_State[control] = state;
}

void CONTROLLER_swap(void)
{
	ControllerSwapped ^= 1;
}

int CONTROLLER_swapped(void)
{
	return ControllerSwapped;
}

int CONTROLLER_portRead(uint8_t port)
{
	if(port==ConsolePort)
		 return (CONTROLLER_State[Console]^0xFF) & 0x0F;
	if(ControllerEnabled)
   {
      if(port==ControlAPort)
         return(CONTROLLER_State[ControlA]^0xFF);
      if(port==ControlBPort)
         return(CONTROLLER_State[ControlB]^0xFF);
   }
	return 0;
}

void CONTROLLER_portReceive(uint8_t port, uint8_t val)
{
	if(port==ConsolePort) // Console
		ControllerEnabled = (val&0x40)==0;
}

/* Console buttons */

int cursorX    = 4; /* initial cursor setting 'Start'  */
int cursorDown = 0;

void CONTROLLER_consoleInput(int action, int pressed)
{
	switch(action)
	{
		case 0:
         if(pressed)
            cursorX--;
         break;
		case 1:
         if(pressed)
            cursorX++;
         break;
		case 2:
			cursorDown = pressed;
			if(cursorX==0)
         {
            if(pressed)
               CHANNELF_reset();
         }
			else
				setButton(0, cursorX-1, pressed);
			break;
	}

	if (cursorX<0)
      cursorX = 4;
	if (cursorX>4)
      cursorX = 0;
}

int CONTROLLER_cursorPos(void)
{
	return cursorX;
}
int CONTROLLER_cursorDown(void)
{
	return cursorDown;
}
