#ifndef RUALSA_H
#define RUALSA_H
#include "framework/rack/RackUnit.h"
class RuAlsa : public RackoonIO::RackUnit
{
	enum WorkState {
		IDLE,
		INIT,
		READY,
		PRIMING,
		STREAMING,
		FLUSHING
	};

	WorkState workState;
	snd_pcm_t *handle;
	FILE *fp;
	unsigned int sampleRate, mLatency, bufSize, bufLevel, maxPeriods;
	short *frameBuffer;
	snd_pcm_uframes_t triggerLevel, fPeriod;
	unsigned int x;

	RackoonIO::FeedState feedJackAudio();
	std::mutex bufLock;

	void audioFeed();

	void actionInitAlsa();
	void actionFlushBuffer();

public:
	RuAlsa();
	RackoonIO::FeedState feed(RackoonIO::Jack*);
	void setConfig(string,string);

	RackoonIO::RackState init();
	RackoonIO::RackState cycle();
};

#endif
