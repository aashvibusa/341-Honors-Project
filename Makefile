CC = gcc
PORTAUDIO_DIR = C:/Users/ronit/Downloads/portaudio

CFLAGS = -Wall -Wextra -O2 -std=c11 -Isrc -I$(PORTAUDIO_DIR)/include
LDFLAGS = -lm -lpthread -lasound -L$(PORTAUDIO_DIR)/lib -lportaudio

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = vocoder

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
