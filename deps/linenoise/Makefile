CFLAGS := -g

linenoise.o: linenoise.c
	$(CC) -c $(CFLAGS) $< -o $@

linenoise_example: linenoise.h linenoise.c

linenoise_example: linenoise.c example.c
	$(CC) -Wall -W -Os -g -o linenoise_example linenoise.c example.c

clean:
	rm -f linenoise_example linenoise.o
