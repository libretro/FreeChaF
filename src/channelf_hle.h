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
	unsigned char psu1_hle;
	unsigned char psu2_hle;
	unsigned char fast_screen_clear;
	unsigned char screen_clear_row;
	unsigned char screen_clear_pal;
	unsigned char screen_clear_color;
};

extern struct hle_state_s hle_state;

#endif