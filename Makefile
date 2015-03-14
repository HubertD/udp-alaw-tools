all:
	gcc -c udp-alaw-sink.c -o obj/udp-alaw-sink.o 
	gcc -o bin/udp-alaw-sink obj/udp-alaw-sink.o -lasound

	gcc -c udp-alaw-play.c  -o obj/udp-alaw-play.o
	gcc -o bin/udp-alaw-play obj/udp-alaw-play.o

clean:
	rm bin/*
	rm obj/*.o
