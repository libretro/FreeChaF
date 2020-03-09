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

#include <math.h>
#include <string.h>

#define Pi 3.14159265

short AUDIO_Buffer[735];

unsigned char tone = 0; // current tone

static const int sampleRate = 44100;
static const float framesPerSecond = 59.94; // NTSC
static const float samplePeriod = 0.00002267573; // 1.0 / sampleRate
static const int samplesPerFrame = 735; // sampleRate / framesPerSecond

int sampleInCycle = 0; // time since start of tone, resets to 0 after every full cycle
#define FULL_AMPLITUDE 16384
short amp = FULL_AMPLITUDE; // tone amplitude (16384 = full)
static const float decay = 0.998; // multiplier for amp per sample

short ticks = 0; // unprocessed ticks in 1/100 of tick
int sample = 0; // current sample buffer position

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
			// All tones are multiples of 20 Hz
			float time = sampleInCycle * samplePeriod;
			float toneOutput = 0;
			switch (tone) {
			case 1:
				toneOutput = sin(2*Pi*1000*time) / Pi;
				break;
			case 2:
				toneOutput = sin(2*Pi*500*time) /  Pi;
				break;
			case 3:
				toneOutput = sin(2*Pi*120*time) /  Pi;
				toneOutput += sin(2*Pi*240*time) /  Pi;
				break;
			}

			AUDIO_Buffer[2 * sample] = (int)(toneOutput * amp);
			AUDIO_Buffer[2 * sample + 1] = (int)(toneOutput * amp);
		}
		
		amp *= decay;
		sample++;

		// generate tones //
		sampleInCycle++;
		sampleInCycle %= sampleRate / 20;
	}
}

void AUDIO_frame(void)
{
	while(sample<samplesPerFrame)
	{
		AUDIO_Buffer[sample]=0;
		sample++;
	}
	// start a new audio frame
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
