#ifndef EFFECTS_H
#define EFFECTS_H
#include "constants.h"
#include <stdio.h>

void process_low_effect(const float* input, float* output, unsigned long frame_count);
void process_high_effect(const float* input, float* output, unsigned long frame_count);
void process_pitch_effect(const float* input, float* output, unsigned long frame_count, float pitch_val);
void process_wobble_effect(const float* input, float* output, unsigned long frame_count);
void process_robot_effect(const float* input, float* output, unsigned long frame_count);
void process_echo_effect(const float* input, float* output, unsigned long frame_count);

#endif