CC = gcc

tcpping: tcpping.c
	$(CC) tcpping.c -o tcpping

install: tcpping
	sudo cp tcpping /usr/bin/

clean:
	rm tcpping
