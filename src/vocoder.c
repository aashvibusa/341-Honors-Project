#include <stdlib.h>
#include <string.h>
#include "vocoder.h"

VocoderState vocoderState = {0};

void initVocoderState() {
    vocoderState.buffer1 = (float*)malloc(FRAMES_PER_BUFFER * sizeof(float));
    vocoderState.buffer2 = (float*)malloc(FRAMES_PER_BUFFER * sizeof(float));
    vocoderState.currentBuffer = 0;
}

void freeVocoderState() {
    free(vocoderState.buffer1);
    free(vocoderState.buffer2);
}

void parseArguments(int argc, char* argv[]) {
    vocoderState.distortion = 0;
    vocoderState.reverb = 0;
    vocoderState.pitchShift = 1.0f;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) vocoderState.distortion = 1;
        else if (strcmp(argv[i], "-r") == 0) vocoderState.reverb = 1;
        else if (strcmp(argv[i], "-p") == 0) vocoderState.pitchShift = 15.0f;
    }
}
