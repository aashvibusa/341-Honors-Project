#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define CHANNELS 1
#define CARRIER_FREQ 200.0  // Hz
#define MAX_EFFECTS 3

typedef struct {
    float phase;
    float* buffer1;
    float* buffer2;
    int currentBuffer;  // 0 for buffer1, 1 for buffer2
    int distortion;
    int reverb;
    float pitchShift;
} VocoderState;

VocoderState vocoderState = {0};

float applyDistortion(float sample) {
    if (sample > 0.7f) {
        return 0.7f;
    }

    if (sample < -0.7f) {
        return -0.7f;
    }

    return sample;
}

float applyReverb(float sample, float* prevSample) {
    float reverbAmount = 0.5f;
    *prevSample = sample + (*prevSample * reverbAmount);
    return *prevSample;
}

float applyPitchShift(float sample, float pitchShiftFactor) {
    return sample * pitchShiftFactor;
}

static int audioCallback(const void *input, void *output, unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    const short* inputBuffer = (const short*) input;
    short* outputBuffer = (short*) output;
    VocoderState* state = (VocoderState*) userData;
    
    // select the current buffer based on the toggle
    float* currentBuffer = (state->currentBuffer == 0) ? state->buffer1 : state->buffer2;

    // for reverb, keep track of the previous sample
    static float prevSample = 0.0f;
    
    if (inputBuffer == NULL) {
        return paContinue;
    }

    // fill the current buffer
    for (unsigned long i = 0; i < frameCount; ++i) {
        float modulator = inputBuffer[i] / 32768.0f; // normalize voice input
        float carrier = sinf(state->phase) * modulator; // multiply carrier wave with input

        if (state->pitchShift != 1.0f) {
            carrier = applyPitchShift(carrier, state->pitchShift);
        }

        if (state->distortion) {
            carrier = applyDistortion(carrier);
        }

        if (state->reverb) {
            carrier = applyReverb(carrier, &prevSample);
        }

        state->phase += 2.0f * M_PI * CARRIER_FREQ / SAMPLE_RATE; // formula

        if (state->phase >= 2.0f * M_PI) {
            state->phase -= 2.0f * M_PI;
        }

        currentBuffer[i] = carrier; // store in the current buffer
    }
    
    // copy the processed data to the output buffer
    for (unsigned long i = 0; i < frameCount; ++i) {
        outputBuffer[i] = (short)(currentBuffer[i] * 32767.0f); // convert back to 16-bit PCM
    }

    // toggle the buffer for the next iteration
    state->currentBuffer = 1 - state->currentBuffer;

    return paContinue;
}

void parseArguments(int argc, char* argv[]) {
    vocoderState.distortion = 0;
    vocoderState.reverb = 0;
    vocoderState.pitchShift = 1.0f;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            vocoderState.distortion = 1;
        } else if (strcmp(argv[i], "-r") == 0) {
            vocoderState.reverb = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            vocoderState.pitchShift = 15.0f;
        }
    }
}

int main(int argc, char* argv[]) {
    // parse command-line arguments to determine which effects to apply
    parseArguments(argc, argv);

    // initialize buffers
    vocoderState.buffer1 = (float*)malloc(FRAMES_PER_BUFFER * sizeof(float));
    vocoderState.buffer2 = (float*)malloc(FRAMES_PER_BUFFER * sizeof(float));
    vocoderState.currentBuffer = 0;  // start with buffer1

    // initialize PortAudio
    Pa_Initialize();
    PaStream* stream;

    // open the stream with the double-buffering callback
    Pa_OpenDefaultStream(&stream, CHANNELS, CHANNELS, paInt16, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, &vocoderState);
    Pa_StartStream(stream);

    printf("Vocoder Running with Effects (Press Enter to exit)...\n");
    getchar();

    // clean up
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    // free the buffers
    free(vocoderState.buffer1);
    free(vocoderState.buffer2);
    
    return 0;
}
