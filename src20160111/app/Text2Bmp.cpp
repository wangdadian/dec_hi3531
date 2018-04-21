
#include "Text2Bmp.h"
#include "PublicDefine.h"
#include "define.h"

CText2Bmp::CText2Bmp()
{


}

CText2Bmp::~CText2Bmp()
{


}

int CText2Bmp::GetBitmap( char *szText, 
				int iFont, 
				int &nWidth, 
				int &nHeight, 
				unsigned char *&pBmp )
{


	nWidth = iFont;
	nHeight = iFont;
	if ( iFont != 16 && iFont != 24 && iFont != 32 )
	{
		_DEBUG_("unsupported font %d should =16 \n", iFont );
		return -1;
	}

	int *pnCodes = NULL;
	int nWordCount = 0;
	int nHalfFont = 0;

	GetWordCount( szText,  pnCodes, nWordCount, nHalfFont );


	if ( nWordCount == 0 )
	{
		_DEBUG_("get word count error");
		return -1;
	}

	int nBytesPerWord = iFont * iFont / 8;
	unsigned char *pDZ = (unsigned char*)malloc( nBytesPerWord );
	int nImageWidth = nHalfFont * iFont  / 2;
	pBmp = (unsigned char*)malloc( nImageWidth * iFont * 2 );
	nWidth = nImageWidth; 
	nHeight = iFont;
	int iStrideIndex = 0;
	
	for ( int i = 0;  i < nWordCount; i++)
	{
		int nHalf = 0;
		if ( GetDZK(pnCodes[i], iFont, pDZ, nHalf ) < 0 )
		{
			PTR_DELETE_A( pnCodes );
			PTR_FREE( pBmp );
			PTR_FREE( pDZ );
			return -1;
		}
		int nWidthBits =  iFont/(nHalf>0?2:1);
		unsigned char *pArgb=NULL;
		
		Dzk2Argb1555( pDZ,  nWidthBits, iFont, pArgb);

		//iStrideIndex += nStride /
		for ( int j = 0; j < iFont; j++ )
		{
			memcpy( pBmp +  ( nImageWidth * j * 2  + iStrideIndex ), pArgb+j*nWidthBits*2, nWidthBits * 2 );
		}
		
		iStrideIndex += nWidthBits*2;
		PTR_FREE( pArgb );
	}
	DrawEdge( pBmp, nWidth, nHeight );

   	PTR_FREE( pDZ );
	PTR_DELETE_A( pnCodes );
	return 0;
}

void CText2Bmp::DrawEdge( unsigned char *pBmp, int nWidth , int nHeight )
{
	for ( int i = 0; i < nHeight ; i++ )
	{
		for ( int j = 0; j < nWidth; j++ )
		{
			if ( pBmp[2*(i*nWidth+j)] == 0xffUL )
			{
				unsigned char color = 0x80UL;
				if ( i - 1 >= 0 )
				{
					if ( pBmp[2*((i-1)*nWidth + j)] != 0xff )
					{
						pBmp[2*((i-1)*nWidth + j )] = 0x00;
						pBmp[2*((i-1)*nWidth + j )+ 1] = color;
					}
				}
				if ( i + 1 <  nHeight ) 
				{
					if ( pBmp[2*((i+1)*nWidth + j)]  != 0xff )
					{
						pBmp[2*((i+1)*nWidth + j) ] = 0x00;
						pBmp[2*((i+1)*nWidth + j)+1] = color;
					}
				}

				if ( j - 1 >= 0  )
				{

					if ( pBmp[2*((j-1)+i*nWidth)]  != 0xff )
					{
						pBmp[2*((j-1)+i*nWidth) ] = 0x00;
						pBmp[2*((j-1)+i*nWidth)+1] = color;
					}
				}

				if ( j + 1 < nWidth )
				{

					if ( pBmp[ 2*((j+1)+i*nWidth) ]  != 0xff )
					{
						pBmp[2*((j+1)+i*nWidth) ] = 0x00;
						pBmp[2*((j+1)+i*nWidth)+1] = color;
					}
				}

				if ( i - 1 >= 0 && j -1 >=0 )
				{
					if ( pBmp[2*((i-1)*nWidth+(j-1))] != 0xff )
					{
						pBmp[2*((i-1)*nWidth+(j-1)) ] = 0x00;
						pBmp[2*((i-1)*nWidth+(j-1))+1] = color;
					}
				}

				if ( i + 1 < nHeight && j -1 >=0 )
				{
					if ( pBmp[2*((i+1)*nWidth+(j-1))] != 0xff )
					{
						pBmp[2*((i+1)*nWidth+(j-1)) ] = 0x00;
						pBmp[2*((i+1)*nWidth+(j-1))+1] = color;
					}
				}


				if ( i - 1 >= 0 && j + 1 < nWidth )
				{
					if ( pBmp[2*((i-1)*nWidth+(j+1))] != 0xff )
					{
						pBmp[2*((i-1)*nWidth+(j+1)) ] = 0x00;
						pBmp[2*((i-1)*nWidth+(j+1))+1] = color;
					}
				}

				if ( i + 1 < nHeight && j + 1 < nWidth )
				{
					if ( pBmp[2*((i+1)*nWidth + (j+1))  ] != 0xff )
					{
						pBmp[2*((i+1)*nWidth + (j+1)) ] = 0x00;
						pBmp[2*((i+1)*nWidth + (j+1))+1] = color;
					}
				}
			
			}

		}
	}
	/*

	for ( int i = 0; i < nHeight ; i++ )
	{
		for ( int j = 0; j < nWidth; j++ )
		{
			if ( pBmp[2*(j+i*nWidth+1)] == 0 )
				printf("-", pBmp[2*(j+i*nWidth)] );
			else if ( pBmp[2*(j+i*nWidth+1)] == 0xff )
				printf("#", pBmp[2*(j+i*nWidth)] );
		
			else 
				printf("+", pBmp[2*(j+i*nWidth)] );
		}
		printf("\n");
	}
	*/
}

void CText2Bmp::GetWordCount( char *szText, 
								  int *&pnCodes, 
								  int &nCount, 
								  int &nHalfFont )
{
	pnCodes  = NULL;
	nCount = 0;
	nHalfFont = 0;
	
	if (szText == NULL ) return;
	int nLen = strlen(szText);
	if ( nLen == 0 ) return;

	pnCodes = new int[nLen];
	for ( int i = 0; i < nLen; i++ )
	{
		if ( szText[i] < 127 )
		{
			pnCodes[nCount++] = szText[i];
			nHalfFont++;
		}
		else if ( i <= nLen -1 )
		{
			int ch0 = (unsigned char)szText[i]-0x81; /* ÇøÂë */
			int ch1 = 0;
			
			if ( szText[i+1] < 0x7f )
		    {
		    	 ch1 = (unsigned char)szText[i+1]-0x40; /* Î»Âë */
			}
			else
			{
				ch1 = (unsigned char)szText[i+1]-0x41; /* Î»Âë */
			}
			
			pnCodes[nCount++] = (int)((ch0) * 190 + ch1);
			i++;
			nHalfFont += 2;
		}
	}
	
}

void CText2Bmp::Dzk2Argb1555( unsigned char *pDzk,
								int nWidthBits,
								int nHeightBits, 
								unsigned char *&pArgb )
{
	pArgb = (unsigned char*)malloc(nWidthBits*nHeightBits *2);

	int iIndex = 0;
	for ( int i = 0; i < nWidthBits * nHeightBits / 8; i++ )
	{
		for ( int j = 7; j >= 0; j-- )
		{
			if ( pDzk[i] & ( 1 << j ) )
			{
			   uARGB115 uargb115;
			   uargb115.rgb.blue = 31;
			   uargb115.rgb.green = 31;
			   uargb115.rgb.red = 31;
			   uargb115.rgb.alpha = 1;
			   
			   unsigned char color = 0xffUL;
			   pArgb[iIndex++] = color;
			   //printf( "##", pArgb[iIndex-1] );
			   pArgb[iIndex++] = color;
			   //printf( "## ", pArgb[iIndex-1] );
			   //printf( "1 ");
			  
			}
			else
			{
			   uARGB115 uargb115;
			   uargb115.rgb.blue = 0;
			   uargb115.rgb.green = 0;
			   uargb115.rgb.red = 0;
			   uargb115.rgb.alpha = 0;
			   pArgb[iIndex++] = 0;
			  // printf( "  ", pArgb[iIndex-1] );
			   pArgb[iIndex++] = 0;
			   //printf( "   ", pArgb[iIndex-1] );
				//printf( "0 ");
			   //printf( "0x%x ", uargb115.iRgb );
			}
		}

		//if ( (i+1) % (nWidthBits/8) == 0 ) printf( "\n" );
	}
}

int  CText2Bmp::GetDZK( int nCode, 
						int iFont, 
						unsigned char *pZk, 
						int &nHalf )
{

	FILE *fileZk=NULL;
	char szFileName[1024]={0};
	int nBytesPerCode=0;
	int nStrideBits = 0;
	nHalf = 0;
	if ( nCode < 127 )
	{
		sprintf( szFileName, "/opt/dws/en%d.DZK", iFont ); 
		nBytesPerCode = iFont * (iFont /2) / 8;
		nStrideBits = iFont /2;
		nHalf = 1;
	}
	else
	{
		sprintf( szFileName, "/opt/dws/heiti%d.DZK", iFont ); 
		nBytesPerCode = iFont * iFont / 8;
		nStrideBits = iFont;
		nHalf = 0;
	}

	fileZk = fopen( szFileName, "r" );
	if ( fileZk == NULL )
	{
		_DEBUG_( "open zk %s error.", szFileName );
		return -1;
	}

		
	fseek( fileZk, nBytesPerCode*nCode, SEEK_SET );
	int nRead = fread( pZk, 1, nBytesPerCode, fileZk );
	if ( nRead != nBytesPerCode )
	{
		_DEBUG_( "read error." );
		perror( "reead zk error.");
		fclose( fileZk );
		return -1;
	}
	/*
    int iIndex = 0;
	for ( int i = 0; i < nStrideBits*iFont/8; i++ )
	{

		for ( int k = 7; k >= 0; k-- )
		{
			if ( pZk[i] & (1 << k) )
			{
				printf( "# " );
			}
			else
				printf("  " );
			iIndex ++;
			if ( iIndex % nStrideBits== 0 ) 
			printf("\n");
		}
		
	}
	*/
	fclose( fileZk );
	return 0;

}



