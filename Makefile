tcpping: tcpping.c
	gcc tcpping.c -o tcpping

install: tcpping
	cp tcpping /usr/bin/

clean:
	rm tcpping
