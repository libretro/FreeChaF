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
#include "channelf.h"
#include "memory.h"
#include "f8.h"
#include "audio.h"
#include "f2102.h"
#include "ports.h"
#include "video.h"

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

int CHANNELF_loadROM_mem(const unsigned char* data, int sz, int address)
{
	int l = sz;
	if (l > 0x10000 - address)
		l = 0x10000 - address;
	memcpy(Memory + address, data, l);
		
	if (address+l>MEMORY_RAMStart) { MEMORY_RAMStart = address+l; }

	return 1;
}

static void hle_clear_row(int row) {
	memset(VIDEO_Buffer_raw + (row << 7), hle_state.screen_clear_color, 125);
	VIDEO_Buffer_raw[(row << 7) + 125] = 0;
	VIDEO_Buffer_raw[(row << 7) + 126] = hle_state.screen_clear_pal;
	VIDEO_Buffer_raw[(row << 7) + 127] = 0;

}

#define TICKS_PER_ROW 18606

static int CHANNELF_HLE(void)
{
	if (hle_state.screen_clear_row) {
		hle_clear_row(hle_state.screen_clear_row++);
		if (hle_state.screen_clear_row == 64)
			hle_state.screen_clear_row = 0;
		return TICKS_PER_ROW;
	}
	switch (PC0) {
	case 0x0: // init
		memset (R, 0, sizeof(R));
		if (Memory[0x800] == 0x55) {
			A = 0x55;
			DC0 = 0x801;
			PC0 = 0x802;
			R[0x3b] = 0x28;
			ISAR = 0x3b;
			return 1459;
		}

		unsupported_hle_function ();
		return 14914;
	case 0x8f: // delay
	{
		int ticks = 2563 * R[5];
		R[5] = 0;
		R[6] = 0;
		A = 0xff;
		PC0 = PC1;
		return ticks;
	}
	case 0xd0: // screen clear
	{
		// guesswork
		switch (R[3]) {
		case 0xc6:
			hle_state.screen_clear_pal = 3;
			hle_state.screen_clear_color = 0;
			break;
		case 0x21:
			hle_state.screen_clear_pal = 0;
			hle_state.screen_clear_color = 0;
			break;
		default:
			unsupported_hle_function ();
			return TICKS_PER_FRAME;
		}

		PC0 = PC1;

		if (hle_state.fast_screen_clear) {
			int row;
			for(row=0; row<64; row++)
				hle_clear_row(row);
			return TICKS_PER_FRAME;
		}

		hle_clear_row(0);
		hle_state.screen_clear_row = 1;
		
		return TICKS_PER_ROW;
	}
	default:
		unsupported_hle_function ();
		return TICKS_PER_FRAME;
	}
}

static int is_hle(void) {
	if (hle_state.screen_clear_row)
		return 1;

	if (PC0 < 0x400 && hle_state.psu1_hle)
		return 1;

	if (PC0 >= 0x400 && PC0 < 0x800 && hle_state.psu2_hle) {
		return 1;
	}

	if (PC0 == 0xd0 && hle_state.fast_screen_clear
	    && (R[3] == 0xc6 || R[3] == 0x21))
		return 1;

	return 0;
}

struct hle_state_s hle_state;
int cpu_ticks_debt;

void CHANNELF_run(void) // run for one frame
{
	int tick  = 0;
	int ticks = cpu_ticks_debt;

	while(ticks<TICKS_PER_FRAME)
	{
		if (is_hle()) {
			tick = CHANNELF_HLE();
		} else
			tick = F8_exec();

		ticks+=tick;
		AUDIO_tick(tick);
	}

	cpu_ticks_debt = ticks - TICKS_PER_FRAME;
}

void CHANNELF_init(void)
{
	F8_init();
	CHANNELF_reset();
}

void CHANNELF_reset(void)
{
	MEMORY_reset();
	F2102_reset();
	F8_reset();
	AUDIO_reset();
	PORTS_reset();
}
