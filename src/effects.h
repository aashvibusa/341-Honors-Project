#ifndef EFFECTS_H
#define EFFECTS_H

float applyDistortion(float sample);
float applyReverb(float sample, float* prevSample);
float applyPitchShift(float sample, float pitchShiftFactor);

#endif
