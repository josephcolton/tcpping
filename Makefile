CC ?= gcc
CFLAGS ?= -O2 -Wall
LDFLAGS ?=
PREFIX ?= /usr/bin
MANPREFIX ?= /usr/share/man
TARGET = tcpping
MAN = tcpping.1

# Native Windows build (MinGW-w64 gcc) needs Winsock
ifeq ($(OS),Windows_NT)
TARGET = tcpping.exe
LDLIBS = -lws2_32
endif

$(TARGET): tcpping.c
	$(CC) $(CFLAGS) tcpping.c -o $(TARGET) $(LDFLAGS) $(LDLIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/$(TARGET)
	install -Dm644 $(MAN) $(MANPREFIX)/man1/$(MAN)
	gzip -f $(MANPREFIX)/man1/$(MAN)

clean:
	rm -f tcpping tcpping.exe
