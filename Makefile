CFLAGS ?= -O2 -Wall -Wextra
PREFIX ?= $(HOME)/.local

SRC = $(wildcard src/*.c)

wii-stomp: $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC)

install: wii-stomp
	install -Dm755 wii-stomp $(PREFIX)/bin/wii-stomp
	install -d $(PREFIX)/share/wii-stomp/profiles
	install -m644 profiles/*.json $(PREFIX)/share/wii-stomp/profiles/

uninstall:
	rm -f $(PREFIX)/bin/wii-stomp
	rm -rf $(PREFIX)/share/wii-stomp

clean:
	rm -f wii-stomp

.PHONY: install uninstall clean
