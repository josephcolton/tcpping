CC ?= gcc
CFLAGS ?= -O2 -Wall
LDFLAGS ?=
PREFIX ?= /usr/bin
TARGET = tcpping

tcpping: tcpping.c
	$(CC) $(CFLAGS) $(LDFLAGS) tcpping.c -o tcpping

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/$(TARGET)

clean:
	rm -f $(TARGET)
