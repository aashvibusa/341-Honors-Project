#include "audio_io.h"
#include "network.h"
#include "vocoder.h"
#include <stdio.h>

int main() {
    initialize_vocoder();
    
    if (init_audio() != 0) {
        fprintf(stderr, "Audio initialization failed!\n");
        return 1;
    }
    
    start_network_servers();
    printf("Vocoder running - press Enter to quit...\n");
    getchar();
    
    close_audio();
    return 0;
}