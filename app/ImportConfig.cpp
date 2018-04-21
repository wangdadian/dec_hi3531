#include <stdio.h>
#include <stdlib.h>
#include "FlashRW.h"

int main( int argc, char **argv )
{

	if ( argc < 2 ) 
	{
		printf("%s [filename]\n", argv[0] );
		return 0;
	}

	CFlashRW *prw= new CFlashRW();
	prw->ImportConfigToMtd( (char*)"/dev/mtd/2",  argv[1]);
	delete prw;
	prw = NULL;
	return 0;
}



