# Realtime Networked TCP Vocoder 

## Project Overview

This project is a real-time vocoder audio processing system that enables voice transformation through multiple audio effects. It features a client-server architecture for networked audio transmission and remote control of effects. The system can process microphone input in real-time, apply various voice modulation effects, and stream the processed audio either locally or through a network.

### Key Features

- **Multiple Audio Effects**:
  - **Low Effect**: Pitch shifting down for a deeper voice
  - **High Effect**: Pitch shifting up for a higher voice
  - **Pitch Effect**: Customizable pitch shifting with variable rate
  - **Wobble Effect**: Amplitude modulation for a tremolo-like sound
  - **Robot Effect**: Carrier wave modulation for a mechanical voice
  - **Echo Effect**: Delay with feedback for echo/reverb effects

- **Network Capabilities**:
  - Stream processed audio over TCP/IP
  - Remote control of effects via network commands
  - Separate control and audio streaming channels

## Team Contributions

**Team Member Contributions:**

- **Aashvi**: 
  - TCP networking + connections
  - Sender/Receiver architecture
  - Raspberry Pi set-up
  - Effect creation (pitch change/robot)

- **Ronit**: 
  - Makefile creation / repo organization
  - Input parsing and processing
  - Audio streaming and buffer handling
  - Error handling and logging

- **Sneha**: 
  -TCP networking + connections 
  -Audio buffer handling
  -Multithreading
  -Effects creation (wobble/echo)

## Installation

### Prerequisites

- GCC compiler (or compatible C compiler)
- POSIX-compatible operating system (Linux/macOS)
- PortAudio library
- ALSA audio library (for Linux)
- pthread support

### Dependencies Installation

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libportaudio2 libportaudio-dev libasound2-dev
```

#### macOS:
```bash
brew install portaudio
```

### Building the Project

1. Clone the repository:
```
git clone https://github.com/your-username/realtime-networked-vocoder.git
cd realtime-networked-vocoder
```

2. Compile the project:

#### Receiver -
```
make
```

This will build the main vocoder application and receiver/sender components.

## Usage

### Network Mode

#### Receiver (Audio Output Device):
```
./receiver
```

Start this on the machine where you want audio playback.

#### Sender (Audio Input Device):
```
./sender <receiver_ip>
```

Start this on the machine with the microphone input, replacing `<receiver_ip>` with the IP address of the receiver machine.

### Effect Control

When running the sender, you'll see a command prompt where you can type effect names:

```
Effect > none    # No effect
Effect > low     # Lower pitch effect
Effect > high    # Higher pitch effect
Effect > pitch 1.5  # Custom pitch
Effect > wobble  # Amplitude modulation effect
Effect > robot   # Robotic voice effect
Effect > echo    # Echo effect
```

## Future Improvements

- Reduce latency
- Implement security features
- Add graphical user interface with real-time visualization of audio waves
