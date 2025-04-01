#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <portaudio.h>
#include "vocoder.h"

#define SAMPLE_RATE 44100
// #define FRAMES_PER_BUFFER 256  // Increased for better stability
#define NUM_CHANNELS 1

int init_audio();
void close_audio();
int audio_callback(const void* input, void* output, 
                  unsigned long frame_count,
                  const PaStreamCallbackTimeInfo* time_info,
                  PaStreamCallbackFlags status_flags, 
                  void* user_data);

#endif
