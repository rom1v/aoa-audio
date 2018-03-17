FLAGS += -Wall -Wextra

audio: audio.c
	${CC} ${FLAGS} audio.c -lusb-1.0 -o audio

clean:
	rm audio
