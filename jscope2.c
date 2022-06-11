/*
 * Copyright(c) 2022 Nicolas Sauzede (nsauzede@laposte.net)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <SDL.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <stdio.h>

int samplerate = 0;

typedef jack_default_audio_sample_t sample_t;
int sampleslen = 0;
jack_ringbuffer_t *rb;

int visiblelen = 0;
sample_t *visible = 0;
int nvisible = 0;

int frameslost = 0;
int runnotstop = 0;
int process_audio(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *buf;
  jack_port_t *input_port = (jack_port_t *)arg;
  buf = jack_port_get_buffer(input_port, nframes);
  if (runnotstop)
    return 0;
  int n;
  n = jack_ringbuffer_write_space(rb) / sizeof(sample_t);
  if (n < nframes) {
    frameslost += nframes - n;
  } else if (n > nframes) {
    n = nframes;
  }
  for (int i = 0; i < n; i++) {
    jack_ringbuffer_write(rb, (const char *)&buf[i], sizeof(sample_t));
  }

  return 0;
}

int main(int argc, char *argv[]) {
#ifdef WIN32
  freopen("CON", "w", stdout);
  freopen("CON", "w", stderr);
#endif
  int ww = 640;
  int hh = 200;
  int arg = 1;
  if (arg < argc)
    sscanf(argv[arg++], "%d", &ww);
  jack_status_t status;
  jack_client_t *client =
      jack_client_open("jscope", JackNoStartServer, &status);
  jack_port_t *port;
  if (client) {
    port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE,
                              JackPortIsInput, 0);
    jack_set_process_callback(client, process_audio, port);
    samplerate = jack_get_sample_rate(client);
    printf("samplerate=%d\n", samplerate);
    int bufsize = jack_get_buffer_size(client);
    printf("bufsize=%d\n", bufsize);
    sampleslen = samplerate * 5;
    // sampleslen = ww;
    rb = jack_ringbuffer_create(sampleslen * sizeof(sample_t));
    jack_activate(client);
  } else {
    printf("jack server not running ?\n");
    exit(1);
  }
  SDL_Surface *screen = 0;
  SDL_Init(SDL_INIT_VIDEO);
  atexit(SDL_Quit);
  SDL_Window *sdlWindow = 0;
  SDL_Renderer *sdlRenderer = 0;
  SDL_Texture *sdlTexture = 0;
  SDL_CreateWindowAndRenderer(ww, hh, 0, &sdlWindow, &sdlRenderer);
  screen = SDL_CreateRGBSurface(0, ww, hh, 32, 0x00FF0000, 0x0000FF00,
                                0x000000FF, 0xFF000000);
  sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, ww, hh);
  visiblelen = ww;
  visible = malloc(visiblelen * sizeof(visible[0]));
  int done = 0;
  int deci0 = 100;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        done = 1;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          done = 1;
          break;
        case SDLK_SPACE:
          runnotstop = 1 - runnotstop;
          break;
        case SDLK_LEFT:
          if (deci0 > 1) {
            deci0--;
            printf("deci0=%d\n", deci0);
          }
          break;
        case SDLK_RIGHT:
          if (deci0 < (visiblelen / 2)) {
            deci0++;
            printf("deci0=%d\n", deci0);
          }
          break;
        default:
          break;
        }
        break;
      }
      if (done)
        break;
    }
    Uint32 col, black, green;
    SDL_Rect rect;
    black = SDL_MapRGB(screen->format, 0, 0, 0);
    green = SDL_MapRGB(screen->format, 0, 255, 0);
    rect.x = 0;
    rect.y = 0;
    rect.w = ww;
    rect.h = hh;
    {
      col = black;
      SDL_FillRect(screen, &rect, col);
      col = green;
      rect.w = 1;
      rect.h = 1;
      int n, nsamples;
      nsamples = jack_ringbuffer_read_space(rb) / sizeof(sample_t);
      int deci = 8;
      deci = deci0;
      if (deci < (nsamples / ww)) {
        deci = nsamples / ww;
      }
      int len = nsamples / deci;
      for (int n = 0; n < visiblelen - len; n++) {
        visible[n] = visible[len + n];
      }
      for (int n = 0; n < len; n++) {
        jack_ringbuffer_read(rb, (char *)&visible[visiblelen - len + n], sizeof(sample_t));
        if (deci > 1) {
          jack_ringbuffer_read_advance(rb, (deci - 1) * sizeof(sample_t));
        }
      }
      sample_t max = 1;
      for (n = 0; n < visiblelen; n++)
        if (visible[n] > max)
          max = visible[n];
      for (n = 0; n < visiblelen; n++) {
        rect.x = ww * n / visiblelen;
        rect.y = hh / 2 - visible[n] * hh / 2 / max;
        SDL_FillRect(screen, &rect, col);
      }
    }
    static int fps = 0;
    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
    fps++;
    static int oldticks = 0;
    int ticks = SDL_GetTicks();
    if (ticks > oldticks + 1000) {
      static int oldframeslost = 0;
      int lost = frameslost;
      if (lost && (oldframeslost != lost)) {
        printf("frameslost=%d\n", lost);
        oldframeslost = lost;
        frameslost = 0;
      }
      oldticks = ticks;
      // printf( "fps=%d\n", fps);
      fps = 0;
    } else
      SDL_Delay(20);
  }
  return 0;
}
