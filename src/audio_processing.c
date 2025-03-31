// audio_processing.c

#include <portaudio.h>
#include "vocoder.h"
#include "audio_processing.h"
#include <stdio.h>      // For printf
#include <math.h>       // For sinf, fmodf, fmaxf, fminf
#include <stdlib.h>     // For NULL
#include "effects.h"

// Declare stream as static so it’s only available in this file
static PaStream* stream = NULL;


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
    state->phase = fmodf(state->phase, 2.0f * M_PI);  // Normalize phase

    currentBuffer[i] = carrier;
    }

    for (unsigned long i = 0; i < frameCount; ++i) {
    // Properly scale audio data to fit in the output buffer's range
    outputBuffer[i] = (short)(fmaxf(fminf(currentBuffer[i] * 32767.0f, 32767), -32768));
    }

    state->currentBuffer = 1 - state->currentBuffer;
    return paContinue;
}

// Initialize audio stream
void startAudioProcessing() {
    Pa_Initialize();

    PaStreamParameters inputParams, outputParams;
    
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        printf("Error: No default input device.\n");
        return;
    }
    inputParams.channelCount = CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = CHANNELS;
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    // Open and start the PortAudio stream
    Pa_OpenStream(&stream, &inputParams, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, audioCallback, &vocoderState);
    Pa_StartStream(stream);
}

// Stop and close the audio stream
void stopAudioProcessing() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

