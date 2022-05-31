#ifndef CHANNELF_HLE_H
#define CHANNELF_HLE_H

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

void CHANNELF_HLE_run(void);

void unsupported_hle_function(void);

struct hle_state_s
{
	uint8_t psu1_hle;
	uint8_t psu2_hle;
	uint8_t fast_screen_clear;
	uint8_t screen_clear_row;
	uint8_t screen_clear_pal;
	uint8_t screen_clear_color;
	uint8_t delay_counter;
};

extern struct hle_state_s hle_state;

#endif
