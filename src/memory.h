#ifndef MEMORY_H
#define MEMORY_H
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

int MEMORY_RAMStart;

unsigned int Memory[0x10000];
// sl131253 - 0x000 - 0x3FF
// sl131254 - 0x400 - 0x7FF
// cart     - 0x800 - 0x1FFF
// vram     - 0x2000 ...

void MEMORY_reset(void);

#endif
