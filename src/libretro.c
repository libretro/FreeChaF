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
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "libretro.h"
#include <file/file_path.h>
#include <retro_miscellaneous.h>
#include <retro_endianness.h>

#include "memory.h"
#include "channelf.h"
#include "controller.h"
#include "audio.h"
#include "video.h"
#include "osd.h"
#include "ports.h"
#include "controller.h"
#include "f2102.h"
#include "channelf_hle.h"

#define DefaultFPS 60
#define frameWidth 306
#define frameHeight 192
#define frameSize (framePitchPixel * frameHeight)

#ifdef PSP
// Workaround for a psp1 gfx driver.
#define framePitchPixel 320
#else
#define framePitchPixel frameWidth
#endif

pixel_t frame[frameSize];

char *SystemPath;

struct hle_state_s hle_state;

retro_environment_t Environ;
retro_log_printf_t log_cb;
retro_video_refresh_t Video;
retro_audio_sample_t Audio;
retro_audio_sample_batch_t AudioBatch;
retro_input_poll_t InputPoll;
retro_input_state_t InputState;
struct retro_vfs_interface *vfs_interface;

void retro_set_environment(retro_environment_t fn)
{
	Environ = fn;

	struct retro_vfs_interface_info vfs_interface_info;
	vfs_interface_info.required_interface_version = 1;
	vfs_interface_info.iface = NULL;
	if (fn(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_interface_info))
	{
		vfs_interface = vfs_interface_info.iface;
	}

	static struct retro_variable variables[] =
		{
			{
				"freechaf_fast_scrclr",
				"Clear screen in single frame; disabled|enabled",
			},
			{ NULL, NULL },
		};

	fn(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

static void update_variables(void)
{
	struct retro_variable var;
	var.key = "freechaf_fast_scrclr";
	var.value = NULL;

	hle_state.fast_screen_clear = (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) && strcmp(var.value, "enabled") == 0;
}

static int CHANNELF_loadROM_libretro(const char* path, int address)
{
	if (vfs_interface != NULL) // load rom using Libretro's vfs interface
	{
		struct retro_vfs_file_handle *h = vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		if (!h) // problem loading file
		{
			return 0;
		}

		ssize_t size = vfs_interface->size(h);
		if (size <= 0) // problem loading file
		{
			return 0;
		}
		if (size > MEMORY_SIZE - address) // if too large to fit in memory...
		{
			size = MEMORY_SIZE - address;
		}

		size = vfs_interface->read(h, Memory + address, size);
		vfs_interface->close(h);
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
	
	// If we can't use Libretro's vfs interface to load the rom, do things the old way ... 
	return CHANNELF_loadROM(path, address);
}


void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
void retro_set_audio_sample(retro_audio_sample_t fn) { Audio = fn; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t fn) { AudioBatch = fn; }
void retro_set_input_poll(retro_input_poll_t fn) { InputPoll = fn; }
void retro_set_input_state(retro_input_state_t fn) { InputState = fn; }

struct retro_game_geometry Geometry;

int joypad0[10]; // joypad 0 state
int joypad1[10]; // joypad 1 state
int joypre0[10]; // joypad 0 previous state
int joypre1[10]; // joypad 1 previous state

bool console_input = false;

// at 44.1khz, read 735 samples (44100/60) 
static const int audioSamples = 735;

static void fallback_log(enum retro_log_level level,
			 const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void retro_init(void)
{
	char PSU_1_Update_Path[PATH_MAX_LENGTH];
	char PSU_1_Path[PATH_MAX_LENGTH];
	char PSU_2_Path[PATH_MAX_LENGTH];

	// init buffers, structs
	memset(frame, 0, frameSize*sizeof(pixel_t));

	OSD_setDisplay(frame, framePitchPixel, frameHeight);

	// init console
	CHANNELF_init();

	struct retro_log_callback log;

	if (Environ(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
		log_cb = log.log;
	else
		log_cb = fallback_log;

	// get paths
	Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath);

	// load PSU 1 Update
	fill_pathname_join(PSU_1_Update_Path, SystemPath, "sl90025.bin", PATH_MAX_LENGTH);
	if(!CHANNELF_loadROM_libretro(PSU_1_Update_Path, 0))
	{
		log_cb(RETRO_LOG_WARN, "[WARN] [FREECHAF] Failed loading Channel F II BIOS(1) from: %s\n", PSU_1_Update_Path);
		
		// load PSU 1 Original
		fill_pathname_join(PSU_1_Path, SystemPath, "sl31253.bin", PATH_MAX_LENGTH);
		if(!CHANNELF_loadROM_libretro(PSU_1_Path, 0))
		{
			log_cb(RETRO_LOG_WARN, "[WARN] [FREECHAF] Failed loading Channel F BIOS(1) from: %s\n", PSU_1_Path);
			log_cb(RETRO_LOG_WARN, "[WARN] [FREECHAF] Switching to HLE for PSU1\n");
			hle_state.psu1_hle = true;
		}
	}

	// load PSU 2
	fill_pathname_join(PSU_2_Path, SystemPath, "sl31254.bin", PATH_MAX_LENGTH);
	if(!CHANNELF_loadROM_libretro(PSU_2_Path, 0x400))
	{
		log_cb(RETRO_LOG_WARN, "[WARN] [FREECHAF] Failed loading Channel F BIOS(2) from: %s\n", PSU_2_Path);
		log_cb(RETRO_LOG_WARN, "[WARN] [FREECHAF] Switching to HLE for PSU2\n");
		hle_state.psu2_hle = true;
	}

	if (hle_state.psu1_hle || hle_state.psu2_hle)
	{
			struct retro_message msg;
			msg.msg    = "Couldn't load BIOS. Using experimental HLE mode. In case of problem please use BIOS";
			msg.frames = 600;
			Environ(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
	}
}

bool retro_load_game(const struct retro_game_info *info)
{
	update_variables();
	if (!CHANNELF_loadROM_mem(info->data, info->size, 0x800))
		return false;

	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "forward" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "back" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "push" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "rotate right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "pull" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "rotate left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"swap left/right controllers" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "swap console/controller input" },

		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "forward" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "back" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "push" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "rotate right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "pull" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "rotate left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"swap left/right controllers" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "swap console/controller input" },

		{ 0 },
	};

	Environ(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	return true;
}

void retro_unload_game(void)
{
	
}

void retro_run(void)
{
	int i = 0;

	bool updated = false;
	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
	{
		update_variables();
	}

	InputPoll();

	for(i=0; i<sizeof(joypre0)/sizeof(joypre0[0]); i++) // Copy previous state 
	{
		joypre0[i] = joypad0[i];
		joypre1[i] = joypad1[i];
	}

	/* JoyPad 0 */

	joypad0[0] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad0[1] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad0[2] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad0[3] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad0[4] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad0[5] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad0[6] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad0[7] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad0[8] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad0[9] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	/* JoyPad 1 */

	joypad1[0] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad1[1] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad1[2] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad1[3] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad1[4] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad1[5] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad1[6] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad1[7] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad1[8] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad1[9] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	// swap console/controller input //
	if((joypad0[8]==1 && joypre0[8]==0) || (joypad1[8]==1 && joypre1[8]==0))
	{
		console_input = !console_input;
	}
	
	// swap left/right controllers //
	if((joypad0[9]==1 && joypre0[9]==0) || (joypad1[9]==1 && joypre1[9]==0))
	{
		CONTROLLER_swap();
	}

	if(console_input) // console input
	{
		if(((joypad0[2]==1 && joypre0[2]==0) || (joypad1[2]==1 && joypre1[2]==0)))  // left
		{
			CONTROLLER_consoleInput(0, 1);
		}

		if(((joypad0[3]==1 && joypre0[3]==0) || (joypad1[3]==1 && joypre1[3]==0))) // right
		{
			CONTROLLER_consoleInput(1, 1);
		}

		for(i=4; i<8; i++)
		{
			if((joypad0[i]==1 && joypre0[i]==0) || (joypad1[i]==1 && joypre1[i]==0)) // a,b,x,y
			{
				CONTROLLER_consoleInput(2, 1);
			}
			if((joypad0[i]==0 && joypre0[i]==1) || (joypad1[i]==0 && joypre1[i]==1)) // a,b,x,y
			{
				CONTROLLER_consoleInput(2, 0);
			}
		}
	}
	else
	{
		// ordinary controller input
		CONTROLLER_setInput(1,
		(joypad0[5]<<7)|               /* push         - B    - ALeft Down  -         */
		(joypad0[6]<<6)|               /* pull         - X    - ALeft Up    -         */
		(joypad0[4]<<5)|               /* rotate right - A    - ALeft Right - ShRight */
		(joypad0[7]<<4)|               /* rotate left  - Y    - ALeft rLeft - ShLeft  */
		(joypad0[0]<<3)|               /* forward      - Up   - ARight Up   -         */
		(joypad0[1]<<2)|               /* back         - Down - ARight Down -         */
		(joypad0[2]<<1)|               /* left         - Left - ARight Left -         */
		(joypad0[3]) );                /* right        - Right- ARight Right-         */

		CONTROLLER_setInput(2,
		(joypad1[5]<<7)|               /* push         - B    - ALeft Down  -         */
		(joypad1[6]<<6)|               /* pull         - X    - ALeft Up    -         */
		(joypad1[4]<<5)|               /* rotate right - A    - ALeft Right - ShRight */
		(joypad1[7]<<4)|               /* rotate left  - Y    - ALeft rLeft - ShLeft  */
		(joypad1[0]<<3)|               /* forward      - Up   - ARight Up   -         */
		(joypad1[1]<<2)|               /* back         - Down - ARight Down -         */
		(joypad1[2]<<1)|               /* left         - Left - ARight Left -         */
		(joypad1[3]) );                /* right        - Right- ARight Right-         */
	}

	// grab frame
	if(hle_state.psu1_hle || hle_state.psu2_hle || hle_state.fast_screen_clear)
	{
		CHANNELF_HLE_run();
	}
	else
	{
		CHANNELF_run();
	}

	AudioBatch (AUDIO_Buffer, audioSamples);
	AUDIO_frame(); // notify audio to start new audio frame

	// send frame to libretro
	VIDEO_drawFrame();
	// 3x upscale (gives more resolution for OSD)
	int offset = 0;
	int color = 0;
	int row;
	int col;
	for(row=0; row<64; row++)
	{
		offset = (row*3)*framePitchPixel;
		for(col=0; col<102; col++)
		{
			color =  VIDEO_Buffer_rgb[row*128+col+4];
			frame[offset]   = color;
			frame[offset+1] = color;
			frame[offset+2] = color;

			frame[offset+framePitchPixel] = color;
			frame[offset+framePitchPixel+1] = color;
			frame[offset+framePitchPixel+2] = color;

			frame[offset+2*framePitchPixel] = color;
			frame[offset+2*framePitchPixel+1] = color;
			frame[offset+2*framePitchPixel+2] = color;
			offset+=3;
		}
	}
	// OSD
	if((joypad0[9]==1) || (joypad1[9]==1)) // Show Controller Swap State 
	{
		if(CONTROLLER_swapped())
		{
			OSD_drawP1P2();
		}
		else
		{
			OSD_drawP2P1();
		}
	}
	if(console_input) // Show Console Buttons
	{
		 OSD_drawConsole(CONTROLLER_cursorPos(), CONTROLLER_cursorDown());
	}
	// Output video
	Video(frame, frameWidth, frameHeight, sizeof(pixel_t) * framePitchPixel);
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "FreeChaF";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
	info->library_version = "1.0" GIT_VERSION;
	info->valid_extensions = "bin|rom|chf";
	info->need_fullpath = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
#ifdef USE_RGB565
	int pixelformat = RETRO_PIXEL_FORMAT_RGB565;
#else
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;
#endif	

	memset(info, 0, sizeof(*info));
	info->geometry.base_width   = frameWidth;
	info->geometry.base_height  = frameHeight;
	info->geometry.max_width    = frameWidth;
	info->geometry.max_height   = frameHeight;
	info->geometry.aspect_ratio = ((float)frameWidth) / ((float)frameHeight);

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = 44100.0;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void) {  }

void retro_reset(void)
{
	CHANNELF_reset();
}

struct serialized_state
{
	unsigned int CPU_Ticks_Debt;
	unsigned char Memory[MEMORY_SIZE];
	unsigned char R[R_SIZE]; // 64 byte Scratchpad
	unsigned char VIDEO_Buffer[8192];
	unsigned char Ports[64];

	unsigned short PC0; // Program Counter
	unsigned short PC1; // Program Counter alternate
	unsigned short DC0; // Data Counter
	unsigned short DC1; // Data Counter alternate
	unsigned char ISAR; // Indirect Scratchpad Address Register (6-bit)
	unsigned char W; // Status Register (flags)

	unsigned short f2102_state;
	unsigned char f2102_memory[1024];
	unsigned short f2102_address;
	unsigned char f2102_rw;
	unsigned char A; // Accumulator

	unsigned char ARM, X, Y, Color;
	unsigned char ControllerEnabled;
	unsigned char ControllerSwapped;

	unsigned char console_input;
	unsigned char tone;
	unsigned short amp;

	struct hle_state_s hle_state;
};

size_t retro_serialize_size(void)
{
	return sizeof (struct serialized_state);
}

bool retro_serialize(void *data, size_t size)
{
	if (size < sizeof (struct serialized_state))
		return false;

	struct serialized_state *st = data;
	memcpy(st->Memory, Memory, MEMORY_SIZE);
	memcpy(st->R, R, R_SIZE);
	memcpy(st->VIDEO_Buffer, VIDEO_Buffer_raw, sizeof(VIDEO_Buffer_raw));
	memcpy(st->Ports, Ports, sizeof(Ports));
	memcpy(st->f2102_memory, f2102_memory, sizeof(f2102_memory));

	st->A = A;
	st->ISAR = ISAR;
	st->W = W;

	st->PC0 = retro_cpu_to_be16(PC0);
	st->PC1 = retro_cpu_to_be16(PC1);
	st->DC0 = retro_cpu_to_be16(DC0);
	st->DC1 = retro_cpu_to_be16(DC1);

	st->X = X;
	st->Y = Y;
	st->Color = Color;
	st->ARM = ARM;

	st->f2102_rw = f2102_rw;
	st->f2102_address = retro_cpu_to_be16(f2102_address);
	st->f2102_state = retro_cpu_to_be16(f2102_state);

	st->ControllerEnabled = ControllerEnabled;
	st->ControllerSwapped = ControllerSwapped;
	st->console_input = console_input;

	st->tone = tone;
	st->amp = retro_cpu_to_be16(amp);
	st->hle_state = hle_state;
	st->CPU_Ticks_Debt = retro_cpu_to_be32(CPU_Ticks_Debt);

	return true;
}

bool retro_unserialize(const void *data, size_t size)
{
	if (size < sizeof (struct serialized_state))
		return false;

	const struct serialized_state *st = data;
	memcpy (Memory, st->Memory, MEMORY_SIZE);
	memcpy (R, st->R, R_SIZE);
	memcpy (VIDEO_Buffer_raw, st->VIDEO_Buffer, sizeof(VIDEO_Buffer_raw));
	memcpy (Ports, st->Ports, sizeof(Ports));
	memcpy (f2102_memory, st->f2102_memory, sizeof(f2102_memory));

	A = st->A;
	ISAR = st->ISAR;
	W = st->W;

	PC0 = retro_be_to_cpu16(st->PC0);
	PC1 = retro_be_to_cpu16(st->PC1);
	DC0 = retro_be_to_cpu16(st->DC0);
	DC1 = retro_be_to_cpu16(st->DC1);

	X = st->X;
	Y = st->Y;
	Color = st->Color;
	ARM = st->ARM;

	f2102_rw = st->f2102_rw;
	f2102_address = retro_be_to_cpu16(st->f2102_address);
	f2102_state = retro_be_to_cpu16(st->f2102_state);

	ControllerEnabled = st->ControllerEnabled;
	ControllerSwapped = st->ControllerSwapped;

	console_input = st->console_input;
	hle_state = st->hle_state;

	tone = st->tone;
	amp = retro_be_to_cpu16(st->amp);
	CPU_Ticks_Debt = retro_be_to_cpu32(st->CPU_Ticks_Debt);

	return true;
}

size_t retro_get_memory_size(unsigned id)
{
	switch(id)
	{
		case RETRO_MEMORY_SYSTEM_RAM: // System Memory
			return 0x10000; //65536
	
		case RETRO_MEMORY_VIDEO_RAM: // Video Memory
			return 0x2000; //8192

		//case RETRO_MEMORY_SAVE_RAM: // SRAM / Regular save RAM
		//case RETRO_MEMORY_RTC: // Real-time clock value  
	}
	return 0;
}

void *retro_get_memory_data(unsigned id)
{
	switch(id)
	{
		case RETRO_MEMORY_SYSTEM_RAM: // System Memory
			return Memory;
	
		case RETRO_MEMORY_VIDEO_RAM: // Video Memory
			return VIDEO_Buffer_raw;

		//case RETRO_MEMORY_SAVE_RAM: // SRAM / Regular save RAM
		//case RETRO_MEMORY_RTC: // Real-time clock value  
	}
	return 0;
}

/* Stubs */
unsigned int retro_api_version(void) { return RETRO_API_VERSION; }
void retro_cheat_reset(void) {  }
void retro_cheat_set(unsigned index, bool enabled, const char *code) {  }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {  }
