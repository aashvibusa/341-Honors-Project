#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <string.h>
#include <stdbool.h>
#include <sndfile.h>
#include <signal.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 128
#define NUM_CHANNELS 1
#define SAMPLE_MAX (32767.0f)
#define SAMPLE_MIN (-32768.0f)
#define PI_2 (2.0f * (float)M_PI)
#define REVERB_BUFFER_SIZE 441
#define PITCH_BUFFER_SIZE 4096  
#define ECHO_BUFFER_SIZE 44100

typedef enum {
    EFFECT_NONE,
    EFFECT_LOW,
    EFFECT_WOBBLE,
    EFFECT_ROBOT,
    EFFECT_ECHO
} EffectType;

typedef struct {
    EffectType effect_type;

    float echo_buffer[ECHO_BUFFER_SIZE];
    int echo_index;

    float wobble_phase;
    float wobble_speed;
    float sine_table[1024];

    float input_buffer[PITCH_BUFFER_SIZE];
    float output_buffer[PITCH_BUFFER_SIZE];
    int buffer_position;
    float read_position;
} VocoderState;

VocoderState vocoder = {0};

static void initialize_vocoder() {
    for (int i = 0; i < 1024; i++) {
        vocoder.sine_table[i] = sinf((float)i * PI_2 / 1024.0f);
    }

    vocoder.echo_index = 0;
    vocoder.wobble_phase = 0.0f;
    vocoder.wobble_speed = 5.0f; 
    vocoder.buffer_position = 0;
    vocoder.read_position = 0.0f;

    memset(vocoder.echo_buffer, 0, sizeof(vocoder.echo_buffer));
    memset(vocoder.input_buffer, 0, sizeof(vocoder.input_buffer));
    memset(vocoder.output_buffer, 0, sizeof(vocoder.output_buffer));
}

static inline float fast_sin(float x) {
    while (x >= PI_2) x -= PI_2;
    while (x < 0) x += PI_2;
    int index = (int)((x / PI_2) * 1024.0f);
    if (index >= 1024) index = 1023;
    return vocoder.sine_table[index];
}

static void process_low_effect(float* input, float* output, unsigned long frame_count) {
    float pitch_factor = 1.5f; 

    for (unsigned long i = 0; i < frame_count; i++) {
        vocoder.input_buffer[vocoder.buffer_position] = input[i];
        vocoder.buffer_position = (vocoder.buffer_position + 1) % PITCH_BUFFER_SIZE;
    }

    if (fabsf(vocoder.buffer_position - vocoder.read_position) > PITCH_BUFFER_SIZE/2) {
        vocoder.read_position = vocoder.buffer_position;
    }

    for (unsigned long i = 0; i < frame_count; i++) {
        float read_pos = vocoder.read_position - (float)frame_count + (float)i / pitch_factor;
        
        while (read_pos < 0)
            read_pos += PITCH_BUFFER_SIZE;
        while (read_pos >= PITCH_BUFFER_SIZE)
            read_pos -= PITCH_BUFFER_SIZE;

        int pos1 = (int)read_pos;
        int pos2 = (pos1 + 1) % PITCH_BUFFER_SIZE;
        float frac = read_pos - pos1;
        
        output[i] = vocoder.input_buffer[pos1] * (1.0f - frac) + 
                    vocoder.input_buffer[pos2] * frac;
    }

    vocoder.read_position += frame_count / pitch_factor;
    while (vocoder.read_position >= PITCH_BUFFER_SIZE)
        vocoder.read_position -= PITCH_BUFFER_SIZE;
}

static void process_wobble_effect(float* input, float* output, unsigned long frame_count) {
    for (unsigned long i = 0; i < frame_count; i++) {
        float wobble_value = fast_sin(vocoder.wobble_phase);
        float amplitude_mod = wobble_value * 0.6f + 0.4f;
        
        output[i] = input[i] * amplitude_mod;
        vocoder.wobble_phase += PI_2 * vocoder.wobble_speed / SAMPLE_RATE;
        if (vocoder.wobble_phase >= PI_2) {
            vocoder.wobble_phase -= PI_2;
        }
    }
}

static void process_robot_effect(float* input, float* output, unsigned long frame_count) {
    static float carrier_phase = 0.0f;
    float carrier_freq = 50.0f; 
    
    for (unsigned long i = 0; i < frame_count; i++) {
        float carrier = fast_sin(carrier_phase);

        float envelope = fabsf(input[i]);
        output[i] = (carrier > 0 ? 0.8f : -0.8f) * envelope;
    
        carrier_phase += PI_2 * carrier_freq / SAMPLE_RATE;
        if (carrier_phase >= PI_2) {
            carrier_phase -= PI_2;
        }
    }
}

static void process_echo_effect(float* input, float* output, unsigned long frame_count) {
    float echo_feedback = 0.5f;
    int delay_samples = SAMPLE_RATE / 2; 
    
    for (unsigned long i = 0; i < frame_count; i++) {
        int pos = (vocoder.echo_index - delay_samples + ECHO_BUFFER_SIZE) % ECHO_BUFFER_SIZE;
        float echo_sample = vocoder.echo_buffer[pos];

        output[i] = input[i] * 0.7f + echo_sample * 0.3f;

        vocoder.echo_buffer[vocoder.echo_index] = input[i] + echo_sample * echo_feedback;
        vocoder.echo_index = (vocoder.echo_index + 1) % ECHO_BUFFER_SIZE;
    }
}

static int audio_callback(const void* input, void* output, unsigned long frame_count, 
                          const PaStreamCallbackTimeInfo* time_info, 
                          PaStreamCallbackFlags status_flags, void* user_data) {
    (void)time_info; (void)status_flags; (void)user_data;
    const int16_t* in = (const int16_t*)input;
    int16_t* out = (int16_t*)output;
    if (!in) return paContinue;

    float in_buffer[frame_count];
    float out_buffer[frame_count];
    
    for (unsigned long i = 0; i < frame_count; ++i) {
        in_buffer[i] = in[i] / SAMPLE_MAX;
    }

    switch (vocoder.effect_type) {
        case EFFECT_LOW:
            process_low_effect(in_buffer, out_buffer, frame_count);
            break;
        case EFFECT_WOBBLE:
            process_wobble_effect(in_buffer, out_buffer, frame_count);
            break;
        case EFFECT_ROBOT:
            process_robot_effect(in_buffer, out_buffer, frame_count);
            break;
        case EFFECT_ECHO:
            process_echo_effect(in_buffer, out_buffer, frame_count);
            break;
        default:
            memcpy(out_buffer, in_buffer, frame_count * sizeof(float));
            break;
    }

    for (unsigned long i = 0; i < frame_count; ++i) {
        if (out_buffer[i] > 1.0f) out_buffer[i] = 1.0f;
        if (out_buffer[i] < -1.0f) out_buffer[i] = -1.0f;
        
        out[i] = (int16_t)(out_buffer[i] * SAMPLE_MAX);
    }
    
    return paContinue;
}

static void parse_arguments(int argc, char* argv[]) {
    vocoder.effect_type = EFFECT_NONE;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-low") == 0) {
            vocoder.effect_type = EFFECT_LOW;
            printf("Low effect enabled\n");
        } else if (strcmp(argv[i], "-wobble") == 0) {
            vocoder.effect_type = EFFECT_WOBBLE;
            printf("Wobble effect enabled\n");

            if (i + 1 < argc && argv[i + 1][0] != '-') {
                vocoder.wobble_speed = atof(argv[i + 1]);
                i++; 
                printf("Wobble speed set to: %.2f Hz\n", vocoder.wobble_speed);
            } else {
                vocoder.wobble_speed = 5.0f;
                printf("Using default wobble speed: 5.0 Hz\n");
            }
        } else if (strcmp(argv[i], "-robot") == 0) {
            vocoder.effect_type = EFFECT_ROBOT;
            printf("Robot effect enabled\n");
        } else if (strcmp(argv[i], "-echo") == 0) {
            vocoder.effect_type = EFFECT_ECHO;
            printf("Echo effect enabled\n");
        }
    }
}

int main(int argc, char* argv[]) {
    initialize_vocoder();

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio initialization error: %s\n", Pa_GetErrorText(err));
        return EXIT_FAILURE;
    }

    parse_arguments(argc, argv);

    PaStream* stream;
    PaStreamParameters inputParams, outputParams;
    
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        printf("No default input device found.\n");
        Pa_Terminate();
        return EXIT_FAILURE;
    }
    
    inputParams.channelCount = NUM_CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;
    
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        printf("No default output device found.\n");
        Pa_Terminate();
        return EXIT_FAILURE;
    }
    
    outputParams.channelCount = NUM_CHANNELS;
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        &inputParams,
        &outputParams,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        audio_callback,
        NULL
    );
    
    if (err != paNoError) {
        printf("Failed to open audio stream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Failed to start audio stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return EXIT_FAILURE;
    }
    
    printf("Vocoder Running with effect type: ");
    switch (vocoder.effect_type) {
        case EFFECT_NONE: printf("None\n"); break;
        case EFFECT_LOW: printf("Low\n"); break;
        case EFFECT_WOBBLE: printf("Wobble\n"); break;
        case EFFECT_ROBOT: printf("Robot\n"); break;
        case EFFECT_ECHO: printf("Echo\n"); break;
    }
    
    printf("Press Enter to exit...\n");
    getchar();

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    return EXIT_SUCCESS;
}


