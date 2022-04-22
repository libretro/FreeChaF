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
#include "channelf_hle.h"

#define TICKS_PER_ROW 18606

// Old MSVC has no snprintf
#if defined(_MSC_VER) && _MSC_VER < 1900
#define any_snprintf _snprintf
#else
#define any_snprintf snprintf
#endif

void unsupported_hle_function(void)
{
	char formatted[1024];
	struct retro_message msg;
	memset(formatted, 0, sizeof(formatted));
	any_snprintf(formatted, 1000, "Unsupported HLE function: 0x%x\n", F8_PC0);
	log_cb(RETRO_LOG_ERROR, formatted);
	msg.msg    = formatted;
	msg.frames = 600;
	Environ(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	Environ(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

static void hle_clear_row(int row)
{
	memset(VIDEO_Buffer_raw + (row << 7), hle_state.screen_clear_color, 125);
	VIDEO_Buffer_raw[(row << 7) + 125] = 0;
	VIDEO_Buffer_raw[(row << 7) + 126] = hle_state.screen_clear_pal;
	VIDEO_Buffer_raw[(row << 7) + 127] = 0;
}

static int CHANNELF_HLE(void)
{
	if (hle_state.screen_clear_row)
	{
		hle_clear_row(hle_state.screen_clear_row++);
		if (hle_state.screen_clear_row == 64)
		{
			hle_state.screen_clear_row = 0;
		}
		return TICKS_PER_ROW;
	}
	switch (F8_PC0)
	{
	case 0x0: // init
		memset (F8_R, 0, sizeof(F8_R));
		if (MEMORY_read8(0x800) == 0x55)
		{
			F8_A = 0x55;
			F8_DC0 = 0x801;
			F8_PC0 = 0x802;
			F8_R[0x3b] = 0x28;
			F8_ISAR = 0x3b;
			return 1459;
		}

		unsupported_hle_function();
		return 14914;
	case 0x8f: // delay
	{
		int ticks = 2563 * F8_R[5];
		F8_R[5] = 0;
		F8_R[6] = 0;
		F8_A = 0xff;
		F8_PC0 = F8_PC1;
		return ticks;
	}
	case 0xd0: // screen clear
	{
		// guesswork
		switch (F8_R[3])
		{
		case 0xd0:
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

		F8_PC0 = F8_PC1;

		if (hle_state.fast_screen_clear)
		{
			int row;
			for(row=0; row<64; row++)
				hle_clear_row(row);
			return TICKS_PER_FRAME;
		}

		hle_clear_row(0);
		hle_state.screen_clear_row = 1;
		
		return TICKS_PER_ROW;
	}
	case 0x107: // pushk
	{
		int tisar = F8_R[0x3b];
		F8_R[tisar & 0x3f] = F8_R[12];
		F8_R[(tisar + 1) & 0x3f] = F8_R[13];
		F8_R[0x3b] = (tisar + 2) & 0x3f;

		// Simulate clobbering
		F8_A = F8_ISAR;
		F8_R[7] = F8_ISAR;

		// Return
		F8_PC0 = F8_PC1;
		return 48;
	}
	case 0x11e: // popk
	{
		int tisar = F8_R[0x3b];
		F8_R[13] = F8_R[(tisar - 1) & 0x3f];
		F8_R[12] = F8_R[(tisar - 2) & 0x3f];
		F8_R[0x3b] = (tisar - 2) & 0x3f;

		// Simulate clobbering
		F8_A = F8_ISAR;
		F8_R[7] = F8_ISAR;

		// Return
		F8_PC0 = F8_PC1;
		return 50;
	}
	default:
		unsupported_hle_function();
		return TICKS_PER_FRAME;
	}
}

static int is_hle(void)
{
	if (hle_state.screen_clear_row)
		return 1;

	if (F8_PC0 < 0x400 && hle_state.psu1_hle)
		return 1;

	if (F8_PC0 >= 0x400 && F8_PC0 < 0x800 && hle_state.psu2_hle)
	{
		return 1;
	}

	if (F8_PC0 == 0xd0 && hle_state.fast_screen_clear && (F8_R[3] == 0xc6 || F8_R[3] == 0x21 || F8_R[3] == 0xd0))
	{
		return 1;
	}

	return 0;
}

void CHANNELF_HLE_run(void) // run for one frame
{
	int tick  = 0;
	int ticks = CPU_Ticks_Debt;

	while(ticks<TICKS_PER_FRAME)
	{
		if (is_hle())
			tick = CHANNELF_HLE();
		else
			tick = F8_exec();
		ticks+=tick;
		AUDIO_tick(tick);
	}

	CPU_Ticks_Debt = ticks - TICKS_PER_FRAME;
}
