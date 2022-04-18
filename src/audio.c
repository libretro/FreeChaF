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
#include "audio.h"

#include <string.h>

#include "sintable.h"

short AUDIO_Buffer[735 * 2];

unsigned char tone = 0; // current tone

static const int samplesPerFrame = 735; // sampleRate / framesPerSecond

int sampleInCycle = 0; // time since start of tone, resets to 0 after every full cycle
#define FULL_AMPLITUDE 16384
short amp = FULL_AMPLITUDE; // tone amplitude (16384 = full)
static const float decay = 0.998; // multiplier for amp per sample

short ticks = 0; // unprocessed ticks in 1/100 of tick
static int sample = 0; // current sample buffer position

void AUDIO_portReceive(int port, unsigned char val)
{
	if(port==5)
	{
		// Set Tone, bits 6 & 7: 
		// 0 - Silence
		// 1 - 1000hz
		// 2 - 500hz
		// 3 - 120hz
		
		val = (val&0xC0)>>6;
		if(val!=tone)
		{
			tone = val;
			amp = FULL_AMPLITUDE;
			sampleInCycle=0;
		}
	}
}

void AUDIO_tick(int dt) // dt = ticks elapsed since last call
{
	// an audio frame lasts ~14914 ticks
	// at 44.1khz, there are 735 samples per frame
	// ~20.29 ticks per sample (14913.15 ticks/frame)
	
	ticks += dt * 100;

	while(ticks>2029)
	{
		ticks-=2029;
		
		AUDIO_Buffer[sample] = 0;
		if(sample<samplesPerFrame) // output sample
		{
			int toneOutput = 0;
			int res;
			// sintable is a 20Hz tone, we need to speed it up to 1000, 500, 120 or 240 Hz
			switch (tone) {
			case 1:
				toneOutput = 2 * sintable[(sampleInCycle * 50) % SINSAMPLES];
				break;
			case 2:
				toneOutput = 2 * sintable[(sampleInCycle * 25) % SINSAMPLES];
				break;
			case 3:
				toneOutput = sintable[(sampleInCycle * 6) % SINSAMPLES];
				toneOutput += sintable[(sampleInCycle * 12) % SINSAMPLES];
				break;
			}

			res = (toneOutput * amp) / 100000;
			AUDIO_Buffer[2 * sample] = res;
			AUDIO_Buffer[2 * sample + 1] = res;
		}
		
		amp *= decay;
		sample++;

		// generate tones //
		sampleInCycle++;
		// All tones are multiples of 20 Hz
		sampleInCycle %= SINSAMPLES;
	}
}

void AUDIO_frame(void)
{
	// start a new audio frame
	memset(AUDIO_Buffer, 0, sizeof(AUDIO_Buffer));
	sample = 0;
}

void AUDIO_reset(void)
{
	memset(AUDIO_Buffer, 0, sizeof(AUDIO_Buffer));

	// reset tone generator
	tone = 0;

	// start a new audio frame
	sample = 0;
}
