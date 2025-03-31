#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <string.h>
#include <stdbool.h>
#include <sndfile.h>
#include <signal.h>

#define SAMPLE_RATE         44100
#define FRAMES_PER_BUFFER   128
#define NUM_CHANNELS        1
#define CARRIER_FREQ        200.0f
#define FORMANT_FREQ        800.0f
#define SAMPLE_MAX          (32767.0f)
#define SAMPLE_MIN          (-32768.0f)
#define PI_2                (2.0f * (float)M_PI)
#define DEFAULT_OUTPUT_FILE "vocoder_output.wav"
#define REVERB_BUFFER_SIZE  (441)
#define PITCH_BUFFER_SIZE   (1024)

typedef struct {
    float phase_carrier;
    float phase_formant;
    float* buffer_a;
    float* buffer_b;
    float* write_buffer;
    float* read_buffer;
    bool distortion_enabled;
    bool reverb_enabled;
    float pitch_shift;
    float reverb_buffer[REVERB_BUFFER_SIZE];
    int reverb_index;
    float sine_table[1024];
    bool recording_enabled;
    int recording_seconds;
    SNDFILE* recording_file;
    SF_INFO recording_info;
    int recording_frames_left;
    int16_t* recording_buffer;
    
    // pitch shift variables
    float pitch_phase_accum;
    float pitch_buffer[PITCH_BUFFER_SIZE];
    int pitch_buffer_index;

    bool test_mode_enabled;
    float test_frequency;
} VocoderState;

VocoderState vocoder = {0};
volatile bool stop_recording = false;

/* Function declarations */
static inline float fast_sin(float x);
static inline float apply_pitch_shift(float sample);
static inline float apply_distortion(float sample);
static inline float apply_reverb(float sample);
static void process_vocoder(const float* modulator, float* carrier, unsigned long n);
static void swap_buffers(void);
static int audio_callback(const void* input, void* output, 
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo* time_info, 
                         PaStreamCallbackFlags status_flags, 
                         void* user_data);
static void parse_arguments(int argc, char* argv[]);
static void init_sine_table(void);
static bool init_buffers(void);
static bool init_recording(const char* filename);
static void cleanup_recording(void);
static void handle_sigint(int sig);

/* Function implementations */
static inline float fast_sin(float x) {
    while (x >= PI_2) x -= PI_2;
    while (x < 0) x += PI_2;
    
    float index = (x / PI_2) * 1024.0f;
    int idx = (int)index;
    float frac = index - idx;
    
    if (idx >= 1023) 
        return vocoder.sine_table[0] * frac + vocoder.sine_table[1023] * (1.0f - frac);
    return vocoder.sine_table[idx] * (1.0f - frac) + vocoder.sine_table[idx + 1] * frac;
}

static inline float apply_pitch_shift(float sample) {
    if (vocoder.pitch_shift == 1.0f) {
        return sample;
    }

    // store sample in circular buffer???
    vocoder.pitch_buffer[vocoder.pitch_buffer_index] = sample;
    vocoder.pitch_buffer_index = (vocoder.pitch_buffer_index + 1) % PITCH_BUFFER_SIZE;

    // compute read position for pitch shifting??
    float read_pos_float = vocoder.pitch_buffer_index - PITCH_BUFFER_SIZE * vocoder.pitch_shift;
    
    // handle negative positions by wrapping around
    while (read_pos_float < 0) {
        read_pos_float += PITCH_BUFFER_SIZE;
    }

    int read_pos = (int)read_pos_float;
    float frac = read_pos_float - read_pos;

    // get positions with safe wraparound
    int next_pos = (read_pos + 1) % PITCH_BUFFER_SIZE;
    read_pos = read_pos % PITCH_BUFFER_SIZE;

    // linear interpolation between adjacent samples
    float prev = vocoder.pitch_buffer[read_pos];
    float next = vocoder.pitch_buffer[next_pos];

    return prev + (next - prev) * frac;
}

static inline float apply_distortion(float sample) {
    if (!vocoder.distortion_enabled) return sample;
    
    // Hard clipping with oversampling
    sample *= 3.0f;
    if (sample > 0.95f) sample = 0.95f;
    else if (sample < -0.95f) sample = -0.95f;
    
    // Soft clipping
    return sample - (sample*sample*sample)/3.0f;
}

static inline float apply_reverb(float sample) {
    if (!vocoder.reverb_enabled) return sample;
    
    #define REVERB_TAPS 4
    const int delays[REVERB_TAPS] = {50, 96, 31, 67};
    const float gains[REVERB_TAPS] = {0.6f, 0.4f, 0.3f, 0.2f};
    
    float reverb_out = 0;
    for (int i = 0; i < REVERB_TAPS; i++) {
        int pos = (vocoder.reverb_index - delays[i] + REVERB_BUFFER_SIZE) % REVERB_BUFFER_SIZE;
        reverb_out += vocoder.reverb_buffer[pos] * gains[i];
    }
    
    vocoder.reverb_buffer[vocoder.reverb_index] = sample + reverb_out * 0.5f;
    vocoder.reverb_index = (vocoder.reverb_index + 1) % REVERB_BUFFER_SIZE;
    
    return (sample * 0.6f + reverb_out * 0.4f);
}

static void process_vocoder(const float* modulator, float* carrier, unsigned long n) {
    float phase_carrier = vocoder.phase_carrier;
    float phase_formant = vocoder.phase_formant;
    
    for (unsigned long i = 0; i < n; ++i) {
        float carr = 0.5f * fast_sin(phase_carrier) + 0.3f * fast_sin(phase_formant);
        carrier[i] = carr * modulator[i] * 1.8f;
        
        phase_carrier += (PI_2 * CARRIER_FREQ / SAMPLE_RATE);
        phase_formant += (PI_2 * FORMANT_FREQ / SAMPLE_RATE);
        
        if (phase_carrier >= PI_2) phase_carrier -= PI_2;
        if (phase_formant >= PI_2) phase_formant -= PI_2;
    }
    
    vocoder.phase_carrier = phase_carrier;
    vocoder.phase_formant = phase_formant;
}

static void swap_buffers(void) {
    float* temp = vocoder.write_buffer;
    vocoder.write_buffer = vocoder.read_buffer;
    vocoder.read_buffer = temp;
}

static int audio_callback(const void* input, void* output, 
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo* time_info, 
                         PaStreamCallbackFlags status_flags, 
                         void* user_data) {
    (void)time_info; (void)status_flags; (void)user_data;
    
    const int16_t* in = (const int16_t*)input;
    int16_t* out = (int16_t*)output;
    
    float modulator[FRAMES_PER_BUFFER];
    
    if (vocoder.test_mode_enabled) {
        // Generate sine wave for testing
        static float phase = 0.0f;
        float phase_inc = PI_2 * vocoder.test_frequency / SAMPLE_RATE;
        
        for (unsigned long i = 0; i < frame_count; ++i) {
            modulator[i] = fast_sin(phase) * 0.8f;  // Scale to avoid clipping
            phase += phase_inc;
            if (phase >= PI_2) phase -= PI_2;
        }
    } else {
        // Normal mode - use microphone input
        if (input == NULL) return paContinue;
        
        for (unsigned long i = 0; i < frame_count; ++i) {
            modulator[i] = in[i] / SAMPLE_MAX;
        }
    }
    
    process_vocoder(modulator, vocoder.write_buffer, frame_count);
    
    for (unsigned long i = 0; i < frame_count; ++i) {
        float sample = vocoder.write_buffer[i];
        sample = apply_pitch_shift(sample);
        sample = apply_distortion(sample);
        sample = apply_reverb(sample);
        
        // Final clipping
        if (sample > 0.95f) sample = 0.95f;
        else if (sample < -0.95f) sample = -0.95f;
        
        out[i] = (int16_t)(sample * SAMPLE_MAX);
    }
    
    if (vocoder.recording_enabled && vocoder.recording_file && vocoder.recording_frames_left > 0) {
        int frames_to_write = (frame_count < vocoder.recording_frames_left) ? 
                              frame_count : vocoder.recording_frames_left;
        sf_write_short(vocoder.recording_file, out, frames_to_write);
        vocoder.recording_frames_left -= frames_to_write;
        
        if (vocoder.recording_frames_left <= 0 || stop_recording) {
            printf("Recording complete.\n");
            cleanup_recording();
        }
    }
    
    swap_buffers();
    return paContinue;
}

static void parse_arguments(int argc, char* argv[]) {
    vocoder.distortion_enabled = false;
    vocoder.reverb_enabled = false;
    vocoder.pitch_shift = 1.0f;
    vocoder.recording_enabled = false;
    vocoder.recording_seconds = 0;
    char* output_filename = NULL;

    vocoder.test_mode_enabled = false;
    vocoder.test_frequency = 440.0f;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            vocoder.distortion_enabled = true;
        } 
        else if (strcmp(argv[i], "-r") == 0) {
            vocoder.reverb_enabled = true;
        } 
        else if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            vocoder.pitch_shift = atof(argv[++i]);
            if (vocoder.pitch_shift < 0.5f) vocoder.pitch_shift = 0.5f;
            if (vocoder.pitch_shift > 2.0f) vocoder.pitch_shift = 2.0f;
        }
        else if (strcmp(argv[i], "-record") == 0 && i+1 < argc) {
            vocoder.recording_enabled = true;
            vocoder.recording_seconds = atoi(argv[++i]);
            if (vocoder.recording_seconds <= 0) {
                fprintf(stderr, "Invalid recording duration. Using 10 seconds.\n");
                vocoder.recording_seconds = 10;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            output_filename = argv[++i];
        } else if (strcmp(argv[i], "-test") == 0) {
            vocoder.test_mode_enabled = true;
            if (i+1 < argc && atof(argv[i+1]) > 0) {
                vocoder.test_frequency = atof(argv[++i]);
            }
        }
    }
    
    if (vocoder.recording_enabled) {
        if (!init_recording(output_filename)) {
            fprintf(stderr, "Failed to initialize recording. Continuing without recording.\n");
            vocoder.recording_enabled = false;
        }
    }
}

static void init_sine_table(void) {
    for (int i = 0; i < 1024; i++) {
        vocoder.sine_table[i] = sinf((float)i * PI_2 / 1024.0f);
    }
}

static bool init_buffers(void) {
    vocoder.buffer_a = (float*)calloc(FRAMES_PER_BUFFER, sizeof(float));
    vocoder.buffer_b = (float*)calloc(FRAMES_PER_BUFFER, sizeof(float));
    
    if (!vocoder.buffer_a || !vocoder.buffer_b) {
        fprintf(stderr, "Failed to allocate audio buffers\n");
        return false;
    }
    
    vocoder.write_buffer = vocoder.buffer_a;
    vocoder.read_buffer = vocoder.buffer_b;
    
    // Initialize effects
    memset(vocoder.reverb_buffer, 0, REVERB_BUFFER_SIZE * sizeof(float));
    vocoder.reverb_index = 0;
    
    memset(vocoder.pitch_buffer, 0, PITCH_BUFFER_SIZE * sizeof(float));
    vocoder.pitch_buffer_index = 0;
    vocoder.pitch_phase_accum = 0.0f;
    
    init_sine_table();
    
    if (vocoder.recording_enabled) {
        vocoder.recording_buffer = (int16_t*)calloc(FRAMES_PER_BUFFER, sizeof(int16_t));
        if (!vocoder.recording_buffer) {
            fprintf(stderr, "Failed to allocate recording buffer\n");
            return false;
        }
    }
    
    return true;
}

static bool init_recording(const char* filename) {
    memset(&vocoder.recording_info, 0, sizeof(SF_INFO));
    vocoder.recording_info.samplerate = SAMPLE_RATE;
    vocoder.recording_info.channels = NUM_CHANNELS;
    vocoder.recording_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    
    vocoder.recording_frames_left = vocoder.recording_seconds * SAMPLE_RATE;
    
    const char* output_file = filename ? filename : DEFAULT_OUTPUT_FILE;
    
    vocoder.recording_file = sf_open(output_file, SFM_WRITE, &vocoder.recording_info);
    if (!vocoder.recording_file) {
        fprintf(stderr, "Error opening output file %s: %s\n", 
                output_file, sf_strerror(NULL));
        return false;
    }
    
    printf("Recording to %s for %d seconds...\n", output_file, vocoder.recording_seconds);
    return true;
}

static void cleanup_recording(void) {
    if (vocoder.recording_file) {
        sf_close(vocoder.recording_file);
        vocoder.recording_file = NULL;
    }
    vocoder.recording_enabled = false;
}

static void handle_sigint(int sig) {
    (void)sig;
    printf("\nInterrupting recording...\n");
    stop_recording = true;
}

int main(int argc, char* argv[]) {
    PaError err;
    PaStreamParameters inputParams, outputParams;
    PaStream* stream;
    
    signal(SIGINT, handle_sigint);
    
    parse_arguments(argc, argv);
    
    if (!init_buffers()) {
        cleanup_recording();
        return EXIT_FAILURE;
    }
    
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        cleanup_recording();
        return EXIT_FAILURE;
    }

    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = NUM_CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;
    
    outputParams.device = Pa_GetDefaultOutputDevice();
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
        NULL);
    
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        cleanup_recording();
        return EXIT_FAILURE;
    }
    
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        cleanup_recording();
        return EXIT_FAILURE;
    }
    
    const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream);
    
    printf("Vocoder Running - Press Enter to exit...\n");
    printf("Buffer Size: %d samples (%.1f ms)\n", 
        FRAMES_PER_BUFFER, 
        (float)FRAMES_PER_BUFFER * 1000.0f / SAMPLE_RATE);
    printf("Effects: %s%s%s\n",
        vocoder.distortion_enabled ? "[Distortion] " : "",
        vocoder.reverb_enabled ? "[Reverb] " : "",
        vocoder.pitch_shift != 1.0f ? "[Pitch Shift] " : "");
        
    if (vocoder.test_mode_enabled) {
        printf("Test Mode: Sine wave at %.1f Hz\n", vocoder.test_frequency);
    }
    if (vocoder.recording_enabled) {
        printf("Recording: %d seconds\n", vocoder.recording_seconds);
    }
    
    getchar();
    
    stop_recording = true;
    cleanup_recording();
    
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    free(vocoder.buffer_a);
    free(vocoder.buffer_b);
    if (vocoder.recording_buffer) {
        free(vocoder.recording_buffer);
    }
    
    return EXIT_SUCCESS;
}
