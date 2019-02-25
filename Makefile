swcursor: swcursor.c swcursor-window.h swcursor-window.c
	gcc `pkg-config --cflags x11 xext gtk+-3.0` -o swcursor swcursor.c swcursor-window.c `pkg-config --libs x11 xext gtk+-3.0`

clean:
	rm -f swcursor

.PHONY: clean
