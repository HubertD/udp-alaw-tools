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
#include <time.h>

#define UDP_TARGET_HOST   "127.0.0.1"
#define UDP_TARGET_PORT   "4001"
#define UDP_PACKET_SIZE      64
#define FILE_SAMPLE_RATE   8000

int8_t alaw_encode(int16_t number)
{
	/*
	*  Taken from:
	*  http://dystopiancode.blogspot.de/2012/02/pcm-law-and-u-law-companding-algorithms.html
	*/

	const uint16_t ALAW_MAX = 0xFFF;
	uint16_t mask = 0x800;
	uint8_t sign = 0;
	uint8_t position = 11;
	uint8_t lsb = 0;
	if (number < 0)	{
		number = -number;
		sign = 0x80;
	}
	if (number > ALAW_MAX) {
		number = ALAW_MAX;
	}
	for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--);
	lsb = (number >> ((position == 4) ? (1) : (position - 4))) & 0x0f;

	return (sign | ((position - 4) << 4) | lsb) ^ 0x55;
}

int main(int argc, char **argv) {

	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags    = AI_ADDRCONFIG;

	struct addrinfo* res=0;
	getaddrinfo(UDP_TARGET_HOST, UDP_TARGET_PORT, &hints, &res);

	int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (fd==-1) {
		puts(strerror(errno));
		return -1;
	}

	int readfd = open(argv[1], O_RDONLY);
	if (!readfd) { perror("could not open file"); }

	struct timespec t_next_send;
	clock_gettime(CLOCK_MONOTONIC, &t_next_send);

	int frame_inteval_us  = 1000000 / FILE_SAMPLE_RATE;
	int frames_per_packet  = UDP_PACKET_SIZE;
	int packet_interval_us = frames_per_packet * frame_inteval_us;


	int16_t readbuf[UDP_PACKET_SIZE];
	int8_t  sendbuf[UDP_PACKET_SIZE];

	while (1) {
		int bytes_read = read(readfd, readbuf, sizeof(readbuf));
		if (bytes_read<=0) { break; }

		int i;
		for (i=0; i<UDP_PACKET_SIZE; i++) {
			sendbuf[i] = alaw_encode(readbuf[i]/8);
		}

		if (sendto(fd, sendbuf, bytes_read/2, 0, res->ai_addr,res->ai_addrlen)==-1) {
			puts(strerror(errno));
			break;
		}

		t_next_send.tv_nsec += 1000*packet_interval_us;
		while (t_next_send.tv_nsec>1000000000UL) {
			t_next_send.tv_nsec -= 1000000000UL;
			t_next_send.tv_sec += 1;
		}

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next_send, NULL);
	}

	close(readfd);

	return 0;

}
