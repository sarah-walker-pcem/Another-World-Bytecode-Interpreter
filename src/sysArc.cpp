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
#include "sys.h"
#include "util.h"

#define InternalKey_Up			57
#define InternalKey_Down		41
#define InternalKey_Left		25
#define InternalKey_Right		121
#define InternalKey_Space		98
#define InternalKey_C			82

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
	virtual void updateDisplay(const uint8_t *src);
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

	bool keyDown(uint8_t key);

	char *AnotherWorldDir;
	char AnotherWorldDataDir[256];
};

void ArcStub::init(const char *title) {
	static const uint8_t mode_string[] = {22, 9, 23, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	for (int i = 0; i < sizeof(mode_string); i++)
		_kernel_oswrch(mode_string[i]);

	memset(&input, 0, sizeof(input));
}

void ArcStub::destroy() {
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

void ArcStub::updateDisplay(const uint8_t *src) {
	const uint32_t vdu_variables_in[] = {148, -1};
	uint32_t *screen_addr = 0;

	_swi(OS_ReadVduVariables, _IN(0) | _IN(1), &vdu_variables_in, &screen_addr);

	memcpy(screen_addr, src, 160 * 200);
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

void ArcStub::startAudio(AudioCallback callback, void *param) {
/*	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));

	desired.freq = SOUND_SAMPLE_RATE;
	desired.format = AUDIO_U8;
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) == 0) {
		SDL_PauseAudio(0);
	} else {
		error("SDLStub::startAudio() unable to open sound device");
	}*/
}

void ArcStub::stopAudio() {
//	SDL_CloseAudio();
}

uint32_t ArcStub::getOutputSampleRate() {
	return SOUND_SAMPLE_RATE;
}

int ArcStub::addTimer(uint32_t delay, TimerCallback callback, void *param) {
//	return SDL_AddTimer(delay, (SDL_TimerCallback)callback, param);
	return 0;
}

void ArcStub::removeTimer(int timerId) {
//	SDL_RemoveTimer(timerId);
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

ArcStub sysImplementation;
System *stub = &sysImplementation;

