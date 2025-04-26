# Realtime Vocoder with Network-Controlled Effects

This project is a realtime vocoder audio processor built with PortAudio, supporting dynamic effect switching via TCP commands.

## Features

- Real-time audio processing with customizable effects:
  - **Low Effect**: Uses a circular buffer and linear interpolation to read from delayed positions in the buffer at a slower rate (leading to pitch shifting).
  - **Wobble Effect**: Applies a slow sine wave modulation to the input signal's amplitude for a tremolo-adjacent sound.
  - **Robot Effect**: Multiplies the input signal with a square wave carrier for a robotic, monotone voice.
  - **Echo Effect**: Implements a circular delay buffer with feedback, mixing delayed samples with the live input to produce echoes.
- TCP server to receive effect switch commands from external clients

## Requirements

- GCC or Clang
- PortAudio
- POSIX system (Linux/macOS)

Install dependencies (example for Ubuntu):

```bash
sudo apt install libportaudio2 libportaudio-dev
