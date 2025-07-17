/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <swis.h>
#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <unixlib/local.h>
#include <signal.h>
#include "sys.h"
#include "util.h"
#include "mixer.h"

#define InternalKey_Up			57
#define InternalKey_Down		41
#define InternalKey_Left		25
#define InternalKey_Right		121
#define InternalKey_Space		98
#define InternalKey_C			82
#define InternalKey_Escape		112

#define OSByte_Vsync 			19
#define OSByte_WriteDisplayScreenBank	113

#define MAX_TIMERS 8

struct ArcTimer {
	uint32_t interval;
	uint32_t next_callback;
	System::TimerCallback callback;
	void *param;
};

struct ArcStub : System {
	enum {
		SCREEN_W = 320,
		SCREEN_H = 200,
		SOUND_SAMPLE_RATE = 20833
	};

	int DEFAULT_SCALE = 3;

	virtual ~ArcStub() {}
	virtual void init(const char *title);
	virtual void destroy();
	virtual void setPalette(const uint8_t *buf);
	virtual void updateDisplay(const uint8_t *src, uint8_t pageId);
	virtual void processEvents();
	virtual void sleep(uint32_t duration);
	virtual uint32_t getTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32_t getOutputSampleRate();
	virtual int addTimer(uint32_t delay, TimerCallback callback, void *param);
	virtual void removeTimer(int timerId);
	virtual void *createMutex();
	virtual void destroyMutex(void *mutex);
	virtual void lockMutex(void *mutex);
	virtual void unlockMutex(void *mutex);
	virtual void getDefaultDataDir(const char **path);
	virtual bool getVideoPages(uint8_t *pages[4]);

	bool keyDown(uint8_t key);
	void timerCallback();
	void initVideoMemory();

	char *AnotherWorldDir;
	char AnotherWorldDataDir[256];

	uint32_t old_callback_handler;
	uint32_t old_callback_r12;
	uint32_t old_callback_register_buffer;

	ArcTimer timers[MAX_TIMERS];

	uint8_t last_page;

	bool use_joystick;
};

extern void *tickerv_handler;
extern void *callback_handler;
extern uint32_t *callback_register_buffer;

static const uint32_t videoSize = 160*256*2 + 160*200*2;

void ArcStub::initVideoMemory()
{
	const uint32_t vdu_variables_in[] = {148, -1};
	uint32_t *screen_addr = 0;
	uint32_t area_size;

	// Read old screen size
	_swi(OS_ReadDynamicArea, _IN(0) | _OUT(1), 2, &area_size);

	if (area_size < videoSize) {
		// Attempt to change size. OS_ChangeDynamicArea will exit app on error
		_swi(OS_ChangeDynamicArea, _INR(0, 1), 2, videoSize - area_size);
	}

	// Clear out video memory
	_swi(OS_ReadVduVariables, _IN(0) | _IN(1), &vdu_variables_in, &screen_addr);
	memset(screen_addr, 0, videoSize);
}

void ArcStub::init(const char *title) {
	struct sigaction sigint_action;
	const uint8_t mode_string[] = {22, 9, 23, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Disable SIGINT (Escape)
	sigint_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sigint_action, NULL);

	memset(timers, 0, sizeof(timers));

	last_page = 2;

	for (int i = 0; i < sizeof(mode_string); i++)
		_kernel_oswrch(mode_string[i]);

	initVideoMemory();

	_swi(OS_ChangeEnvironment, _INR(0,3) | _OUTR(1,3),
	     7, &callback_handler, 0, &callback_register_buffer,
	     &old_callback_handler, &old_callback_r12, &old_callback_register_buffer);

	_swi(OS_Claim, _INR(0, 2),
	     0x1c, &tickerv_handler, 0);

	memset(&input, 0, sizeof(input));

	use_joystick = _swix(Joystick_Read, _IN(0), 0) ? false : true;
}

void ArcStub::destroy() {
	_swi(OS_Release, _INR(0, 2),
	     0x1c, &tickerv_handler, 0);

	_swi(OS_ChangeEnvironment, _INR(0,3),
	     7, old_callback_handler, old_callback_r12, old_callback_register_buffer);

	_kernel_oswrch(22);
	_kernel_oswrch(12);
}

void ArcStub::setPalette(const uint8_t *p) {
  // The incoming palette is in 565 format.
	uint8_t palette_block[5] = {0, 16, 0, 0, 0};

	for (int i = 0; i < 16; i++)
	{
		palette_block[0] = i;
    		uint8_t c1 = *(p + 0);
		uint8_t c2 = *(p + 1);
		p += 2;

		palette_block[2] = (((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2)) << 2; // r
    		palette_block[3] = (((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6)) << 2; // g
		palette_block[4] = (((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2)) << 2; // b

		_kernel_osword(0xc, (int *)palette_block);
	}
}

void ArcStub::updateDisplay(const uint8_t *src, uint8_t pageId) {
	uint8_t new_page = last_page;

	switch (pageId) {
	case 1:
		new_page = 1;
		break;
	case 2:
		new_page = 2;
		break;
	case 0xff:
		new_page = (last_page == 1) ? 2 : 1;
		break;
	}

	_kernel_osbyte(OSByte_WriteDisplayScreenBank, new_page, 0);
	_kernel_osbyte(OSByte_Vsync, 0, 0);
	last_page = new_page;
}

bool ArcStub::getVideoPages(uint8_t *pages[4])
{
	const uint32_t vdu_variables_in[] = {148, -1};
	uint8_t *screen_addr = 0;

	_swi(OS_ReadVduVariables, _IN(0) | _IN(1), &vdu_variables_in, &screen_addr);

	// Allocate the top of screen memory to the actual display pages; this
	// ensures we can use OSByte calls to change page without having to
	// use screen-sized allocations for the back buffers
	//
	// Add an offset to centre the pages on the screen
	uint32_t offset = ((256 - 200) / 2) * 160;

	pages[1] = screen_addr + offset;
	pages[2] = screen_addr + offset + 160*256;

	pages[0] = screen_addr + 160*256*2;
	pages[3] = screen_addr + 160*256*2 + 160*200;

	return true;
}

bool ArcStub::keyDown(uint8_t key)
{
	uint32_t ret = _kernel_osbyte(129, key ^ 0xff, 0xff);

	return ((ret & 0xffff) == 0xffff);
}

void ArcStub::processEvents() {
	if (keyDown(InternalKey_Up))
		input.dirMask |= PlayerInput::DIR_UP;
	else
		input.dirMask &= ~PlayerInput::DIR_UP;
	if (keyDown(InternalKey_Down))
		input.dirMask |= PlayerInput::DIR_DOWN;
	else
		input.dirMask &= ~PlayerInput::DIR_DOWN;
	if (keyDown(InternalKey_Left))
		input.dirMask |= PlayerInput::DIR_LEFT;
	else
		input.dirMask &= ~PlayerInput::DIR_LEFT;
	if (keyDown(InternalKey_Right))
		input.dirMask |= PlayerInput::DIR_RIGHT;
	else
		input.dirMask &= ~PlayerInput::DIR_RIGHT;
	if (keyDown(InternalKey_Space))
		input.button = true;
	else
		input.button = false;
	if (keyDown(InternalKey_C))
		input.code = true;
	if (keyDown(InternalKey_Escape))
		input.quit = true;

	if (use_joystick) {
		int8_t x, y;
		uint8_t sw;
		uint32_t state;

		_swi(Joystick_Read, _IN(0) | _OUT(0), 0, &state);

		y = state & 0xff;
		x = (state >> 8) & 0xff;
		sw = (state >> 16) & 0xff;

		if (x < -32)
			input.dirMask |= PlayerInput::DIR_LEFT;
		if (x >  32)
			input.dirMask |= PlayerInput::DIR_RIGHT;
		if (y >  32)
			input.dirMask |= PlayerInput::DIR_UP;
		if (y < -32)
			input.dirMask |= PlayerInput::DIR_DOWN;

		if (sw)
			input.button = true;
	}
}

void ArcStub::sleep(uint32_t duration) {
	uint32_t start_time, cur_time;

	_swi(OS_ReadMonotonicTime, _OUT(0), &start_time);
	start_time *= 10;
	cur_time = start_time;

	do
	{
		_swi(OS_ReadMonotonicTime, _OUT(0), &cur_time);
		cur_time *= 10;
	} while ((cur_time - start_time) < duration);
}

uint32_t ArcStub::getTimeStamp() {
	uint32_t mono_time;

	_swi(OS_ReadMonotonicTime, _OUT(0), &mono_time);

	return mono_time * 10;
}

static uint32_t channel_handler_header[4];
uint32_t sound_config_data[5];

void ArcStub::startAudio(AudioCallback callback, void *param) {
	channel_handler_header[0] = (uint32_t)callback;
	channel_handler_header[1] = 0;
	channel_handler_header[2] = 0;
	channel_handler_header[3] = 0;

	sound_config_data[0] = AUDIO_NUM_CHANNELS;
	sound_config_data[1] = 256;
	sound_config_data[2] = 48; // 48us, 20833 Hz
	sound_config_data[3] = (uint32_t)channel_handler_header;
	sound_config_data[4] = 0;

	_swi(Sound_Configure, _INR(0, 4) | _OUTR(0, 4),
	      sound_config_data[0],  sound_config_data[1],  sound_config_data[2],  sound_config_data[3],  sound_config_data[4],
	     &sound_config_data[0], &sound_config_data[1], &sound_config_data[2], &sound_config_data[3], &sound_config_data[4]);
	// Old sound config written to sound_config_data[]
}

void ArcStub::stopAudio() {
	_swi(Sound_Configure, _INR(0, 4),
	     sound_config_data[0], sound_config_data[1], sound_config_data[2], sound_config_data[3], sound_config_data[4]);
}

uint32_t ArcStub::getOutputSampleRate() {
	return SOUND_SAMPLE_RATE;
}

int ArcStub::addTimer(uint32_t delay, TimerCallback callback, void *param) {
	uint32_t mono_time;

	_swi(OS_ReadMonotonicTime, _OUT(0), &mono_time);

	for (int i = 0; i < MAX_TIMERS; i++) {
		if (!timers[i].interval) {
			timers[i].callback = callback;
			timers[i].param = param;
			timers[i].next_callback = (mono_time * 10) + delay;
			asm volatile("": : :"memory");
			timers[i].interval = delay;

			return i;
		}
	}

	return 0;
}

void ArcStub::removeTimer(int timerId) {
	timers[timerId].interval = 0;
}

void *ArcStub::createMutex() {
//	return SDL_CreateMutex();
	return 0;
}

void ArcStub::destroyMutex(void *mutex) {
//	SDL_DestroyMutex((SDL_mutex *)mutex);
}

void ArcStub::lockMutex(void *mutex) {
//	SDL_mutexP((SDL_mutex *)mutex);
}

void ArcStub::unlockMutex(void *mutex) {
//	SDL_mutexV((SDL_mutex *)mutex);
}

void ArcStub::getDefaultDataDir(const char **path) {
	char riscosAWDir[256];

	if (!_kernel_getenv("AnotherWorld$Dir", riscosAWDir, sizeof(riscosAWDir))) {
		AnotherWorldDir = __unixify_std(riscosAWDir, NULL, 0, __RISCOSIFY_FILETYPE_NOTSPECIFIED);

		snprintf(AnotherWorldDataDir, sizeof(AnotherWorldDataDir), "%s/data", AnotherWorldDir);
		AnotherWorldDataDir[sizeof(AnotherWorldDataDir) - 1] = 0;
		*path = AnotherWorldDataDir;
	}
}

void ArcStub::timerCallback() {
	uint32_t mono_time;

	_swi(OS_ReadMonotonicTime, _OUT(0), &mono_time);
	mono_time *= 10;

	for (int i = 0; i < MAX_TIMERS; i++) {
		while (timers[i].interval && (mono_time - timers[i].next_callback) < (1u << 31)) {
			timers[i].interval = timers[i].callback(timers[i].interval, timers[i].param);
			timers[i].next_callback += timers[i].interval;
		}
	}
}

ArcStub sysImplementation;
System *stub = &sysImplementation;

extern "C" void arcStubTimerCallback()
{
	sysImplementation.timerCallback();
}
