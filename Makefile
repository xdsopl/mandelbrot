
#CC = $(notdir $(shell ls /usr/bin/gcc-*.*.* | tail -n1))

LDFLAGS = $(shell sdl-config --libs)
CFLAGS = $(shell sdl-config --cflags) \
	-D_GNU_SOURCE=1 \
	-std=c99 -W -Wall -O3 \
	-march=native \
	-fno-math-errno \
	-ffinite-math-only \
	-fno-rounding-math \
	-fno-signaling-nans \
	-fno-trapping-math \
	-fcx-limited-range \
	-g

all: mandelbrot

test: mandelbrot
	./mandelbrot

clean:
	rm -f mandelbrot

