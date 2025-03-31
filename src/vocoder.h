#ifndef VOCODER_STATE_H
#define VOCODER_STATE_H

typedef struct {
    float phase;
    float* buffer1;
    float* buffer2;
    int currentBuffer;
    int distortion;
    int reverb;
    float pitchShift;
} VocoderState;

extern VocoderState vocoderState;

void initVocoderState();
void freeVocoderState();
void parseArguments(int argc, char* argv[]);

#endif