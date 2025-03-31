#include <stdio.h>
#include <stdlib.h>
#include "audio_processing.h"
#include "vocoder.h"

int main(int argc, char* argv[]) {
    parseArguments(argc, argv); 

    initVocoderState(); 

    startAudioProcessing(); 

    printf("Vocoder Running... Press Enter to exit.\n");
    getchar(); 

    stopAudioProcessing(); // Stop audio processing
    freeVocoderState();

    return 0;
}