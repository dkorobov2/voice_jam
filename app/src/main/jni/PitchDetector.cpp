#include "PitchDetector.h"
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#define  LOG_TAG "RECORD TAG"

#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// from C2 to B5
static std::string notes[NUM_NOTES] = {"C2", "Db2", "D2", "Eb2", "E2", "F2", "Gb2", "G2", "Ab2", "A2", "Bb2", "B2", "C3", "Db3", "D3", "Eb3", "E3", "F3", "Gb3", "G3", "Ab3", "A3", "Bb3", "B3", "C4", "Db4", "D4", "Eb4", "E4", "F4", "Gb4", "G4", "Ab4", "A4", "Bb4", "B4", "C5", "Db5", "D5", "Eb5", "E5", "F5", "Gb5", "G5", "Ab5", "A5", "Bb5", "B5",};

static float noteFrequencyData[NUM_NOTES] = {65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77};
static float noteBandData[NUM_NOTES] = {1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f, 1.0f / 12.0f};

float * PitchDetector::noteFrequencies = noteFrequencyData;
float * PitchDetector::noteBands = noteBandData;

static PitchDetector *pitchDetector = NULL;

// variables for interfacing with jni
int lastNoteIndex;
JavaVM * g_vm;
jobject g_obj;
jmethodID g_mid;

static void printEventIndex(SuperpoweredAdvancedAudioPlayerEvent event) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        LOGD("Superpowered Event: LoadSuccess");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadError) {
        LOGD("Superpowered Event: LoadError");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_NetworkError) {
        LOGD("Superpowered Event: NetworkError");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_EOF) {
        LOGD("Superpowered Event: EOF");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_JogParameter) {
        LOGD("Superpowered Event: JogParameter");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_DurationChanged) {
        LOGD("Superpowered Event: DurationChanged");
    } else if (event == SuperpoweredAdvancedAudioPlayerEvent_LoopEnd) {
        LOGD("Superpowered Event: LoopEnd");
    }
}

// audio processing callback
static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples, int __unused samplerate) {
    return ((PitchDetector *)clientdata)->process(audioIO, (unsigned int)numberOfSamples);
}

// constructor
PitchDetector::PitchDetector(unsigned int samplerate, unsigned int buffersize, const char *path, const char * storagePath) {
    recordingBuffer = (float *)memalign(16, (buffersize + 16) * sizeof(float) * 2);
    tempFile = new char[PATH_LENGTH];
    recording = false;

    lastNoteIndex = -1;
    // recording player
    apkPath.assign(path);
    LOGD("APKPATH: %s", apkPath.c_str());

    // recording
    strcpy(tempFile, storagePath);
    strcat(tempFile, "/1");
    LOGD("Temporary file: %s", tempFile);

    //recorder = new SuperpoweredRecorder(tempFile, samplerate, 1);
    recorder = new SuperpoweredRecorder("", samplerate, 1);

    recorder->setSamplerate(DEFAULT_SAMPLE_RATE);
    // frequency filter
    bandResults = new float[NUM_NOTES];
    bandPeak = new float;
    bandSum = new float;

    numRepititions = 0;

    frequency = new SuperpoweredBandpassFilterbank(NUM_NOTES, noteFrequencies, noteBands, samplerate);

    // initialize audiosystem
    //set the third argument to false for the emulator
    audioSystem = new SuperpoweredAndroidAudioIO(samplerate, buffersize, true, true, audioProcessing, this, SL_ANDROID_RECORDING_PRESET_GENERIC, SL_ANDROID_STREAM_MEDIA, buffersize * 2);
}

// destructor
PitchDetector::~PitchDetector() {
    delete audioSystem;
    delete recorder;
    delete tempFile;
    free(recordingBuffer);
    free(noteFrequencies);
    free(noteBands);
    free(bandResults);
}

// start recording
void PitchDetector::onStart(bool record) {
    if (record) {
        bool check = recorder->start(tempFile);
        LOGD("Started recording!");
        if (!check)
            LOGE("Another recording is still active!");
        else
            recording = true;
    }
    else {
        recorder->stop();
        LOGD("Stopped recording!");
        recording = false;
    }
    SuperpoweredCPU::setSustainedPerformanceMode(record); // <-- Important to prevent audio dropouts.
}

// method to communicate the detected frequency to JAVA VM
void playNote() {
    JNIEnv * g_env;
    // double check it's all ok
    int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6); LOGD("1");
    if (getEnvStat == JNI_EDETACHED) {
        LOGE("GetEnv: not attached");
        if (g_vm->AttachCurrentThread(&g_env, NULL) != 0) {
            LOGE("Failed to attach");
        }
    } else if (getEnvStat == JNI_OK) {
        //
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
    }
    g_env->CallVoidMethod(g_obj, g_mid, lastNoteIndex);
    if (g_env->ExceptionCheck()) {
        g_env->ExceptionDescribe();
    }
    g_vm->DetachCurrentThread();
}

// method to get the fundamental frequency of last audio sample
bool PitchDetector::getFrequency() {

    if (*bandPeak < MIN_PEAK) {
        return false;
    }

    int noteIndex = 0;
    float peak = bandResults[0];
    for (int i = 1; i < NUM_NOTES; i++) {
        if(bandResults[i] > peak) {
            peak = bandResults[i];
            noteIndex = i;
        }
    }

    if (noteIndex == lastNoteIndex) {
        return false;
    }

    lastNoteIndex = noteIndex;
    LOGD("Note Frequency: %.2f. Note: %s. Peak: %f", noteFrequencyData[lastNoteIndex], notes[lastNoteIndex].c_str(), *bandPeak);
    playNote();
    return true;
}

// Superpowered callback used for recording and detecting pitch
bool PitchDetector::process(short int *output, unsigned int numberOfSamples) {
    if (recording) {
        SuperpoweredShortIntToFloat(output, recordingBuffer, numberOfSamples);
        recorder->process(recordingBuffer, NULL, numberOfSamples);

        if (numRepititions < MIN_FREQUENCY_REPETITIONS)
        {
            if (numRepititions == 0)
            {
                *bandSum = 0;
                *bandPeak = 0;
                frequency->processNoAdd(recordingBuffer, bandResults, bandPeak, bandSum, numberOfSamples);
            }
            else {
                frequency->process(recordingBuffer, bandResults, bandPeak, bandSum, numberOfSamples);
            }
            numRepititions++;
        }
        else {
            frequency->process(recordingBuffer, bandResults, bandPeak, bandSum, numberOfSamples);
            getFrequency();
            numRepititions = 0;
        }
    }
    return false;
}

// JNI Initializer
extern "C" JNIEXPORT void JNICALL Java_com_superpowered_voicejam_MainActivity_registerJNI(JNIEnv * env, jobject obj)
{
        bool returnValue = true;
        // convert local to global reference
        // (local will die after this method call)
        g_obj = env->NewGlobalRef(obj);

        // save refs for callback
        jclass g_clazz = env->GetObjectClass(g_obj);
        if (g_clazz == NULL) {
            LOGE("Failed to find class");
        }

        g_mid = env->GetMethodID(g_clazz, "playNote", "(I)V");
        if (g_mid == NULL) {
            LOGE("Unable to get method ref");
        }

        env->GetJavaVM(&g_vm);
}

// PitchDetector constructor
extern "C" JNIEXPORT void Java_com_superpowered_voicejam_MainActivity_PitchDetector(JNIEnv *javaEnvironment, jobject __unused obj, jint samplerate, jint buffersize, jstring apkPath, jstring storePath) {
    const char *path = javaEnvironment->GetStringUTFChars(apkPath, JNI_FALSE);
    const char *storagePath = javaEnvironment->GetStringUTFChars(storePath, JNI_FALSE);
    pitchDetector = new PitchDetector((unsigned int)samplerate, (unsigned int)buffersize, path, storagePath);
    javaEnvironment->ReleaseStringUTFChars(apkPath, path);
}

// Start recording
extern "C" JNIEXPORT void Java_com_superpowered_voicejam_MainActivity_onStart(JNIEnv * __unused javaEnvironment, jobject __unused obj, jboolean record) {
    pitchDetector->onStart(record);
}