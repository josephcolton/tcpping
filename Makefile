CC ?= gcc
CFLAGS ?= -O2 -Wall
LDFLAGS ?=
PREFIX ?= /usr/bin
MANPREFIX ?= /usr/share/man
TARGET = tcpping
MAN = tcpping.1

tcpping: tcpping.c
	$(CC) tcpping.c -o tcpping

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/$(TARGET)
	install -Dm644 $(MAN) $(MANPREFIX)/man1/$(MAN)
	gzip -f $(MANPREFIX)/man1/$(MAN)

clean:
	rm -f $(TARGET)
