// handle audio callback from mic
#include "audio_io.h"
#include "vocoder.h"
#include "effects.h"
#include <stdio.h>
#include <string.h>
#include "network.h"

static PaStream *stream;

int audio_callback(const void *input, void *output,
                   unsigned long frame_count,
                   const PaStreamCallbackTimeInfo *time_info,
                   PaStreamCallbackFlags status_flags,
                   void *user_data)
{
    (void)time_info;
    (void)status_flags;
    (void)user_data;

    const float *in = (const float *)input;
    float *out = (float *)output;

    if (!in)
        return paContinue;

    VocoderState *v = get_vocoder();
    v->effect_type = g_current_effect;

    /*static int count = 0;
    if (count++ % 100 == 0) {
        printf("Audio: in=%.3f out=%.3f (Effect=%d)\n",
               in[0], out[0], v->effect_type);
    }*/

    switch (g_current_effect)
    {
    case EFFECT_LOW:
        process_low_effect(in, out, frame_count);
        break;
    case EFFECT_WOBBLE:
        process_wobble_effect(in, out, frame_count);
        break;
    case EFFECT_ROBOT:
        process_robot_effect(in, out, frame_count);
        break;
    case EFFECT_ECHO:
        process_echo_effect(in, out, frame_count);
        break;
    default:
        memcpy(out, in, frame_count * sizeof(float));
    }

    return paContinue;
}

int init_audio()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
        return 1;

    PaStreamParameters input_params = {
        .device = Pa_GetDefaultInputDevice(),
        .channelCount = 1,
        .sampleFormat = paFloat32,
        .suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice())->defaultLowInputLatency,
        .hostApiSpecificStreamInfo = NULL};

    PaStreamParameters output_params = {
        .device = Pa_GetDefaultOutputDevice(),
        .channelCount = 1,
        .sampleFormat = paFloat32,
        .suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency,
        .hostApiSpecificStreamInfo = NULL};

    err = Pa_OpenStream(&stream, &input_params, &output_params,
                        SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff,
                        audio_callback, NULL);
    if (err != paNoError)
        return 1;

    return Pa_StartStream(stream);
}

void close_audio()
{
    if (stream)
    {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}