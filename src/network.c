#include "network.h"
#include "vocoder.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

volatile int g_current_effect = EFFECT_NONE;

#define TCP_PORT 9999

static void* network_server(void* arg) {
    (void)arg;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        char buf[32] = {0};
        recv(client_fd, buf, sizeof(buf), 0);

        if (strcmp(buf, "low\n") == 0) {
            g_current_effect = EFFECT_LOW;
        } else if (strcmp(buf, "wobble\n") == 0) {
            g_current_effect = EFFECT_WOBBLE;
        } else if (strcmp(buf, "robot\n") == 0) {
            g_current_effect = EFFECT_ROBOT;
        } else if (strcmp(buf, "echo\n") == 0) {
            g_current_effect = EFFECT_ECHO;
        } else {
            g_current_effect = EFFECT_NONE;
        }

        printf("Effect changed to: %s\n", effect_to_string(g_current_effect));
        
        close(client_fd);
    }
    return NULL;
}

void start_network_servers() {
    pthread_t net_thread;
    pthread_create(&net_thread, NULL, network_server, NULL);
    pthread_detach(net_thread);
}
