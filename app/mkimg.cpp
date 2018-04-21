
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "FlashRW.h"

void Usage( char *szProg )
{

	printf( "Usage: %s -i [in filename] -o [out filename] -t [type]\n\t [type] 1:rootfs \n" , szProg );
}

int main( int argc, char **argv )
{


	

	
	if ( argc != 7 )
	{
		Usage(argv[0]);
		exit(1);
	}

	

	
		

	char szInFile[1024]={0};
	char szOutFile[1024]={0};

	
	int c;
	int imgtype;
	opterr = 0;
	 
	while ((c = getopt (argc, argv, "i:o:t:")) != -1)

	
		switch (c)
		{
		case 'i':
			strcpy( szInFile, optarg );
			break;
		case 'o':
			strcpy( szOutFile, optarg);
			break;
		case 't':
			imgtype = atoi(optarg);
			break;
		case '?':
			if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			Usage( argv[0]);
			return 1;
		default:
			//Usage( argv[0]);
			//return 1;
			break;
		}

	if (optind > argc)
	{
		Usage(argv[0]);
		exit(1);
	}

	if ( imgtype >= (int)PACKTYPE_INVALID && imgtype <= 0 )
	{
		printf( "invalid image type\n" );
		Usage(argv[0]);
		exit(1);
	}

	
	CFlashRW *prw= new CFlashRW();
	if(prw->PackFile(  szInFile, szOutFile, imgtype ) < 0)
    {
        printf("make image file [%s] failed!\n", szOutFile);
    }
    else
    {
        printf("make image file [%s]  ok.\n", szOutFile);
    }
    
	delete prw;
	prw = NULL;

	return 0;
}


