/*
** Copyright 2009, Brian Swetland. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions, and the following disclaimer.
** 2. The name of the authors may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "audio.h"

/* http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
    uint16_t block_align;     /* num_channels * bps / 8 */
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int play_file(unsigned rate, unsigned channels, int fd, unsigned count)
{
    struct pcm *pcm;
    unsigned avail, xfer, bufsize;
    char *data, *next;

    data = malloc(count);
    if (!data) {
        fprintf(stderr,"could not allocate %d bytes\n", count);
        return -1;
    }
    if (read(fd, data, count) != count) {
        close(fd);
        fprintf(stderr,"could not read %d bytes\n", count);
        return -1;
    }

    close(fd);
    avail = count;
    next = data;

    pcm = pcm_alloc();
    if (pcm_open(pcm))
        goto fail;

    bufsize = pcm_buffer_size(pcm);

    while (avail > 0) {
        xfer = (avail > bufsize) ? bufsize : avail;
        if (pcm_write(pcm, next, xfer))
            goto fail;
        next += xfer;
        avail -= xfer;
    }

    pcm_close(pcm);
    return 0;
    
fail:
    fprintf(stderr,"pcm error: %s\n", pcm_error(pcm));
    return -1;
}

int play_wav(const char *fn)
{
    struct wav_header hdr;
    unsigned rate, channels;
    int fd;
    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "playwav: cannot open '%s'\n", fn);
        return -1;
    }
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "playwav: cannot read header\n");
        return -1;
    }
    fprintf(stderr,"playwav: %d ch, %d hz, %d bit, %s\n",
            hdr.num_channels, hdr.sample_rate, hdr.bits_per_sample,
            hdr.audio_format == FORMAT_PCM ? "PCM" : "unknown");
    
    if ((hdr.riff_id != ID_RIFF) ||
        (hdr.riff_fmt != ID_WAVE) ||
        (hdr.fmt_id != ID_FMT)) {
        fprintf(stderr, "playwav: '%s' is not a riff/wave file\n", fn);
        return -1;
    }
    if ((hdr.audio_format != FORMAT_PCM) ||
        (hdr.fmt_sz != 16)) {
        fprintf(stderr, "playwav: '%s' is not pcm format\n", fn);
        return -1;
    }
    if (hdr.bits_per_sample != 16) {
        fprintf(stderr, "playwav: '%s' is not 16bit per sample\n", fn);
        return -1;
    }

    return play_file(hdr.sample_rate, hdr.num_channels, fd, hdr.data_sz);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr,"usage: playwav <file>\n");
        return -1;
    }

    return play_wav(argv[1]);
}

