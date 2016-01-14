#include <stdio.h>
#include <SDL.h>
#include <jack/jack.h>

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
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
	int ww = 640;
	int hh = 200;
	int arg = 1;
	if (arg < argc)
		sscanf( argv[arg++], "%d", &ww);
	jack_status_t status;
	jack_client_t *client = jack_client_open( "jscope", JackNoStartServer, &status);
	jack_port_t *port;
	if (client) {
		port = jack_port_register( client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		jack_set_process_callback( client, process_audio, port);
		samplerate = jack_get_sample_rate( client);
		printf( "samplerate=%d\n", samplerate);
		int bufsize = jack_get_buffer_size( client);
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
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	visiblelen = ww;
	visible = malloc( visiblelen * sizeof(visible[0]));
	int done = 0;
	int deci0 = 100;
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
						case SDLK_LEFT:
							if (deci0 > 1)
							{
								deci0--;
								printf( "deci0=%d\n", deci0);
							}
							break;
						case SDLK_RIGHT:
							if (deci0 < (visiblelen / 2))
							{
								deci0++;
								printf( "deci0=%d\n", deci0);
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
		black = SDL_MapRGB( screen->format, 0, 0, 0);
		green = SDL_MapRGB( screen->format, 0, 255, 0);
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
			int n;
			int n0, n1, nsamples;
			n0 = samplequeue;
			n1 = samplehead;
			if (n0 <= n1)
				nsamples = n1 - n0;
			else
				nsamples = sampleslen - (n0 - n1);
			int deci = 8;
			deci = deci0;
			if (deci < (nsamples / ww))
			{
				deci = nsamples / ww;
			}
			int len = nsamples / deci;
			for (n = 0; n < visiblelen - len; n++)
			{
				visible[n] = visible[len + n];
			}
			for (n = 0; n < len; n++)
			{
				visible[visiblelen - len + n] = samples[(n0 + n * deci) % sampleslen];
			}
			samplequeue = (samplequeue + nsamples) % sampleslen;
			double max = 1;
			for (n = 0; n < visiblelen; n++)
				if (visible[n] > max)
					max = visible[n];
			for (n = 0; n < visiblelen; n++)
			{
				rect.x = ww * n / visiblelen;
				rect.y = hh / 2 - visible[n] * hh / 2 / max;
				SDL_FillRect( screen, &rect, col);
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
			if (lost && (oldframeslost != lost))
			{
				printf( "frameslost=%d\n", lost);
				oldframeslost = lost;
				frameslost = 0;
			}
			oldticks = ticks;
			//			printf( "fps=%d\n", fps);
			fps = 0;
		}
		else
			SDL_Delay( 20);
	}
	return 0;
}
