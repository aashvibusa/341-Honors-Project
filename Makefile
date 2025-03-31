CC=gcc
CFLAGS=-Wall -Wextra -Iinclude
SRC=src/main.c src/audio_processing.c src/effects.c src/vocoder.c
OBJ=$(SRC:.c=.o)
TARGET=build/vocoder

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lm -lportaudio

clean:
	rm -f $(OBJ) $(TARGET)
