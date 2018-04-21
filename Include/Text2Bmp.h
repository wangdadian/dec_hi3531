
#pragma once
#include <stdint.h>


class CText2Bmp
{
public:
	CText2Bmp();
	~CText2Bmp();


	int GetBitmap( char *szText, 
					int iFont, 
					int &nWidth, 
					int &nHeight, 
					unsigned char *&pBmp ); 

private:
	void GetWordCount( char *szText,
							int *&pnCodes, 
							int &nCount, 
							int &nHalfFont );


	int GetDZK( int nCode, 
					int iFont, 
					unsigned char *pZk, 
					int &nBytes );


	void Dzk2Argb1555( unsigned char *pDzk,
							int nWidthBits,
							int nHeightBits, 
							unsigned char *&pArgb );
	
	void DrawEdge( unsigned char *pBmp, 
						int nWidth , 
						int nHeight );


	typedef struct _sARGB
	{
		uint8_t blue:5;
		uint8_t green:5;
		uint8_t red:5;
		uint8_t alpha:1;
	}sARGB;

	typedef union
	{
		sARGB rgb;
		unsigned short iRgb;
	}uARGB115;
	
};





