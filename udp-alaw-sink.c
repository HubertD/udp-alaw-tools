/*
 * udp-alaw-sink.c
 *
 *  Created on: 14.03.2015
 *      Author: hd
 */


#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define ALSA_SAMPLE_RATE    8000
#define ALSA_CHANNELS          1
#define UDP_ALAW_PORT       4001
#define UDP_MAX_PACKET_SIZE 1024

int main() {

	/* create UDP socket */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)   {
		perror("Opening datagram socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(UDP_ALAW_PORT);

	if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr))) {
		perror("binding datagram socket");
		exit(1);
	}

	/* open PCM device */
	snd_pcm_t *pcm_handle;
	int rc = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* set hardware paramters */
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(pcm_handle, hw_params);
	snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_A_LAW);
	snd_pcm_hw_params_set_channels(pcm_handle, hw_params, ALSA_CHANNELS);
	snd_pcm_hw_params_set_rate(pcm_handle, hw_params, ALSA_SAMPLE_RATE, 0);
	snd_pcm_hw_params_set_period_size(pcm_handle, hw_params, 32, 0);
	rc = snd_pcm_hw_params(pcm_handle, hw_params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* set software paramters */
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sw_params_malloc(&sw_params);
	snd_pcm_sw_params_current(pcm_handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, 64);
	rc = snd_pcm_sw_params(pcm_handle, sw_params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}
	snd_pcm_sw_params_free(sw_params);


	uint8_t buffer[UDP_MAX_PACKET_SIZE];
	struct sockaddr_in client_addr;

	while (1) {
		socklen_t client_addr_len = sizeof(client_addr);
		int send_bytes_avail = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
		int send_pos = 0;
		while (send_bytes_avail>0) {
			int rc = snd_pcm_writei(pcm_handle, &buffer[send_pos], send_bytes_avail);
			if (rc>0) {
				send_bytes_avail -= rc;
				send_pos += rc;
			} else if (rc == -EPIPE) {
				/* EPIPE means underrun */
				snd_pcm_prepare(pcm_handle);
			} else { // other error
				break;
			}
		}
	}

	close(sock);
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);

	return 0;
}
