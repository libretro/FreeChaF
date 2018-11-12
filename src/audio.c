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

#define Pi 3.14159265

// unsigned int AUDIO_Buffer[735];

int tone = 0; // current tone
float toneOutput[] = {0.0, 0.0, 0.0, 0.0}; // tone generator outputs
float toneCounter = 0.0;

float sampleRate = 44100;
float framesPerSecond = 59.94; // NTSC
float samplePeriod = 0.00002267573; // 1.0 / sampleRate
int samplesPerFrame = 735; // sampleRate / framesPerSecond

float time = 0.0; // time since start of tone
float amp = 1.0; // tone amplitude
float decay = 0.998; // multiplier for amp per sample

float ticks = 0.0; // unprocessed ticks
int sample = 0; // current sample buffer position

void AUDIO_portReceive(int port, int val)
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
			amp = 1;
			time=0;
		}
	}
}

void AUDIO_tick(int dt) // dt = ticks elapsed since last call
{
	// an audio frame lasts ~14914 ticks
	// at 44.1khz, there are 735 samples per frame
	// ~20.29 ticks per sample (14913.15 ticks/frame)
	
	ticks+=(float)dt;

	while(ticks>20.29)
	{
		ticks-=20.29;
		
		AUDIO_Buffer[sample] = 0;
		if(sample<samplesPerFrame) // output sample
		{
			AUDIO_Buffer[sample] = (int)((toneOutput[tone] * amp)*16384);
		}
		
		amp *= decay;
		sample++;

		// generate tones //
		time += samplePeriod;
		toneOutput[1] = sin(2*Pi*1000*time) / Pi;
		toneOutput[2] = sin(2*Pi*500*time) /  Pi;
		toneOutput[3] = sin(2*Pi*120*time) /  Pi;
		toneOutput[3]+= sin(2*Pi*240*time) /  Pi;
	}
}

void AUDIO_frame()
{
	while(sample<samplesPerFrame)
	{
		AUDIO_Buffer[sample]=0;
		sample++;
	}
	// start a new audio frame
	sample = 0;
}

void AUDIO_reset()
{
	int i;

	// clear buffer
	for(i=0; i<735; i++)
	{
		AUDIO_Buffer[i] = 0;
	}

	// reset tone generator
	tone = 0;
	toneCounter = 0;
	for(i=1; i<4; i++)
	{
		toneOutput[i] = 0;
	}

	// start a new audio frame
	sample = 0;
}