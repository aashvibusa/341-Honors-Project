#include "vocoder.h"

static VocoderState vocoder_instance = {
    .effect_type = EFFECT_NONE,
    .wobble_phase = 0.0f,
    .wobble_speed = 5.0f
};

VocoderState* get_vocoder() {
    return &vocoder_instance;
}

void initialize_vocoder() {
    // Re-initialize if needed
    VocoderState* v = get_vocoder();
    v->effect_type = EFFECT_NONE;
    v->wobble_phase = 0.0f;
    v->wobble_speed = 5.0f;
}
