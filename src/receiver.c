#include "network.h"
#include "constants.h"
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

// listen for audio stream
void *audio_stream_server(void *arg)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(AUDIO_PORT),
        .sin_addr.s_addr = INADDR_ANY};

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 1);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("Audio accept failed");
        return NULL;
    }

    snd_pcm_t *playback_handle;
    snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_set_params(playback_handle,
                       SND_PCM_FORMAT_S16_LE,
                       SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS,
                       SAMPLE_RATE,
                       1,
                       500000);

    int16_t raw_buffer[AUDIO_BUFFER_SIZE];

    while (1)
    {
        int bytes = recv(client_fd, raw_buffer, AUDIO_BUFFER_SIZE, 0);
        if (bytes <= 0)
            break;

        snd_pcm_writei(playback_handle, raw_buffer, AUDIO_BUFFER_SIZE / 2);
    }

    snd_pcm_close(playback_handle);
    close(client_fd);
    close(server_fd);
    return NULL;
}

// check for effect changes and display
void *receive_effect_control(void *arg)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ctrl_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CONTROL_PORT),
        .sin_addr.s_addr = INADDR_ANY};

    bind(sock, (struct sockaddr *)&ctrl_addr, sizeof(ctrl_addr));
    listen(sock, 1);

    int client = accept(sock, NULL, NULL);
    char effect[32];

    while (1)
    {
        int n = recv(client, effect, sizeof(effect) - 1, 0);
        if (n <= 0)
            break;
        effect[n] = '\0';
        printf("Effect changed to: %s\n", effect);
    }

    close(client);
    close(sock);
    return NULL;
}

int main()
{
    // create threads to listen for audio and accept change effects simultaneously
    pthread_t audio_thread, control_thread;
    pthread_create(&audio_thread, NULL, audio_stream_server, NULL);
    pthread_create(&control_thread, NULL, receive_effect_control, NULL);

    pthread_detach(audio_thread);
    pthread_detach(control_thread);

    printf("Receiver started. Listening for audio and effect changes...\n");

    while (1)
    {
        // keep program in execution
    }

    return 0;
}
