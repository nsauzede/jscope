#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>

#include <jack/jack.h>

#define FDELTA 10.0
#define ADELTA 0.1

jack_client_t *jclient;
jack_port_t *jport;
double f = 440.0;
double a = 1.0;
typedef jack_default_audio_sample_t sample_t;
int end;
int sr = 0;

void jack_shutdown( void *arg)
{
	end = 1;
}

int fswitch = 1;
int N;

int cb( jack_nframes_t nframes, void *arg)
{
	sample_t *buf = (sample_t *)jack_port_get_buffer( jport, nframes);
	int i, n = (int)nframes;
	static int t = 0;

	static int old_t = -99999999;
	if (t > (old_t + 20*N))
	{
		if (fswitch) {
			f *= 2;
			if (f >= (sr / 2)) {
				f = 81.5;
			}
		}
		old_t = t;
		printf( "freq=%.1f\n", f);
	}

	for (i = 0; i < n; i++)
	{
		double res;
		
		res = a * sin( (t++ * f * 2 * M_PI) / sr);
		buf[i] = res;
	}
	
	return 0;
}

int main( int argc, char *argv[])
{
	char *name = "jsine";
	char *connect = NULL;
	int arg = 1;

	while (argc > arg)
	{
		if (!strcmp( argv[arg], "-name"))
		{
			arg++;
			if (argc > arg)
				name = argv[arg++];
			else
				printf( "error : missing name\n");
		}
		else if (!strcmp( argv[arg], "-connect"))
		{
			arg++;
			if (argc > arg)
				connect = argv[arg++];
			else
				printf( "error : missing connect\n");
		}
		else if (!strcmp( argv[arg], "-switch"))
		{
			arg++;
			fswitch = 0;
		}
		else if (!strcmp( argv[arg], "-startf"))
		{
			arg++;
			sscanf(argv[arg++], "%lf", &f);
		}
	}

	printf( "name=%s connect=%s\n", name, connect);
	jclient = jack_client_open( name, JackNullOption, NULL, NULL);
	if (jclient)
	{
	N = jack_get_buffer_size( jclient);
	sr = jack_get_sample_rate( jclient);
	printf( "jack opened : nframes=%d\n sr=%d", (int)N, sr);

	jport = jack_port_register( jclient, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	jack_set_process_callback( jclient, cb, NULL);
    jack_on_shutdown( jclient, jack_shutdown, 0);
	jack_activate( jclient);
	if (connect)
	{
		const char **ports;
	
		ports = jack_get_ports( jclient, connect, NULL, JackPortIsInput);
		if (ports)
		{
			if (jack_connect( jclient, jack_port_name( jport), ports[0]))
				printf( "couldn't connect ports %s and %s\n", jack_port_name( jport), ports[0]);
			free( ports);
		}
		else
			printf( "couldn't get port %s\n", connect);
	}
	end = 0;
	while (!end)
	{
#ifndef WIN32
		fd_set rfds;
		
		FD_ZERO( &rfds);
		FD_SET( 0, &rfds);
		select( 1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET( 0, &rfds))
		{
			char cmd;
			scanf( "%c", &cmd);
			switch (cmd)
			{
				case 'q':
					end = 1;
					break;
				case 'w':
					if (a >= ADELTA)
						a -= ADELTA;
					break;
				case 'x':
					a += ADELTA;
					break;
				case 'a':
					if (f >= FDELTA)
						f -= FDELTA;
					break;
				case 'z':
					f += FDELTA;
					break;
				default:
					break;
			}
			printf( "f=%f a=%f\n", f, a);fflush( stdout);
		}
#endif
	}
	jack_port_unregister( jclient, jport);
	jack_deactivate( jclient);
	
	jack_client_close( jclient);
	}
	else
	{
	printf( "couldn't open jack\n");
	}
	
	return 0;
}

