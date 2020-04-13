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
#include "libretro.h"
#include "channelf.h"
#include "memory.h"
#include "f8.h"
#include "audio.h"
#include "f2102.h"
#include "ports.h"
#include "video.h"

retro_log_printf_t channelf_log;

int CPU_Ticks_Debt = 0;

int CHANNELF_loadROM(const char* path, int address)
{
	unsigned char word;
	FILE *fp;
	if((fp = fopen(path,"rb"))!=NULL)
	{
		while(fread(&word,sizeof(word),1,fp) && address<0x10000)
		{
			Memory[address] = word;
			address++;
		}
		
		if (address>MEMORY_RAMStart) { MEMORY_RAMStart = address; }

		fclose(fp);
		return 1;	
	}
   return 0;
}

int CHANNELF_loadROM_mem(const unsigned char* data, int size, int address)
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

void CHANNELF_run(void) // run for one frame
{
	int tick  = 0;
	int ticks = CPU_Ticks_Debt;

	while(ticks<TICKS_PER_FRAME)
	{
		tick = F8_exec();
		ticks+=tick;
		AUDIO_tick(tick);
	}

	CPU_Ticks_Debt = ticks - TICKS_PER_FRAME;
}

void CHANNELF_init(void)
{
	F8_init();
	CHANNELF_reset();
}

void CHANNELF_reset(void)
{
	CPU_Ticks_Debt = 0;
	MEMORY_reset();
	F2102_reset();
	F8_reset();
	AUDIO_reset();
	PORTS_reset();
}
