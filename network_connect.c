#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <string.h>
#include <stdbool.h>
#include <sndfile.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 128
#define NUM_CHANNELS 1
#define TCP_PORT 5000
#define AUDIO_PORT 5001

typedef enum {
    EFFECT_NONE,
    EFFECT_LOW,
    EFFECT_WOBBLE,
    EFFECT_ROBOT,
    EFFECT_ECHO
} EffectType;

typedef struct {
    EffectType effect_type;
} VocoderState;

VocoderState vocoder = {EFFECT_NONE};

void* network_server(void* arg) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(TCP_PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Server listening on port %d...\n", TCP_PORT);
    
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        recv(client_fd, buffer, sizeof(buffer), 0);

        if (strcmp(buffer, "low") == 0) vocoder.effect_type = EFFECT_LOW;
        else if (strcmp(buffer, "wobble") == 0) vocoder.effect_type = EFFECT_WOBBLE;
        else if (strcmp(buffer, "robot") == 0) vocoder.effect_type = EFFECT_ROBOT;
        else if (strcmp(buffer, "echo") == 0) vocoder.effect_type = EFFECT_ECHO;
        else vocoder.effect_type = EFFECT_NONE;
        
        printf("Effect changed to: %s\n", buffer);
        close(client_fd);
    }
    return NULL;
}

void* audio_streaming_server(void* arg) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int16_t buffer[FRAMES_PER_BUFFER];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(AUDIO_PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Audio streaming on port %d...\n", AUDIO_PORT);
    
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        printf("Client connected for audio streaming.\n");

        while (1) {
            memset(buffer, 0, sizeof(buffer));
            if (send(client_fd, buffer, sizeof(buffer), 0) <= 0) break;
        }
        close(client_fd);
        printf("Client disconnected.\n");
    }
    return NULL;
}

int main() {
    pthread_t server_thread, audio_thread;
    pthread_create(&server_thread, NULL, network_server, NULL);
    pthread_create(&audio_thread, NULL, audio_streaming_server, NULL);
    
    pthread_detach(server_thread);
    pthread_detach(audio_thread);
    
    while (1) {
        // process and play audio
    }
    return 0;
}
