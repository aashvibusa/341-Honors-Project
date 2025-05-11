// network.c - Sender

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
#include <sys/socket.h>
#include <netinet/tcp.h>

#define OPTIMAL_BUFFER_SIZE 512  // Smaller buffer for reduced latency
#define MAX_EFFECT_NAME_LEN 32

volatile int g_current_effect = EFFECT_NONE;
float current_pitch = 1.0f;

// Thread-safe effect change flag
volatile int effect_changed = 0;
pthread_mutex_t effect_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread: Send effect name changes to receiver
void* send_effect_control(void* arg) {
    const char* ip = (const char*)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Set TCP_NODELAY to disable Nagle's algorithm and reduce latency
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    
    struct sockaddr_in ctrl_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONTROL_PORT),
        .sin_addr.s_addr = inet_addr(ip)
    };

    if (connect(sock, (struct sockaddr*)&ctrl_addr, sizeof(ctrl_addr)) < 0) {
        perror("Control socket connect failed");
        return NULL;
    }

    int last_effect = -1;

    while (1) {
        int current_effect;
        pthread_mutex_lock(&effect_mutex);
        current_effect = g_current_effect;
        int changed = effect_changed;
        if (changed) effect_changed = 0;
        pthread_mutex_unlock(&effect_mutex);

        if (changed || current_effect != last_effect) {
            const char* msg = effect_to_string(current_effect);
            send(sock, msg, strlen(msg) + 1, 0);
            last_effect = current_effect;
        }
        usleep(50000);  // Check every 50ms instead of sleeping for 1 second
    }

    close(sock);
    return NULL;
}

// Thread: Audio capture and sending
void* audio_stream_client(void* arg) {
    const char* ip = (const char*)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Set TCP_NODELAY to disable Nagle's algorithm
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    
    // Increase socket buffer size
    int sndbuf = 65536;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    
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
    if (snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
        perror("Failed to open audio capture device");
        close(sock);
        return NULL;
    }
    
    // Configure for low latency
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(capture_handle, hw_params);
    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, CHANNELS);
    snd_pcm_hw_params_set_rate(capture_handle, hw_params, SAMPLE_RATE, 0);
    snd_pcm_hw_params_set_buffer_size(capture_handle, hw_params, OPTIMAL_BUFFER_SIZE * 4);
    snd_pcm_hw_params_set_period_size(capture_handle, hw_params, OPTIMAL_BUFFER_SIZE, 0);
    snd_pcm_hw_params(capture_handle, hw_params);
    
    // Prefill buffer once to reduce initial processing delay
    static int16_t raw_buffer[OPTIMAL_BUFFER_SIZE];
    static float input_f[OPTIMAL_BUFFER_SIZE];
    static float output_f[OPTIMAL_BUFFER_SIZE];
    
    // Pre-initialize buffers - reset to avoid stale data
    memset(raw_buffer, 0, sizeof(raw_buffer));
    memset(input_f, 0, sizeof(input_f));
    memset(output_f, 0, sizeof(output_f));

    while (1) {
        // Read audio samples with error handling
        int read_frames = snd_pcm_readi(capture_handle, raw_buffer, OPTIMAL_BUFFER_SIZE);
        
        if (read_frames < 0) {
            if (read_frames == -EPIPE) {
                snd_pcm_prepare(capture_handle);
                fprintf(stderr, "Buffer underrun occurred\n");
                continue;
            } else {
                fprintf(stderr, "Error reading from audio device: %s\n", 
                        snd_strerror(read_frames));
                break;
            }
        }
        
        // Process only frames actually read
        for (int i = 0; i < read_frames; i++)
            input_f[i] = raw_buffer[i] / 32768.0f;
        
        int current_effect;
        float pitch_value;

        // Thread-safe effect access
        pthread_mutex_lock(&effect_mutex);
        current_effect = g_current_effect;
        pitch_value = current_pitch;
        pthread_mutex_unlock(&effect_mutex);

        switch (current_effect) {
            case EFFECT_LOW:
                process_pitch_effect(input_f, output_f, read_frames, 0.6f);
                break;
            case EFFECT_HIGH:
                process_pitch_effect(input_f, output_f, read_frames, 1.4f);
                break;
            case EFFECT_PITCH:
                process_pitch_effect(input_f, output_f, read_frames, pitch_value);
                break;
            case EFFECT_WOBBLE:
                process_wobble_effect(input_f, output_f, read_frames);
                break;
            case EFFECT_ROBOT:
                process_robot_effect(input_f, output_f, read_frames);
                break;
            case EFFECT_ECHO:
                process_echo_effect(input_f, output_f, read_frames);
                break;
            default:
                // Fast path - direct memcpy for EFFECT_NONE
                memcpy(output_f, input_f, read_frames * sizeof(float));
                break;
        }

        // Faster conversion with SIMD-friendly loop
        for (int i = 0; i < read_frames; i++) {
            // Clamp values efficiently
            float sample = output_f[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            raw_buffer[i] = (int16_t)(sample * 32767.0f);
        }

        // Use MSG_NOSIGNAL to prevent SIGPIPE on connection loss
        int bytes_to_send = read_frames * sizeof(int16_t);
        int bytes_sent = send(sock, raw_buffer, bytes_to_send, MSG_NOSIGNAL);
        
        if (bytes_sent < 0) {
            perror("Failed to send audio data");
            break;
        }
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
    
    // Initialize mutex
    pthread_mutex_init(&effect_mutex, NULL);

    pthread_t audio_thread, control_thread;
    pthread_create(&audio_thread, NULL, audio_stream_client, (void*)receiver_ip);
    pthread_create(&control_thread, NULL, send_effect_control, (void*)receiver_ip);

    pthread_detach(audio_thread);
    pthread_detach(control_thread);

    printf("Streaming to %s\n", receiver_ip);
    printf("Available effects: none, low, high, pitch <pitch value>, wobble, robot, echo\n");

    char input[MAX_EFFECT_NAME_LEN];
    while (1) {
        printf("Effect > ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        
        char effect_str[MAX_EFFECT_NAME_LEN];
        float val = 1.0f;
        int matched = sscanf(input, "%31s %f", effect_str, &val);

        pthread_mutex_lock(&effect_mutex);
        
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
            
        effect_changed = 1;
        pthread_mutex_unlock(&effect_mutex);
    }

    pthread_mutex_destroy(&effect_mutex);
    return 0;
}