#include "effects.h"

float applyDistortion(float sample) {
    return (sample > 0.7f) ? 0.7f : (sample < -0.7f) ? -0.7f : sample;
}

float applyReverb(float sample, float* prevSample) {
    float reverbAmount = 0.5f;
    *prevSample = sample + (*prevSample * reverbAmount);
    return *prevSample;
}

float applyPitchShift(float sample, float pitchShiftFactor) {
    return sample * pitchShiftFactor;
}
