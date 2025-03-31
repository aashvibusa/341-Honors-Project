// vocoder.h

#ifndef VOCODER_STATE_H
#define VOCODER_STATE_H

#define FRAMES_PER_BUFFER 512
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define CARRIER_FREQ 440.0f

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
