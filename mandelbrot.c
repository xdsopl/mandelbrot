/*
mandelbrot - visualize Mandelbrot set using SIMD
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <immintrin.h>

#ifdef __AVX__
typedef float vnsf __attribute__ ((__vector_size__ (32)));
typedef unsigned vnsu __attribute__ ((__vector_size__ (32)));
#define vnsf_set1(a) _mm256_set1_ps(a)
#define vnsf_cmplt(a, b) ((vnsu)_mm256_cmp_ps(a, b, _CMP_LT_OQ))
#define vnsf_len (8)
#else
typedef float vnsf __attribute__ ((__vector_size__ (16)));
typedef unsigned vnsu __attribute__ ((__vector_size__ (16)));
#define vnsf_set1(a) _mm_set1_ps(a)
#define vnsf_cmplt(a, b) ((vnsu)_mm_cmplt_ps(a, b))
#define vnsf_len (4)
#endif

SDL_Surface *screen;
const int NUM = 32;

float zoom = 5.0;
float xoff = -0.75;
float yoff = 0.0;

void resize_screen(int w, int h)
{
	screen = SDL_SetVideoMode(w, h, 32, SDL_DOUBLEBUF|SDL_RESIZABLE);
	if (NULL == screen)
		exit(1);
	if (screen->format->BytesPerPixel != 4)
		exit(1);
}

void handle_events()
{
	static int button_left = 0;
	static int button_right = 0;
	static int button_middle = 0;
	int width = 0, height = 0;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						button_left = 1;
						break;
					case SDL_BUTTON_MIDDLE:
						button_middle = 1;
						break;
					case SDL_BUTTON_RIGHT:
						button_right = 1;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						button_left = 0;
						break;
					case SDL_BUTTON_MIDDLE:
						button_middle = 0;
						break;
					case SDL_BUTTON_RIGHT:
						button_right = 0;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				if (button_left) {
					yoff -= zoom * (float)event.motion.yrel / (float)screen->h;
					xoff -= zoom * (float)event.motion.xrel / (float)screen->w;
				}
				if (button_middle) {
				}
				if (button_right) {
					zoom -= zoom * (float)event.motion.yrel / (float)screen->h;
				}
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_q:
						exit(0);
						break;
					case SDLK_ESCAPE:
						exit(0);
						break;
					case SDLK_r:
						zoom = 5.0;
						xoff = -0.75;
						yoff = 0.0;
						break;
					default:
						break;
				}
				break;
			case SDL_VIDEORESIZE:
				width = event.resize.w;
				height = event.resize.h;
				break;
			case SDL_QUIT:
				exit(1);
				break;
			default:
				break;
		}
	}
	if (width && height)
		resize_screen(width, height);
}

void handle_stats(double elapsed, uint64_t stat_pixel, uint64_t pixels)
{
	double pps; /* pixel per second */
	double mpps; /* mega pixel per second */
	double fps; /* frames per second */
	pps = (double)stat_pixel / elapsed;
	mpps = 0.000001 * pps;
	fps = pps / (double)pixels;

	char outs[128];
	if (fps < 1.0) {
		snprintf(outs, sizeof(outs), "%.1f s/f %.1f p/s", 1.0/fps, pps);
	} else {
		snprintf(outs, sizeof(outs), "%.1f f/s %.1f mp/s", fps, mpps);
	}

	SDL_WM_SetCaption(outs, "mandelbrot");
//	fprintf(stderr, "%s\n", outs);
}

#if 0
void calc(int t[NUM], complex float c[NUM])
{
	for (int i = 0; i < NUM; i++) {
		complex float z = 0;
		for (t[i] = 0; t[i] < 100; t[i]++) {
			float abs2 = crealf(z) * crealf(z) + cimagf(z) * cimagf(z);
			if (abs2 >= 4.0f)
				break;
			z = z * z + c[i];
		}
		if (t[i] >= 100)
			t[i] = 0;
	}
}
#else
void calc(int T[NUM], complex float C[NUM])
{
	const vnsf _1 = vnsf_set1(1.0f);
	const vnsf _2 = vnsf_set1(2.0f);
	const vnsf _4 = vnsf_set1(4.0f);
	const vnsf _100 = vnsf_set1(100.0f);
	vnsf c_real[NUM / vnsf_len];
	vnsf c_imag[NUM / vnsf_len];
	for (int i = 0; i < NUM; i++) {
		((float *)c_real)[i] = crealf(C[i]);
		((float *)c_imag)[i] = cimagf(C[i]);
	}
	vnsf z_real[NUM / vnsf_len];
	vnsf z_imag[NUM / vnsf_len];
	vnsf t[NUM / vnsf_len];
	memset(z_real, 0, sizeof(z_real));
	memset(z_imag, 0, sizeof(z_imag));
	memset(t, 0, sizeof(t));
	for (int n = 0; n < 100; n++) {
		for (int i = 0; i < NUM / vnsf_len; i++) {
			vnsf abs2 = z_real[i] * z_real[i] + z_imag[i] * z_imag[i];
			t[i] += (vnsf)((vnsu)_1 & vnsf_cmplt(abs2, _4));
			vnsf tmp_real = z_real[i] * z_real[i] - z_imag[i] * z_imag[i] + c_real[i];
			vnsf tmp_imag = _2 * z_imag[i] * z_real[i] + c_imag[i];
			z_real[i] = tmp_real;
			z_imag[i] = tmp_imag;
		}
	}
	for (int i = 0; i < NUM / vnsf_len; i++)
		t[i] = (vnsf)(vnsf_cmplt(t[i], _100) & (vnsu)t[i]);
	for (int i = 0; i < NUM; i++)
		T[i] = ((float *)t)[i];
}
#endif

uint32_t argb(float r, float g, float b)
{
	return ((int)fminf(fmaxf(255.0f * r, 0.0f), 255.0f) << 16) |
		((int)fminf(fmaxf(255.0f * g, 0.0f), 255.0f) << 8) |
		((int)fminf(fmaxf(255.0f * b, 0.0f), 255.0f));
}

uint32_t color(float v)
{
	float r = 4.0f * v - 2.0f;
	float g = 2.0f - 4.0f * fabsf(v - 0.5f);
	float b = 2.0f - 4.0f * v;
	return argb(r, g, b);
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO);
	resize_screen(640, 480);
	SDL_WM_SetCaption("Mandelbrot", "mandelbrot");
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	uint32_t stat_ticks = SDL_GetTicks();
	uint64_t stat_pixels = 0;
	while (1) {
		handle_events();
		uint32_t *fbp = (uint32_t *)screen->pixels;
		int w = screen->w;
		int h = screen->h;
		float dx = 1.0f / (float)w;
		float dy = 1.0f / (float)h;
		for (int j = 0; j < (h * w - NUM); j += NUM) {
			complex float c[NUM];
			for (int k = 0; k < NUM; k++)
				c[k] = (zoom * (dx * (float)((j+k)%w) - 0.5f) + xoff)
					+ I * (zoom * (dy * (float)((j+k)/w) - 0.5f) + yoff);
			int t[NUM];
			calc(t, c);
			for (int k = 0; k < NUM; k++)
				fbp[j + k] = t[k] ? color(0.01f * t[k]) : 0;
		}
		stat_pixels += w*h;
		uint32_t cur_ticks = SDL_GetTicks();
		if ((cur_ticks - stat_ticks) > 1000) {
			handle_stats(0.001 * (double)(cur_ticks - stat_ticks), stat_pixels, w*h);
			stat_pixels = 0;
			stat_ticks = cur_ticks;
		}
		SDL_Flip(screen);
	}
	return 0;
}

