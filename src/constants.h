#ifndef CONSTANTS_H
#define CONSTANTS_H

// Audio Constants
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 128
#define NUM_CHANNELS 1
#define SAMPLE_MAX (32767.0f)
#define SAMPLE_MIN (-32768.0f)
#define PI_2 (2.0f * 3.1415926535f)

// Network Constants
#define TCP_PORT 9999
#define AUDIO_PORT 9998

// Effect Constants
// #define ECHO_BUFFER_SIZE 44100
#define PITCH_BUFFER_SIZE 4096
#define WOBBLE_SPEED_DEFAULT 5.0f

#endif
