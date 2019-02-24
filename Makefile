swcursor: swcursor.c
	gcc `pkg-config --cflags x11 xext gtk+-3.0` -o swcursor swcursor.c `pkg-config --libs x11 xext gtk+-3.0`

clean:
	rm -f swcursor

.PHONY: clean
