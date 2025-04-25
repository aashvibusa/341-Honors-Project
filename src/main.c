#include "audio_io.h"
#include "network.h"
#include "vocoder.h"
#include <stdio.h>
#include <fcntl.h>   
#include <unistd.h>  
#include <stdlib.h>   

int main() {
    initialize_vocoder();

    int fd = open("error.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if(fd == -1){
        perror("unable to write");
        exit(2);
    }

    dup2(fd, STDERR_FILENO);
    close(fd);   
    
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
