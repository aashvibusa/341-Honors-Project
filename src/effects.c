#include "effects.h"
#include "vocoder.h"
#include <math.h>
#include <string.h>

#define PI_2 (2.0f * 3.1415926535f)
#define ECHO_BUFFER_SIZE (SAMPLE_RATE * 2)
#define PITCH_BUFFER_SIZE 4096

// Low effect variables
static float pitch_buffer[PITCH_BUFFER_SIZE];
static int pitch_pos = 0;
static float read_pos = 0.0f;

// Wobble effect variables
static float wobble_phase = 0.0f;

// Echo effect variables
static float echo_buffer[ECHO_BUFFER_SIZE];
static int echo_pos = 0;

void process_low_effect(const float* input, float* output, unsigned long frame_count) {
    const float pitch_shift = 0.6f;
    
    for (unsigned i = 0; i < frame_count; i++) {
        pitch_buffer[pitch_pos] = input[i];
        pitch_pos = (pitch_pos + 1) % PITCH_BUFFER_SIZE;
    }
    
    for (unsigned i = 0; i < frame_count; i++) {
        float pos = read_pos;
        while (pos < 0) pos += PITCH_BUFFER_SIZE;
        while (pos >= PITCH_BUFFER_SIZE) pos -= PITCH_BUFFER_SIZE;
        
        int pos1 = (int)pos;
        int pos2 = (pos1 + 1) % PITCH_BUFFER_SIZE;
        float frac = pos - pos1;
        
        output[i] = pitch_buffer[pos1] * (1.0f - frac) + pitch_buffer[pos2] * frac;
        read_pos += pitch_shift;
    }
    
    if (read_pos >= PITCH_BUFFER_SIZE) read_pos -= PITCH_BUFFER_SIZE;
}

void process_wobble_effect(const float* input, float* output, unsigned long frame_count) {
    VocoderState* v = get_vocoder();
    float rate = v->wobble_speed;
    
    for (unsigned i = 0; i < frame_count; i++) {
        float modulation = 0.5f + 0.5f * sinf(wobble_phase);
        output[i] = input[i] * modulation;
        wobble_phase += PI_2 * rate / SAMPLE_RATE;
        if (wobble_phase > PI_2) wobble_phase -= PI_2;
    }
}

void process_robot_effect(const float* input, float* output, unsigned long frame_count) {
    static float phase = 0.0f;
    float carrier_freq = 80.0f;
    
    for (unsigned i = 0; i < frame_count; i++) {
        float carrier = sinf(phase) > 0 ? 1.0f : -1.0f;
        output[i] = carrier * fabsf(input[i]);
        phase += PI_2 * carrier_freq / SAMPLE_RATE;
        if (phase > PI_2) phase -= PI_2;
    }
}

void process_echo_effect(const float* input, float* output, unsigned long frame_count) {
    int delay_samples = SAMPLE_RATE / 2;
    float feedback = 0.5f;
    
    for (unsigned i = 0; i < frame_count; i++) {
        int delay_pos = (echo_pos - delay_samples + ECHO_BUFFER_SIZE) % ECHO_BUFFER_SIZE;
        float echo = echo_buffer[delay_pos];
        output[i] = input[i] + echo;
        echo_buffer[echo_pos] = input[i] + echo * feedback;
        echo_pos = (echo_pos + 1) % ECHO_BUFFER_SIZE;
    }
}