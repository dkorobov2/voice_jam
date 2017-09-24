#ifndef Header_PitchDetector
#define Header_PitchDetector

#include <math.h>

#include "PitchDetector.h"
#include "jni.h"
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <SuperpoweredFilter.h>
#include <SuperpoweredRoll.h>
#include <SuperpoweredFlanger.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SuperpoweredRecorder.h>
#include <SuperpoweredBandpassFilterbank.h>
#include <string>

#define  PATH_LENGTH 100
#define  NUM_OCTAVES 4
#define  NOTES_PER_OCTAVE 12
#define  NUM_NOTES NUM_OCTAVES*NOTES_PER_OCTAVE
#define  DEFAULT_SAMPLE_RATE 44100
#define  MIN_PEAK 0.1
#define  MIN_FREQUENCY_REPETITIONS 3

class PitchDetector {

	static float * noteFrequencies;
	static float * noteBands;
public:
    PitchDetector(unsigned int samplerate, unsigned int buffersize, const char *path, const char * internalPath);
	~PitchDetector();

	bool process(short int *output, unsigned int numberOfSamples);

	// returns 0 if frequency is invalid, else 1
	bool getFrequency();
	void onStart(bool record);
private:
	std::string apkPath;
	SuperpoweredAndroidAudioIO *audioSystem;
	SuperpoweredRecorder *recorder;
	SuperpoweredBandpassFilterbank *frequency;
	float *recordingBuffer;
	float * bandResults;
	float * bandPeak;
	float * bandSum;
	char * tempFile;
	bool recording;
    int numRepititions;
};

#endif
