#include <stdio.h>

#include <jack/jack.h>

int main()
{
#ifdef WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif
	printf( "hello\n");
	return 0;
}
