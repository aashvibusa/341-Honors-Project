#ifndef EFFECTS_H
#define EFFECTS_H

#include "constants.h"

void process_low_effect(const float* input, float* output, unsigned long frame_count);
void process_high_effect(const float* input, float* output, unsigned long frame_count);
void process_wobble_effect(const float* input, float* output, unsigned long frame_count);
void process_robot_effect(const float* input, float* output, unsigned long frame_count);
void process_echo_effect(const float* input, float* output, unsigned long frame_count);

#endif