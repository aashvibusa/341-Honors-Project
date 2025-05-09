#include "network.h"
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

// ========== Audio Streaming Server (Receiver) ==========

static void* audio_stream_server(void* arg) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(AUDIO_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    // Accept connection from the sender
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Failed to accept client connection");
        return NULL;
    }

    // Set up ALSA playback
    snd_pcm_t* playback_handle;
    snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_set_params(playback_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        1,
        500000);

    int16_t raw_buffer[AUDIO_BUFFER_SIZE];

    while (1) {
        // Receive the audio data from the sender
        int bytes_received = recv(client_fd, raw_buffer, AUDIO_BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;  // If connection is closed or error

        // Play the received audio
        snd_pcm_writei(playback_handle, raw_buffer, AUDIO_BUFFER_SIZE / 2);
    }

    // Close ALSA and socket when done
    snd_pcm_close(playback_handle);
    close(client_fd);
    close(server_fd);
    return NULL;
}

void start_audio_receiver() {
    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, audio_stream_server, NULL);
    pthread_detach(audio_thread);
}

// ========== Main Function ==========

int main() {
    // Start the audio receiver server to receive audio
    start_audio_receiver();

    // Keep the program running to continue receiving audio
    printf("Receiver server started. Waiting for audio stream...\n");
    
    // Infinite loop to keep the receiver running
    while (1) {
        // Placeholder to keep the program alive
        // You can add additional logic here if needed
    }

    return 0;
}
