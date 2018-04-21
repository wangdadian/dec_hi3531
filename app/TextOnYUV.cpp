#include "TextOnYUV.h"
#include "PublicDefine.h"
#include "define.h"

CTextOnYuv::CTextOnYuv()
{

	pthread_mutex_init( &m_lock, NULL );

}

CTextOnYuv::~CTextOnYuv()
{

	pthread_mutex_lock(&m_lock);
	map<int, unsigned char*>::iterator dzkiter;
	for( dzkiter = m_mapDzk.begin(); dzkiter != m_mapDzk.end(); dzkiter++ )
	{
		unsigned char *pDzk=(unsigned char*)(dzkiter->second);
		if ( pDzk != NULL )
		{
			free(pDzk);
			pDzk = NULL;
		}
	}
	m_mapDzk.clear();
	pthread_mutex_unlock(&m_lock);

	pthread_mutex_destroy(&m_lock);
}

int CTextOnYuv::DrawText( char *szText, 
				int iFont, 
				int nStartX,
				int nStartY,
				int nImageWidth, 
				int nImageHeight, 
				unsigned char *pYUV )
{
	if (nStartX <= 1)
	{
		nStartX = 2;
	}
	if (nStartX >= (nImageWidth - 1) )
	{
		nStartX = nImageWidth - 2;
	}
	if (nStartY <= 1)
	{
		nStartY = 2;
	}
	if (nStartY >= (nImageHeight - 1) )
	{
		nStartY = nImageHeight - 2;
	}

	if ( nStartX >= nImageWidth || nStartY >= nImageHeight )
	{
		_DEBUG_("startx:%d or starty:%d error.", nStartX, nStartY );
		return -1;
	}

	if ( iFont != 16 && iFont != 24 && iFont != 32 && iFont != 64 )
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
	unsigned char *pDZ = NULL;

	int nOffSetX = nStartX;
	int nOffSetY = nStartY;
	

	for ( int i = 0;  i < nWordCount; i++)
	{
		int nHalf = 0;

		if ( GetDZK(pnCodes[i], iFont, pDZ, nHalf ) < 0 )
		{
			PTR_DELETE_A( pnCodes );
			_DEBUG_("get dzk error.");
			PTR_FREE( pDZ );
			return -1;
		}

		int nWidthBits =  iFont/(nHalf>0?2:1);
		int nStrideBits = iFont / (nHalf>0?2:1);
		
		int iIndex = 0;

		// 描边
		for ( int k = 0; k <iFont; k++ )
		{
			for ( int j = 0; j < nStrideBits; j++ )
			{
				if ( pDZ[k*nStrideBits+j] > 0 )
				{
					for (int m = -2; m<3; m++)
					{
						for (int n = -2; n<3; n++)
						{
							pYUV[nImageWidth*(nOffSetY+k+m)+nOffSetX+j+n] = 0;
						}
					}
				}
				else
				{ 
					//printf(" ");
				}
				iIndex ++;
			}
			
			//printf("\n");
		}

		// 写字
		for ( int k = 0; k <iFont; k++ )
		{
			for ( int j = 0; j < nStrideBits; j++ )
			{
				if ( pDZ[k*nStrideBits+j] > 0 )
				{
					//printf( "# " );
					pYUV[nImageWidth*(nOffSetY+k)+nOffSetX+j] = 255;
				}
				else
				{ 
					//printf(" ");
				}
				iIndex ++;
			}

			//printf("\n");
		}

		nOffSetX += nWidthBits;
	}
	//DrawEdge( pBmp, nWidth, nHeight );

	PTR_DELETE_A( pnCodes );
	return 0;
}

int CTextOnYuv::GetCodeList(char* szText, int iFont, unsigned char** pCodeList, int* pHalfList, int &nCount)
{
    int *pnCodes = NULL;
    int nWordCount = 0;
    int nHalfFont = 0;
    int iRet = 0;
    char* pStr = NULL;
    int i,j,x,y,n;
    int iTmpHalf = 0;
    int iFontWidth = 0;
    unsigned char* pucTmpCode = NULL;
    if(szText == NULL || strlen(szText)<=0)
    {
        return -1;
    }
    
    for(i=0; i<nCount;i++)
    {
        pCodeList[i] = NULL;
        pHalfList[i] = 0;
    }
    //获取区码
    GetWordCount(szText, pnCodes, nWordCount, nHalfFont);
    if(nWordCount <=0) return -1;
    nCount = nWordCount;

    //获取字符点阵
    for(n=0; n<nCount;n++)
    {
        if( GetDZK(pnCodes[n], iFont, pCodeList[n], pHalfList[n]) != 0 )
        {
            nCount = 0;
            return -1;
        }
        // 描边
/*
        pucTmpCode = pCodeList[n];
        iTmpHalf = pHalfList[n];
        iFontWidth = iFont / ((iTmpHalf>0)?2:1);
        for(j=0; j<iFont; j++)
        {
            for(i=0; i<iFontWidth; i++)
            {
                if( pucTmpCode[j*iFontWidth+i] == 255 )
                {
                    // 描边
                    for(y=j-1; y<=j+1; y++)
                    {
                        for(x=i-1; x<=i+1; x++)
                        {
                            if(y<0 || y==iFont || x<0 || x==iFontWidth)
                            {
                                //如果当前点为点阵边缘，则描此点
                                pucTmpCode[j*iFontWidth+i] = 128;
                                continue;
                            }
                            // 描当前点的周围点
                            if( pucTmpCode[y*iFontWidth+x] == 0 )
                            {
                                pucTmpCode[y*iFontWidth+x] = 128;
                            }
                        }
                    }
                }
            }
        }
      */
        
    }

    PTR_DELETE_A( pnCodes );
    return iRet;

}
int CTextOnYuv::GetDz(char* szText, int iFont, unsigned char* &pdz, int &nHalf)
{
    int *pnCodes = NULL;
    int nCode = 0;
    int nWordCount = 0;
    int nHalfFont = 0;
    int iRet = 0;
    char* pStr = NULL;
    int i,j,x,y,n;
    if(szText == NULL || strlen(szText)<=0)
    {
        return -1;
    }
    int iFontSize = iFont;
    if(iFont!=16 && iFont !=32)
    {
        iFontSize = 32;
    }
    //获取区码
    GetWordCount(szText, pnCodes, nWordCount, nHalfFont);
    nCode = pnCodes[0];
    //获取字符点阵
	nHalf  = 0;
	if ( nCode < 127 )
	{
		nHalf = 1;
	}

	FILE *fileZk=NULL;
	char szFileName[1024]={0};
	int nBytesPerCode=0;
	int nStrideBits = 0;
	nHalf = 0;
	if ( nCode < 127 )
	{
		sprintf( szFileName, "/opt/dws/en%d.DZK", iFontSize ); 
		//sprintf( szFileName, "/opt/dws/heiti%d.DZK", iFontSize ); 
		nBytesPerCode = iFontSize * (iFontSize /2) / 8;
		nStrideBits = iFontSize /2;
		nHalf = 1;
	}
	else
	{
		sprintf( szFileName, "/opt/dws/heiti%d.DZK", iFontSize ); 
		nBytesPerCode = iFontSize * iFontSize / 8;
		nStrideBits = iFontSize;
		nHalf = 0;
	}

	fileZk = fopen( szFileName, "r" );
	if ( fileZk == NULL )
	{
		_DEBUG_( "open zk %s error.", szFileName );
		return -1;
	}

		
	fseek( fileZk, nBytesPerCode*nCode, SEEK_SET );

    int iDzLen = iFontSize*iFontSize/(nHalf==0?1:2)/8;
	pdz = (unsigned char*)malloc( iDzLen);
    memset(pdz, 0, iDzLen*sizeof(unsigned char));
	int nRead = fread( pdz, 1, nBytesPerCode, fileZk );
	if ( nRead != nBytesPerCode )
	{
		_DEBUG_( "read error." );
		perror( "reead zk error.");
		fclose( fileZk );
		free(pdz);
        pdz=NULL;
		return -1;
	}
	fclose( fileZk );

    PTR_DELETE_A( pnCodes );

}


void CTextOnYuv::GetWordCount( char *szText, 
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
			int ch0 = (unsigned char)szText[i]-0x81; /* 区码 */
			int ch1 = 0;
			
			if ( szText[i+1] < 0x7f )
		    {
		    	 ch1 = (unsigned char)szText[i+1]-0x40; /* 位码 */
			}
			else
			{
				ch1 = (unsigned char)szText[i+1]-0x41; /* 位码 */
			}
			
			pnCodes[nCount++] = (int)((ch0) * 190 + ch1);
			i++;
			nHalfFont += 2;
		}
	}
	
}


int CTextOnYuv::GetDzkInMem( int nCode, int nFont, unsigned char *&pZk )
{

	pthread_mutex_lock(&m_lock);
	if ( m_mapDzk.count(nCode|(nFont<<16)) > 0 )
	{
		pZk = m_mapDzk[nCode|(nFont<<16)];
		pthread_mutex_unlock(&m_lock);
		return 0;
		
	}

	pthread_mutex_unlock(&m_lock);

	return -1;
}


void CTextOnYuv::AddDzkToMem( int nCode, int nFont, unsigned char *pZk )
{
	pthread_mutex_lock(&m_lock);
	m_mapDzk.insert( make_pair(nCode|(nFont<<16), pZk) );
	pthread_mutex_unlock(&m_lock);
}


void zoom_hz(char *hzbuf,char *fontbuf,int w,int h,int xscal)
{
	int x,y,ix,iy,ix1,iy1,ix2,iy2,ix3,iy3,i,j,c,c1,c2,c3,c4,w1,wbit;
	/*bool keyx=0,keyy=0;*/
	char keyx=0,keyy=0;
	int fx,fy;
	char contbuf[512];

	memset(contbuf,0,sizeof(contbuf));
	/*keyx = keyy = 0;
	if ( xscal > 32 )	
	xscal = 32;*/
	w1 = w/4;

	for(y=0;y<h;y++)
	{
		for(x=0;x<w;x++)
		{
			c1 = c2 = c3 = c4 = 0;
			wbit = x+y*w;
			c = ((*(hzbuf+(wbit>>3))>>(7^(wbit&0x07)))&0x01);
			if ( !c )
				continue;
			if ( x && x < w-1 )
			{
				c1 = ((*(hzbuf+((wbit-1)>>3))>>(7^((wbit-1)&0x07)))&0x01);      /*当前点的前点*/   
				c2 = ((*(hzbuf+((wbit+1)>>3))>>(7^((wbit+1)&0x07)))&0x01);      /*当前点的后点*/   
			}                                                                                       
			if ( y && y <= w-1 )                                                                    
			{                                                                                       
				c3 = ((*(hzbuf+((wbit-w)>>3))>>(7^((wbit-w)&0x07)))&0x01);      /*当前点的上点*/   
				c4 = ((*(hzbuf+((wbit+w)>>3))>>(7^((wbit+w)&0x07)))&0x01);    /*当前点的下点*/ 
			}
			fx = x*xscal + w1;
			fy = y*xscal + w1;

			if ( xscal <= w1 )
			{
				if ( c1 && !c2 )
					continue;
			}
			ix = ix2 = fx/w;
			iy = iy1 = fy/w;
			fx += xscal;
			fy += xscal;
			ix1 = ix3 = fx/w;
			iy2 = iy3 = fy/w;
			wbit = ix + iy*xscal;

			if ( c )
			{
				if ( y && !c3 && ((*(contbuf+((wbit-xscal)>>3))>>(7^((wbit-xscal)&0x07)))&0x01) )
					continue;
				else
					*(contbuf+(wbit>>3)) |= (1<<(7^(wbit&0x07)));
				//if ( ix1>ix && iy2>iy )/*如果汉字放大，则补点*/   
				{
					*(contbuf+((ix1-1+iy1*xscal)>>3)) |= (1<<(7^((ix1-1+iy1*xscal)&0x07)));
					*(contbuf+((ix2+(iy2-1)*xscal)>>3)) |= (1<<(7^((ix2+(iy2-1)*xscal)&0x07)));
					*(contbuf+((ix3-1+(iy3-1)*xscal)>>3)) |= (1<<(7^((ix3-1+(iy3-1)*xscal)&0x07)));
				}
			}
			keyx = ~keyx;
		}
		keyy = ~keyy;
	}
	memcpy(fontbuf,contbuf,sizeof(contbuf));
}

void Byte2bit(char *pByte, int w, int h, char *pBit)
{
	for(int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			if ( pByte[w*i + j]>0 )
			{
				pBit[(w*i + j)/8] |= 1 << ( 7 - (w*i + j)%8 );
			}
		}
	}
}

void bit2Byte(char *pBit, int w, int h, char *pByte)
{
	int iIndex = 0;
	for ( int i = 0; i < h*w/8; i++ )
	{
		for ( int k = 7; k >= 0; k-- )
		{
			if ( pBit[i] & (1 << k) )
			{
				pByte[iIndex] = 255; 
			}
			else
			{
				pByte[iIndex] = 0;
			}	
			iIndex ++; 
		}
	}
}

int  CTextOnYuv::GetDZK( int nCode, 
						int iFont, 
						unsigned char *&pZk, 
						int &nHalf )
{

	nHalf  = 0;
	if ( nCode < 127 )
	{
		nHalf = 1;
	}
	
	if ( GetDzkInMem( nCode, iFont, pZk ) == 0 )
	{
		return 0;
	}

	int iSize = iFont==64?32:iFont;

	FILE *fileZk=NULL;
	char szFileName[1024]={0};
	int nBytesPerCode=0;
	int nStrideBits = 0;
	nHalf = 0;
	if ( nCode < 127 )
	{
		sprintf( szFileName, "/opt/dws/en%d.DZK", iSize ); 
		//sprintf( szFileName, "/opt/dws/heiti%d.DZK", iSize ); 
		nBytesPerCode = iSize * (iSize /2) / 8;
		nStrideBits = iSize /2;
		nHalf = 1;
	}
	else
	{
		sprintf( szFileName, "/opt/dws/heiti%d.DZK", iSize ); 
		nBytesPerCode = iSize * iSize / 8;
		nStrideBits = iSize;
		nHalf = 0;
	}

	fileZk = fopen( szFileName, "r" );
	if ( fileZk == NULL )
	{
		_DEBUG_( "open zk %s error.", szFileName );
		return -1;
	}

		
	fseek( fileZk, nBytesPerCode*nCode, SEEK_SET );


	unsigned char *pTmpZk = (unsigned char*)malloc( iFont*iFont );
    memset(pTmpZk, 0, iFont*iFont*sizeof(unsigned char));
	int nRead = fread( pTmpZk, 1, nBytesPerCode, fileZk );
	if ( nRead != nBytesPerCode )
	{
		_DEBUG_( "read error." );
		perror( "reead zk error.");
		fclose( fileZk );
		free(pTmpZk);
		return -1;
	}

	fclose( fileZk );

	pZk = (unsigned char*)malloc( iFont*iFont );
    memset(pZk, 0, iFont*iFont*sizeof(unsigned char));
    // 非汉字做扩大处理,变成和汉字一样，右半部分内存添0
    unsigned char* pzk1=(unsigned char*)malloc( iFont*iFont );
    if(nHalf!=0&&iFont==64)
    {
        memset(pzk1, 0, iFont*iFont*sizeof(unsigned char));
        for(int i=0;i<iSize; i++)
        {
            memcpy(pzk1+4*i, pTmpZk + i*2, 2*sizeof(unsigned char));
        }

        memcpy(pTmpZk, pzk1, iFont*iFont*sizeof(unsigned char));
        
    }
    
	if (iFont==64)
	{
		char p2[512];
		memset(p2, 0, sizeof(p2));
		zoom_hz((char*)pTmpZk, p2, 32, 32, 64);
		memset(pZk, 0, sizeof(pZk));
        /*
        int w=(nHalf>0)?32:64;
		bit2Byte(p2, w, 64, (char*)pZk);
		*/
        bit2Byte(p2, 64, 64, (char*)pZk);
        // 非汉字字符内存移动处理，去掉右半部的0
        if(nHalf !=0)
        {
            memset(pzk1, 0, iFont*iFont*sizeof(unsigned char));
            for(int i=0;i<iFont; i++)
            {
                memcpy(pzk1+i*iFont/2, pZk+iFont*i, iFont/2*sizeof(unsigned char));
            }
            memcpy(pZk, pzk1, iFont*iFont*sizeof(unsigned char));
        }
	}
	else
	{
		int iIndex = 0;
		for ( int i = 0; i < nStrideBits*iFont/8; i++ )
		{

			for ( int k = 7; k >= 0; k-- )
			{
				if ( pTmpZk[i] & (1 << k) )
				{
					//printf( "# " );
					pZk[iIndex] = 255; 
				}
				else
				{
					//printf("  " );
					pZk[iIndex] = 0;
				}	
				iIndex ++;
				//if ( iIndex % nStrideBits== 0 ) 
				//printf("\n");     
			}

		}
	}
	free(pTmpZk);
    free(pzk1);
    // 描边
    if(iFont >= 32 )
    {
        unsigned char *pucTmpCode = pZk;
        int iTmpHalf = nHalf;
        int iFontWidth = iFont / ((iTmpHalf>0)?2:1);
        int i, j, x, y;
        for(j=0; j<iFont; j++)
        {
            for(i=0; i<iFontWidth; i++)
            {
                if( pucTmpCode[j*iFontWidth+i] == 255 )
                {
                    // 描边
                    for(y=j-1; y<=j+1; y++)
                    {
                        for(x=i-1; x<=i+1; x++)
                        {
                            if(y<0 || y==iFont || x<0 || x==iFontWidth)
                            {
                                //如果当前点为点阵边缘，则描此点
                                pucTmpCode[j*iFontWidth+i] = 128;
                                continue;
                            }
                            // 描当前点的周围点
                            if( pucTmpCode[y*iFontWidth+x] == 0 )
                            {
                                pucTmpCode[y*iFontWidth+x] = 128;
                            }
                        }
                    }
                }
            }
        }
    }

	AddDzkToMem(nCode, iFont, pZk );
	return 0;

}




