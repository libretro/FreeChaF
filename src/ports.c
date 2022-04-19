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

#include "ports.h"
#include "f2102.h"
#include "audio.h"
#include "video.h"
#include "controller.h"

uint8_t Ports[64];

// Read state of port
uint8_t PORTS_read(uint8_t port)
{
	return Ports[port] | CONTROLLER_portRead(port); // controllers don't latch?
}

// Write data to port
void PORTS_write(uint8_t port, uint8_t val)
{
	Ports[port] = val;
}

void PORTS_notify(uint8_t port, uint8_t val)
{
	Ports[port] = val;

	F2102_portReceive(port, val);
	AUDIO_portReceive(port, val);
	VIDEO_portReceive(port, val);
	CONTROLLER_portReceive(port, val);
}

void PORTS_reset(void)
{
	memset(Ports, 0, sizeof(Ports));
}
