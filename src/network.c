#include "network.h"
#include "effects.h"  // Assuming you have effect functions like process_low_effect, etc.
#include "constants.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define AUDIO_PORT 5555
#define AUDIO_BUFFER_SIZE 1024
#define SAMPLE_RATE 44100
#define CHANNELS 1

volatile int g_current_effect = EFFECT_NONE;

// ========== Audio Streaming Client (Sender) ==========

static void* audio_stream_client(void* arg) {
    const char* ip = (const char*)arg;

    // Create the socket to send audio data
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(AUDIO_PORT),
        .sin_addr.s_addr = inet_addr(ip)
    };

    // Connect to the receiver server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return NULL;
    }

    // Set up ALSA capture
    snd_pcm_t* capture_handle;
    snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    snd_pcm_set_params(capture_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        1,
        500000);

    int16_t raw_buffer[AUDIO_BUFFER_SIZE];
    float input_f[AUDIO_BUFFER_SIZE / 2];
    float output_f[AUDIO_BUFFER_SIZE / 2];

    while (1) {
        // Read audio from the capture device
        snd_pcm_readi(capture_handle, raw_buffer, AUDIO_BUFFER_SIZE / 2);

        // Convert raw audio to float
        for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
            input_f[i] = raw_buffer[i] / 32768.0f;
        }

        // Apply effect to the audio stream based on the current effect
        switch (g_current_effect) {
            case EFFECT_LOW:
                process_low_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2);
                break;
            case EFFECT_WOBBLE:
                process_wobble_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2);
                break;
            case EFFECT_ROBOT:
                process_robot_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2);
                break;
            case EFFECT_ECHO:
                process_echo_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2);
                break;
            default:
                // No effect, pass the audio unchanged
                for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
                    output_f[i] = input_f[i];
                }
                break;
        }

        // Convert processed audio back to raw PCM format
        for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
            float sample = output_f[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            raw_buffer[i] = (int16_t)(sample * 32767.0f);
        }

        // Send the processed audio to the server
        send(sock, raw_buffer, AUDIO_BUFFER_SIZE, 0);
    }

    // Close ALSA and socket when done
    snd_pcm_close(capture_handle);
    close(sock);
    return NULL;
}

void start_audio_streamer(const char* receiver_ip) {
    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, audio_stream_client, (void*)receiver_ip);
    pthread_detach(audio_thread);
}

// ========== Main Function ==========

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <receiver_ip>\n", argv[0]);
        return 1;
    }

    const char* receiver_ip = argv[1];

    // Start the audio streaming to the receiver IP
    start_audio_streamer(receiver_ip);

    // Keep the program running (e.g., for continuous audio streaming)
    // You can implement a control loop or another way to keep the program alive as needed
    printf("Audio streaming started to %s...\n", receiver_ip);
    while (1) {
        // Placeholder to keep the program running
        // You can handle other tasks or just let it stream continuously
    }

    return 0;
}
