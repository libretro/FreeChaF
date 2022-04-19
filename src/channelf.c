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
#include "libretro.h"
#include "channelf.h"
#include "memory.h"
#include "f8.h"
#include "audio.h"
#include "f2102.h"
#include "ports.h"
#include "video.h"

int CPU_Ticks_Debt = 0;

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
