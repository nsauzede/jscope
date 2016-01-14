#include <stdio.h>
#include <SDL.h>
#include <jack/jack.h>
#include <fftw3.h>

int samplerate = 0;

int sampleslen = 0;
double *samples = 0;
int samplehead = 0;
int samplequeue = 0;

int visiblelen = 0;
double *visible = 0;
int nvisible = 0;

int frameslost = 0;
int runnotstop = 0;
int process_audio( jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *buf;
	jack_port_t *input_port = (jack_port_t *)arg;
#if 0
	static int count = 0;
	printf( "%s: #%d nframes=%d\n", __func__, count, nframes);
	count++;
#endif

	buf = jack_port_get_buffer( input_port, nframes);
	if (runnotstop)
		return 0;
	int i;
	for (i = 0; i < nframes; i++)
	{
		if (((samplehead + 1) % sampleslen) == samplequeue)
		{
			frameslost += nframes - i;
			break;
		}
		samples[samplehead] = buf[i];
		samplehead = (samplehead + 1) % sampleslen;
	}

	return 0;
}

int main( int argc, char *argv[]) {
#ifdef WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif
	int ww = 512;
	int hh = 200;
	jack_status_t status;
	jack_client_t *client = jack_client_open( "jscope", JackNoStartServer, &status);
	jack_port_t *port;
	int bufsize = 0;
	if (client) {
		port = jack_port_register( client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		jack_set_process_callback( client, process_audio, port);
		samplerate = jack_get_sample_rate( client);
		printf( "samplerate=%d\n", samplerate);
		bufsize = jack_get_buffer_size( client);
		printf( "bufsize=%d\n", bufsize);
		sampleslen = samplerate * 5;
//		sampleslen = ww;
		samples = malloc( sampleslen * sizeof(samples[0]));
		jack_activate( client);
	} else {
		printf( "jack server not running ?\n");
		exit( 1);
	}
	SDL_Surface *screen = 0;
	SDL_Init( SDL_INIT_VIDEO);
	atexit( SDL_Quit);
	screen = SDL_SetVideoMode( ww, hh, 32, 0);
	visiblelen = ww;
	visible = malloc( visiblelen * sizeof(visible[0]));

	
	FFTW_PLAN p = (void *)0xdeadbeef;
	FFTW_TYPE *in, *out;
	in = fftw_alloc_real( bufsize);
	out = fftw_alloc_real( bufsize);
	printf( "in=%p out=%p bufsize=%d\n", in, out, bufsize);
	p = FFTW_PLAN_R2R_1D( bufsize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	printf( "got plan=%p\n", p);
	if (!p)
		exit( 1);
	int done = 0;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent( &event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					done = 1;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							done = 1;
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
//		Uint32 blue, orange, red;
		SDL_Rect rect;
		black = SDL_MapRGB( screen->format, 0, 0, 0);
//		red = SDL_MapRGB( screen->format, 255, 0, 0);
		green = SDL_MapRGB( screen->format, 0, 255, 0);
//		blue = SDL_MapRGB( screen->format, 0, 0, 255);
//		orange = SDL_MapRGB( screen->format, 255, 0, 0);
		rect.x = 0;
		rect.y = 0;
		rect.w = ww;
		rect.h = hh;
		{
			col = black;
			SDL_FillRect( screen, &rect, col);
			col = green;
			rect.w = 1;
			rect.h = 1;
			int n, n0, n1, nsamples;
			n0 = samplequeue;
			n1 = samplehead;
/*
 * q
 * 0 1 2 3 4
 *   h=1
 *     h=2
 *
 * h=3=5-(2-0)
 * 0 1 2 3 4
 *     q
 *
 * 1000ms=>48000S
 * 10ms=>480S
 */
			if (n0 <= n1)
				nsamples = n1 - n0;
			else
				nsamples = sampleslen - (n0 - n1);
			if (nsamples >= bufsize)
			{
				nsamples = bufsize;
				for (n = 0; n < nsamples; n++)
					in[n] = samples[(n0 + n) % sampleslen];
				FFTW_EXECUTE( p);
				double min = 0, max = 0;
				for (n = 0; n < nsamples; n++)
				{
					out[n] = abs( out[n]);
					if (out[n] > max)
						max = out[n];
					if (out[n] < min)
						min = out[n];
				}
				if (max < 0.001)
					max= 1;
#if 0
				int deci = 1;
				if (nsamples > ww)
					deci = nsamples / ww;
#endif
				for (n = 0; n < nsamples / 2; n++)
				{
//					if (n % deci)
//						continue;
					rect.x = ww * n / (nsamples / 2);
					rect.y = hh / 2 - out[n] * hh / 2 / max;
					SDL_FillRect( screen, &rect, col);
				}
				samplequeue = (samplequeue + nsamples) % sampleslen;
			}
		}
		static int fps = 0;
		SDL_UpdateRect( screen, 0, 0, 0, 0);
		fps++;
		static int oldticks = 0;
		int ticks = SDL_GetTicks();
		if (ticks > oldticks + 1000)
		{
			static int oldframeslost = 0;
			int lost = frameslost;
			if (oldframeslost != lost)
			{
				printf( "frameslost=%d\n", lost);
				oldframeslost = lost;
				frameslost = 0;
			}
			oldticks = ticks;
//			printf( "fps=%d\n", fps);
			fps = 0;
		}
//		else
			SDL_Delay( 20);
	}
	FFTW_DESTROY_PLAN( p);
	fftw_free( in);
	fftw_free( out);

	return 0;
}
