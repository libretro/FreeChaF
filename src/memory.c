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
static uint8_t *ROM;
static uint32_t ROMSize;
static int is_multicart;
uint8_t MEMORY_Multicart;

int MEMORY_loadSysROM_libretro(const char* path, int address)
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

int MEMORY_loadCartROM(const void* data, size_t size)
{
	const uint16_t address = 0x800;
	int length = size;
	if (size == (1 << 18)) { // Sean Riddle multicart
		length = 0x1800;
		is_multicart = 1;
	} else {
		is_multicart = 0;
	}
	ROM = malloc(size);
	if (!ROM) {
		return 0;
	}

	ROMSize = size;
	memcpy(ROM, data, size);
		
	if (address+length>MEMORY_RAMStart) { MEMORY_RAMStart = address+length; }

	return 1;
}

static uint8_t *translate(uint16_t address)
{
	if (address >= 0x800 && address < 0x2000 && is_multicart) {
		uint32_t mapped = (address - 0x800) | ((MEMORY_Multicart & 0x1f) << 13) | ((MEMORY_Multicart & 0x20) << 7);
		if (mapped < ROMSize)
			return &ROM[mapped];
	}
	if (address >= 0x800 && address < (0x800 + ROMSize)) {
		return &ROM[address - 0x800];
	}
	return &Memory[address];
}

uint8_t MEMORY_read8(uint16_t address)
{
	uint8_t *ta = translate(address);
	return *ta;
}

void MEMORY_write8(uint16_t address, uint8_t val)
{
	if (address == 0x3000 && is_multicart) {
		MEMORY_Multicart = val;
		return;
	}
	if (address < MEMORY_RAMStart) { // Protect ROM
		return;
	}
	*translate(address) = val;
}

uint16_t MEMORY_read16(uint16_t address)
{
	uint8_t *ta = translate(address);
	return (ta[0]<<8) | ta[1];
}

void MEMORY_reset(void)
{
	/* clear memory */
	memset (Memory + MEMORY_RAMStart, 0, MEMORY_SIZE - MEMORY_RAMStart);
	MEMORY_Multicart = 0;
}
