#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include "audio_processing.h"
#include "vocoder.h"
#include "effects.h"

static int audioCallback(const void *input, void *output, unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    const short* inputBuffer = (const short*) input;
    short* outputBuffer = (short*) output;
    VocoderState* state = (VocoderState*) userData;
    float* currentBuffer = (state->currentBuffer == 0) ? state->buffer1 : state->buffer2;
    static float prevSample = 0.0f;

    if (inputBuffer == NULL) return paContinue;

    for (unsigned long i = 0; i < frameCount; ++i) {
        float modulator = inputBuffer[i] / 32768.0f;
        float carrier = sinf(state->phase) * modulator;

        if (state->pitchShift != 1.0f) carrier = applyPitchShift(carrier, state->pitchShift);
        if (state->distortion) carrier = applyDistortion(carrier);
        if (state->reverb) carrier = applyReverb(carrier, &prevSample);

        state->phase += 2.0f * M_PI * CARRIER_FREQ / SAMPLE_RATE;
        if (state->phase >= 2.0f * M_PI) state->phase -= 2.0f * M_PI;

        currentBuffer[i] = carrier;
    }

    for (unsigned long i = 0; i < frameCount; ++i) {
        outputBuffer[i] = (short)(currentBuffer[i] * 32767.0f);
    }

    state->currentBuffer = 1 - state->currentBuffer;
    return paContinue;
}

void startAudioProcessing() {
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, CHANNELS, CHANNELS, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, &vocoderState);
    Pa_StartStream(stream);
}

void stopAudioProcessing() {
    Pa_Terminate();
}
