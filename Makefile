CC = gcc
CFLAGS = -Wall -Wextra -O2 -I/opt/homebrew/include -std=c11
LDFLAGS = -L/opt/homebrew/lib -lportaudio -lm -lpthread

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
