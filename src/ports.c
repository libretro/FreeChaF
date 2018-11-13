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

#include "ports.h"
#include "f2102.h"
#include "audio.h"
#include "video.h"
#include "controller.h"

// Read state of port
int PORTS_read(int port)
{
	return Ports[port] | CONTROLLER_portRead(port); // controllers don't latch?
}

// Write data to port
void PORTS_write(int port, int val)
{
	Ports[port] = val & 0xFF;
}

void PORTS_notify(int port, int val)
{
	Ports[port] = val & 0xFF;

	F2102_portReceive(port, val);
	AUDIO_portReceive(port, val);
	VIDEO_portReceive(port, val);
	CONTROLLER_portReceive(port, val);
}

void PORTS_reset()
{
	int i;
	for(i=0; i<64; i++)
	{
		Ports[i] = 0;
	}
}