#include "network.h"
#include "effects.h"
#include "constants.h"
#include "vocoder.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define AUDIO_PORT 5555
#define CONTROL_PORT 5556
#define AUDIO_BUFFER_SIZE 30000
#define SAMPLE_RATE 44100
#define CHANNELS 1

volatile int g_current_effect = EFFECT_NONE; // volatile to allow for changes between threads
float current_pitch = 1.0f;

// send effect changes for client/receiver to display
void* send_effect_control(void* arg) {
    const char* ip = (const char*)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in ctrl_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONTROL_PORT),
        .sin_addr.s_addr = inet_addr(ip)
    };

    if (connect(sock, (struct sockaddr*) &ctrl_addr, sizeof(ctrl_addr)) < 0) {
        perror("Control socket connect failed");
        return NULL;
    }

    int last_effect = -1;

    // check for effect changes and send updates
    while (1) {
        if (g_current_effect != last_effect) {
            const char* msg = effect_to_string(g_current_effect);
            send(sock, msg, strlen(msg) + 1, 0);
            last_effect = g_current_effect;
        }
        sleep(1);
    }

    close(sock);
    return NULL;
}

// capture audio and send to client with applied effects
void* audio_stream_client(void* arg) {
    const char* ip = (const char*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(AUDIO_PORT),
        .sin_addr.s_addr = inet_addr(ip)
    };

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Audio socket connect failed");
        return NULL;
    }

    snd_pcm_t* capture_handle;

    // open default ALSA capture device
    snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    snd_pcm_set_params(capture_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        1,
        500000); // 0.5 sec latency

    int16_t raw_buffer[AUDIO_BUFFER_SIZE];
    float input_f[AUDIO_BUFFER_SIZE / 2];
    float output_f[AUDIO_BUFFER_SIZE / 2];

    while (1) {
        snd_pcm_readi(capture_handle, raw_buffer, AUDIO_BUFFER_SIZE / 2); // read audio input

        for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++)
            input_f[i] = raw_buffer[i] / 32768.0f; //convert to float

        switch (g_current_effect) {
            case EFFECT_LOW:
                process_pitch_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2, 0.6f);
                break;
            case EFFECT_HIGH:
                process_pitch_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2, 1.4f);
                break;
            case EFFECT_PITCH:
                process_pitch_effect(input_f, output_f, AUDIO_BUFFER_SIZE / 2, current_pitch);
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
                for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++)
                    output_f[i] = input_f[i];
                break;
        }

        // converts output to int16_t and clips to a valid range
        for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
            float sample = output_f[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            raw_buffer[i] = (int16_t)(sample * 32767.0f);
        }

        // sends the processed audio to receiver
        send(sock, raw_buffer, AUDIO_BUFFER_SIZE, 0);
    }

    snd_pcm_close(capture_handle);
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <receiver_ip>\n", argv[0]);
        return 1;
    }

    const char* receiver_ip = argv[1];

    // create thread for audio and thread for control
    pthread_t audio_thread, control_thread;
    pthread_create(&audio_thread, NULL, audio_stream_client, (void*)receiver_ip);
    pthread_create(&control_thread, NULL, send_effect_control, (void*)receiver_ip);

    pthread_detach(audio_thread);
    pthread_detach(control_thread);

    printf("Streaming to %s\n", receiver_ip);
    printf("Available effects: none, low, high, pitch <pitch value>, wobble, robot, echo\n");

    char input[32];
    while (1) {
        printf("Effect > ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        char effect_str[32];
        float val;
        int matched = sscanf(input, "%s %f", effect_str, &val);

        // match effects using strcmp
        if(matched > 1 && strcmp(effect_str, "pitch") == 0){
            g_current_effect = EFFECT_PITCH;
            current_pitch = val;
        }
        else if (strcmp(effect_str, "low") == 0)
            g_current_effect = EFFECT_LOW;
        else if (strcmp(effect_str, "high") == 0)
            g_current_effect = EFFECT_HIGH;
        else if (strcmp(effect_str, "wobble") == 0)
            g_current_effect = EFFECT_WOBBLE;
        else if (strcmp(effect_str, "robot") == 0)
            g_current_effect = EFFECT_ROBOT;
        else if (strcmp(effect_str, "echo") == 0)
            g_current_effect = EFFECT_ECHO;
        else
            g_current_effect = EFFECT_NONE;
    }

    return 0;
}
