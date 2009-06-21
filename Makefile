
CFLAGS := -g -Wall -O2

all: playwav

playwav: audio_alsa.c playwav.c
	gcc $(CFLAGS) -o playwav playwav.c audio_alsa.c

clean:
	rm -f playwav *~
