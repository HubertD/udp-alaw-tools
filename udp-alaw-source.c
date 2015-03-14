#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <alsa/asoundlib.h>

#define UDP_TARGET_HOST   "127.0.0.1"
#define UDP_TARGET_PORT   "4001"
#define UDP_PACKET_SIZE      64
#define ALSA_SAMPLE_RATE   8000
#define ALSA_CHANNELS         1

main (int argc, char *argv[]) {

	int err;
	int buffer_frames = 128;

	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags    = AI_ADDRCONFIG;

	struct addrinfo* res=0;
	getaddrinfo(UDP_TARGET_HOST, UDP_TARGET_PORT, &hints, &res);

	int sock = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (sock==-1) {
		fprintf(stderr, "cannot create udp socket (%s)\n", strerror(errno));
		return -1;
	}

	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_A_LAW;

	if ((err = snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", argv[1], snd_strerror(err));
		exit (1);
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		exit (1);
	}

	snd_pcm_hw_params_any(capture_handle, hw_params);
	snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(capture_handle, hw_params, format);
	snd_pcm_hw_params_set_rate(capture_handle, hw_params, ALSA_SAMPLE_RATE, 0);
	snd_pcm_hw_params_set_channels(capture_handle, hw_params, ALSA_CHANNELS);
	snd_pcm_hw_params_set_period_size(capture_handle, hw_params, 32, 0);
	snd_pcm_hw_params(capture_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		exit (1);
	}


	uint8_t buffer[UDP_PACKET_SIZE];
	while (1) {
		if ((err = snd_pcm_readi(capture_handle, buffer, sizeof(buffer))) != sizeof(buffer)) {
			fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
			break;
		}

		if (sendto(sock, buffer, sizeof(buffer), 0, res->ai_addr,res->ai_addrlen)==-1) {
			fprintf(stderr, "udp sendto failed(%s)\n", strerror(errno));
			break;
		}
	}

	snd_pcm_close(capture_handle);
	exit(0);
}
