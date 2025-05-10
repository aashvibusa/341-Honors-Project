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
        case EFFECT_NONE:   return "None";
        case EFFECT_LOW:    return "Low";
        case EFFECT_HIGH:   return "High";
        case EFFECT_WOBBLE: return "Wobble";
        case EFFECT_ROBOT:  return "Robot";
        case EFFECT_ECHO:   return "Echo";
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
