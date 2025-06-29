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

#include "mixer.h"
#include "serializer.h"
#include "sys.h"


static int8_t addclamp(int a, int b) {
	int add = a + b;
	if (add < -128) {
		add = -128;
	}
	else if (add > 127) {
		add = 127;
	}
	return (int8_t)add;
}

Mixer::Mixer(System *stub) 
	: sys(stub) {
}

extern void *mixer_arc_callback;

void *mixer_channels;

void Mixer::init() {
	memset(_channels, 0, sizeof(_channels));
	mixer_channels = _channels;
	sys->startAudio((System::AudioCallback)&mixer_arc_callback, this);
}

void Mixer::free() {
	stopAll();
	sys->stopAudio();
}

void Mixer::playChannel(uint8_t channel, const MixerChunk *mc, uint16_t freq, uint8_t volume) {
	debug(DBG_SND, "Mixer::playChannel(%d, %d, %d)", channel, freq, volume);
	assert(channel < AUDIO_NUM_CHANNELS);

	MixerChannel *ch = &_channels[channel];

	ch->active = false;
	asm volatile("": : :"memory");

	ch->volume = volume;
	ch->chunk = *mc;
	ch->chunkPos = 0;
	ch->chunkInc = (freq << 8) / sys->getOutputSampleRate();

	asm volatile("": : :"memory");
	ch->active = true;
}

void Mixer::stopChannel(uint8_t channel) {
	debug(DBG_SND, "Mixer::stopChannel(%d)", channel);
	assert(channel < AUDIO_NUM_CHANNELS);

	_channels[channel].active = false;
}

void Mixer::setChannelVolume(uint8_t channel, uint8_t volume) {
	debug(DBG_SND, "Mixer::setChannelVolume(%d, %d)", channel, volume);
	assert(channel < AUDIO_NUM_CHANNELS);

	_channels[channel].volume = volume;
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");

	for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		_channels[i].active = false;		
	}
}

void Mixer::mix(int8_t *buf, int len) {
}

void Mixer::mixCallback(void *param, uint8_t *buf, int len) {
}

void Mixer::saveOrLoad(Serializer &ser) {
	sys->lockMutex(_mutex);
	for (int i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		Serializer::Entry entries[] = {
			SE_INT(&ch->active, Serializer::SES_BOOL, VER(2)),
			SE_INT(&ch->volume, Serializer::SES_INT8, VER(2)),
			SE_INT(&ch->chunkPos, Serializer::SES_INT32, VER(2)),
			SE_INT(&ch->chunkInc, Serializer::SES_INT32, VER(2)),
			SE_PTR(&ch->chunk.data, VER(2)),
			SE_INT(&ch->chunk.len, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopPos, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopLen, Serializer::SES_INT16, VER(2)),
			SE_END()
		};
		ser.saveOrLoadEntries(entries);
	}
	sys->unlockMutex(_mutex);
};
