#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include "audio_io.h"
#include "constants.h"

void start_network_servers();

extern volatile int g_current_effect;

#endif