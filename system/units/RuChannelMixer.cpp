#include "RuChannelMixer.h"
#define MIXER_C1_BUF 1
#define MIXER_C2_BUF 2

#define MIXER_C1_ACT 4
#define MIXER_C2_ACT 8

#define MIXER_BUFFER 64

#define C1_FULL (mixerState&MIXER_C1_BUF)
#define C2_FULL (mixerState&MIXER_C2_BUF)
#define MIXER_FULL (mixerState&MIXER_BUFFER)

using namespace RackoonIO;
RuChannelMixer::RuChannelMixer()
: RackUnit() {
	addJack("channel_1", JACK_SEQ);
	addJack("channel_2", JACK_SEQ);
	addPlug("audio_out");
	mixedPeriod = periodC1 = periodC2 = nullptr;
	gainC1 = gainC2 = 1.0;
	mixerState = 0;
}


FeedState RuChannelMixer::feed(Jack *jack) {
	short *period;
	static int task = 0;
	task++;
	Jack *out = getPlug("audio_out")->jack;
	out->frames = jack->frames;
	if(MIXER_FULL) {
		if(out->feed(mixedPeriod) == FEED_WAIT)
			return FEED_WAIT;
		mixerState ^= MIXER_BUFFER;
	}

	// could be stale data here
	if(jack->name == "channel_1") {
		if( C1_FULL ) {
			return FEED_WAIT;
		} else {
			jack->flush(&periodC1);
			mixerState ^= MIXER_C1_BUF;
		}

	} else {

		if( C2_FULL ) {
			return FEED_WAIT;
		} else {
			jack->flush(&periodC2);
			mixerState ^= MIXER_C2_BUF;
		}
	}

	if(!C1_FULL || !C2_FULL) {
		return FEED_OK;
	} else {
		mixedPeriod = (short*) malloc(sizeof(short)*jack->frames);
		for(int i = 0; i < jack->frames; i++) {
			mixedPeriod[i] = ((periodC1[i]) * gainC1) +
					 ((periodC2[i]) * gainC2);
		}

		free(periodC1);
		free(periodC2);
		mixerState ^= (MIXER_BUFFER ^ MIXER_C1_BUF ^ MIXER_C2_BUF );

		if(out->feed(mixedPeriod) == FEED_OK) {
			mixerState ^= MIXER_BUFFER;
		}
	}

	return FEED_OK;
}
RackState RuChannelMixer::init() {
	workState = READY;
	cout << "RuChannelMixer: Initialised" << endl;
	return RACK_UNIT_OK;
}

RackState RuChannelMixer::cycle() {
	return RACK_UNIT_OK;
}

void RuChannelMixer::setConfig(string config, string value) {

}

void RuChannelMixer::block(Jack *jack) {
	Jack *out = getPlug("audio_out")->jack;
	out->block();
}
