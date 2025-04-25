#include "vocoder.h"

static VocoderState vocoder_instance = {
    .effect_type = EFFECT_NONE,
    .wobble_phase = 0.0f,
    .wobble_speed = 5.0f
};

VocoderState* get_vocoder() {
    return &vocoder_instance;
}

const char* effect_to_string(EffectType effect) {
    switch (effect) {
        case EFFECT_NONE:   return "EFFECT_NONE";
        case EFFECT_LOW:    return "EFFECT_LOW";
        case EFFECT_WOBBLE: return "EFFECT_WOBBLE";
        case EFFECT_ROBOT:  return "EFFECT_ROBOT";
        case EFFECT_ECHO:   return "EFFECT_ECHO";
        default:            return "Unknown Effect";
    }
}

void initialize_vocoder() {
    // Re-initialize if needed
    VocoderState* v = get_vocoder();
    v->effect_type = EFFECT_NONE;
    v->wobble_phase = 0.0f;
    v->wobble_speed = 5.0f;
}
