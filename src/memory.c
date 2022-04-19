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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <streams/file_stream.h>
#include "memory.h"

int MEMORY_RAMStart;
uint8_t Memory[MEMORY_SIZE];

int CHANNELF_loadROM_libretro(const char* path, int address)
{
	RFILE *h = filestream_open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
	ssize_t size;
	if (!h) // problem loading file
	{
		return 0;
	}

	size = filestream_get_size(h);
	if (size <= 0) // problem loading file
	{
		return 0;
	}
	if (size > MEMORY_SIZE - address) // if too large to fit in memory...
	{
		size = MEMORY_SIZE - address;
	}

	size = filestream_read(h, Memory + address, size);
	filestream_close(h);
	if (size <= 0) // problem reading file
	{
		return 0;
	}

	if (address+size>MEMORY_RAMStart) 
	{
		MEMORY_RAMStart = address+size;
	}

	return 1;
}

int CHANNELF_loadROM_mem(const uint8_t* data, int size, int address)
{
	int length = size;
	if (length > 0x10000 - address)
	{
		length = 0x10000 - address;
	}
	memcpy(Memory + address, data, length);
		
	if (address+length>MEMORY_RAMStart) { MEMORY_RAMStart = address+length; }

	return 1;
}

void MEMORY_reset(void)
{
	/* clear memory */
	memset (Memory + MEMORY_RAMStart, 0, MEMORY_SIZE - MEMORY_RAMStart);
}
