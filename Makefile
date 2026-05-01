CC = gcc
CFLAGS = -Wall -Wextra -O2 `pkg-config --cflags gtk+-3.0 gio-2.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 gio-2.0`

TARGET = velox
SRCS = src/main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -D -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	install -D -m 644 assets/velox.svg /usr/share/icons/hicolor/scalable/apps/velox.svg
	install -D -m 644 velox.desktop /usr/share/applications/velox.desktop

install-local: $(TARGET)
	mkdir -p ~/.local/bin ~/.local/share/applications ~/.local/share/icons/hicolor/scalable/apps
	cp $(TARGET) ~/.local/bin/
	cp assets/velox.svg ~/.local/share/icons/hicolor/scalable/apps/
	cp velox.desktop ~/.local/share/applications/
	-update-desktop-database ~/.local/share/applications

.PHONY: all clean install install-local
