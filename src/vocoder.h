#ifndef VOCODER_H
#define VOCODER_H

#include "effects/constants.h"

typedef enum {
    EFFECT_NONE,
    EFFECT_LOW,
    EFFECT_HIGH,
    EFFECT_PITCH,
    EFFECT_WOBBLE,
    EFFECT_ROBOT,
    EFFECT_ECHO
} EffectType;

typedef struct {
    EffectType effect_type;
    float wobble_phase;
    float wobble_speed;
} VocoderState;

VocoderState* get_vocoder();
void initialize_vocoder();
const char* effect_to_string(EffectType effect);

#endif