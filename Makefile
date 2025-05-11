all: sender receiver

sender:
	gcc src/network.c src/vocoder.c src/effects/effects.c -o sender -lpthread -lasound -lm

receiver:
	gcc src/receiver.c -o receiver -lpthread -lasound -lm

clean:
	rm -f sender receiver
