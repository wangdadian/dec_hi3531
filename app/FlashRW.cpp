
#include "FlashRW.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>


#include "PublicDefine.h"


#define FILEHEADERSYNC "package"

CFlashRW::CFlashRW()
{
    memset(m_szRootfsMtd, 0, 32);
    memset(m_szConfigMtd, 0, 32);
    sprintf(m_szConfigMtd, "%s", "/dev/mtd/2");
    sprintf(m_szRootfsMtd, "%s", "/dev/mtd/4");
}

CFlashRW::~CFlashRW()
{

}

unsigned int CFlashRW::CalcCRC( char *szInFile )
{

	unsigned int nCRC=0;
	FILE *hInFile = fopen( szInFile, "r" );

	char szBuf[2049]={0};
	ssize_t nRead = 0;
	ssize_t nWrite = 0;
	
	while( 1)
	{
		nRead = fread( (void*)szBuf, 1, 1024, hInFile );
		if ( nRead > 0 )
		{
			for ( int i = 0; i < nRead; i++ )
			{
				nCRC += (unsigned char)szBuf[i];
			}
		}
		else
		{
			break;
		}
		
	};
	fclose( hInFile );
	return nCRC;
	
}


unsigned int CFlashRW::CalcCRCPackageFile( char *szInFile )
{

	unsigned int nCRC=0;
	FILE *hInFile = fopen( szInFile, "r" );

	char szBuf[2049]={0};
	ssize_t nRead = 0;
	ssize_t nWrite = 0;
	fseek( hInFile, sizeof( FileHeader ), SEEK_SET );
	
	while( 1 )
	{
		nRead = fread( (void*)szBuf, 1, 1024, hInFile );
		if ( nRead > 0 )
		{
			for ( int i = 0; i < nRead; i++ )
			{
				nCRC += (unsigned char)szBuf[i];
			}
		}
		else
		{
			break;
		}
		
	};
	fclose( hInFile );
	return nCRC;
	
}



unsigned int CFlashRW::CalcCRC( char *szBuf, int nLen )
{

	unsigned int nCRC=0;

	for ( int i = 0; i < nLen; i++ )
	{
		nCRC += (unsigned char)szBuf[i];
	}
		
	return nCRC;
	
}


int CFlashRW::PackFile( char *szInFile, char *szOutFile,int nPackType )
{

	int nFileSize = GetFileSize(szInFile);

	if ( nFileSize <= 0 )
	{
		_DEBUG_( "input file %s len <=0 ", szInFile );
		return -1;
	}

	FileHeader rootfsheader;
	memset( &rootfsheader, 0, sizeof( FileHeader));
	strcpy( rootfsheader.szSync, FILEHEADERSYNC );
	rootfsheader.nPackType = nPackType;
	rootfsheader.nCRC = CalcCRC(szInFile);
	rootfsheader.nFileSize = nFileSize;
	
	FILE *hOutFile = fopen( szOutFile, "w" );
	if ( hOutFile == NULL )
	{
		_DEBUG_( "open output file %s error ", szOutFile );
		return -1;
	}
	
	ssize_t nWrite = 0;
	nWrite += fwrite( (void*)&rootfsheader, 1, sizeof( FileHeader),  hOutFile );
	
	FILE *hInFile = fopen( szInFile, "r" );
	if ( hInFile == NULL )
	{
		_DEBUG_( "open input file %s error ", szInFile );
		fclose( hOutFile);
		return -1;
	}

	char szBuf[2049]={0};
	ssize_t nRead = 0;
	
	while( 1)
	{
		nRead = fread( (void*)szBuf, 1, 1024, hInFile );
		if ( nRead > 0 )
		{
			nWrite += fwrite( (void*)szBuf, 1, 1024, hOutFile );
		}
		else
		{
			break;
		}
		
	};
	fclose( hInFile );
	fclose( hOutFile );
		
	return nWrite;
}

int CFlashRW::ImportFileToMtd( char *szMtd, char *szFileName )
{
	int nFileSize = GetFileSize(szFileName);

	if ( nFileSize <= 0 )
	{
		_DEBUG_( "file %s size <=0", szFileName );
		return -1;
	}

	FILE *hFile = fopen( szFileName, "r" );
	if ( hFile == NULL )
	{
		_DEBUG_( "open file error:%s", szFileName );
		return -1;
	}
	
	char *szBuf = (char*)malloc(nFileSize);
	int nRd = fread( (void*)szBuf, 1, nFileSize, hFile );
	
	WriteConfigMtd(szMtd, szBuf, nRd );

	if (szBuf != NULL ) 
	{
		free(szBuf);
		szBuf = NULL;
	}
	fclose( hFile );
	_DEBUG_( "%d bytes imported to %s", nRd, szMtd );
	return nRd;

}

int CFlashRW::ImportConfigToMtd( char *szMtd, char *szFileName )
{
    return ImportFileToMtd( szMtd, szFileName);
}
int CFlashRW::ImportConfigToMtd( char *szFileName )
{
    return ImportFileToMtd( m_szConfigMtd, szFileName);

}
int CFlashRW::ImportConfigToMtd5( char *szFileName )
{
    return ImportFileToMtd( "/dev/mtd/5", szFileName);
	//return ImportFileToMtd( "/dev/mtd/2", szFileName);
}

int CFlashRW::DumpRootfsMtd(char *szFileName )
{
    return DumpMtd(m_szRootfsMtd, szFileName);
}

int CFlashRW::DumpConfigMtd( char *szFileName )
{
    return DumpMtd(m_szConfigMtd, szFileName);
}
int CFlashRW::DumpConfigMtd5( char *szFileName )
{
    return DumpMtd("/dev/mtd/5", szFileName);
	//return DumpMtd("/dev/mtd/2", szFileName);
}

int CFlashRW::DumpConfigMtd( char *szMtd, char *szFileName )
{
    return DumpMtd(szMtd, szFileName);
}

int CFlashRW::DumpMtd( char *szMtd, char *szFileName )
{
	char *szBuf = NULL;
	int nLen = ReadMtd( szMtd,  szBuf);

	if ( nLen <= 0 )
	{
		if ( szBuf != NULL )
		{
			free(szBuf );
		}
		return -1;
	}

	FILE *hFile = fopen( szFileName, "w+" );
	if ( hFile == NULL )
	{
		_DEBUG_( "open file error:%s", szFileName );
		if ( szBuf != NULL )
		{
			free(szBuf );
		}
		return -1;

	}
	_DEBUG_( "start dump to file: %s", szFileName );
	nLen = fwrite( (void*)szBuf, 1, nLen, hFile );
	_DEBUG_("dump to file:%s succedded.", szFileName );
	fclose( hFile );
	if ( szBuf != NULL )
	{
		free(szBuf );
	}
	return nLen;
}

int CFlashRW::ReadMtd(char* szMtd, char*& szBuf)
{

	szBuf = NULL;	
	int nMtdSize = GetMtdSize( szMtd );
	if(nMtdSize<0)
        return -1;
	
	int nDevFd = open( szMtd, O_SYNC | O_RDWR );	

	if ( nDevFd < 0 )
	{
		_DEBUG_("open %s mtd error.", szMtd );
		return -1;
	}


	FileHeader fheader;
	memset( (void*)&fheader, 0, sizeof( FileHeader ));

	int nLen = 0;
	int nTmp = read(nDevFd, (void*)&fheader, sizeof( FileHeader));
	if ( nTmp != sizeof( fheader) )
	{
		_DEBUG_( "read mtd %s error.", szMtd );
		close( nDevFd );
		return -1;
	}

	if ( strstr( FILEHEADERSYNC, fheader.szSync ) == NULL )
	{
		_DEBUG_( "read mtd %s error invalid format: %s.", szMtd, fheader.szSync );
		close(nDevFd);
		return -1;
	}


	//if ( nLen <= 0 || fheader.nFileSize > nMtdSize )
	if ( fheader.nFileSize > (unsigned int)nMtdSize )
	{
		_DEBUG_( "file length %d > mtd size %d ,error.", fheader.nFileSize, nMtdSize);
        close(nDevFd);
		return -1;
	}

	szBuf = (char*)malloc( fheader.nFileSize );
	ssize_t lRd = read( nDevFd, szBuf,fheader.nFileSize );
	_DEBUG_( "%d bytes are read", lRd );
	close( nDevFd );
	return lRd;

}

int CFlashRW::ReadConfigMtd( char *&szBuf )
{
    return ReadMtd(m_szConfigMtd, szBuf);
}

int CFlashRW::ReadConfigMtd( char *szMtd, char *&szBuf )
{
    return ReadMtd(szMtd, szBuf);
}

int CFlashRW::ReadRootfsMtd( char *&szBuf )
{
    return ReadMtd(m_szRootfsMtd, szBuf);
}

int CFlashRW::WriteConfigMtd( char *szBuf, int nLen )
{
    return WriteConfigMtd(m_szConfigMtd, szBuf, nLen);
}

int CFlashRW::WriteRootfsMtd( char *szFileName )
{
    return WriteRootfsMtd( m_szRootfsMtd, szFileName);
}

int CFlashRW::WriteConfigMtd( char *szMtd, char *szBuf, int nLen )
{
	if ( szBuf == NULL || nLen <= 0 )
	{
		_DEBUG_("empty contents");
		return -1;
	}

	if ( nLen > GetMtdSize( szMtd ) )
	{
		_DEBUG_( "error:not enough mtd size. " );
		return -1;
	}

	if ( EraseMtd( szMtd ) < 0 )
	{
		_DEBUG_("erase mtd:%s error", szMtd );
		return -1;
	}
	
	int nDevFd = open( szMtd, O_SYNC | O_RDWR );	

	if ( nDevFd < 0 )
	{
		_DEBUG_("open %s mtd error.", szMtd );
		return -1;
	}

	//write length first.
	

	FileHeader rootfsheader;
	memset( (void*)&rootfsheader, 0, sizeof( FileHeader ));
	strcpy( rootfsheader.szSync, FILEHEADERSYNC );
	rootfsheader.nPackType = PACKTYPE_CONFIG;
	rootfsheader.nCRC = CalcCRC(szBuf, nLen);
	rootfsheader.nFileSize = nLen;
	write( nDevFd, (void*)&rootfsheader, sizeof(FileHeader));
	
	ssize_t len = write( nDevFd, szBuf,nLen );
	_DEBUG_( "%d bytes are written", len );
	
	close( nDevFd );
	return 0;
}

int CFlashRW::CheckFileInvalid( char *szMtd, char *szFileName )
{
	long lFileSize = GetFileSize( szFileName);
	long lMtdSize = GetMtdSize(  szMtd);

	if ( lMtdSize <= lFileSize )
	{
		_DEBUG_(" error: file size > mtd %s size ", szMtd );
		return -1;
	}

	FileHeader fheader;
	int nRd = ReadHeader(  szFileName,  fheader);
	if (nRd < 0 )
	{
		_DEBUG_( "read header error");
		return -1;
	}
	
	if ( strstr( FILEHEADERSYNC, fheader.szSync ) == NULL )
	{
		_DEBUG_("invalid file sync.");
		return -1;
	}

	if( fheader.nPackType > PACKTYPE_INVALID || fheader.nPackType <= 0 )
	{
		_DEBUG_( "invalid pack type:%d", fheader.nPackType );
		return -1;
	}
	_DEBUG_("sync ok!");
	if ( fheader.nFileSize != (lFileSize - sizeof(FileHeader) ) )
	{
		_DEBUG_("invalid file size.");
		return -1;
	}

	_DEBUG_("file size ok!");

	if ( CalcCRCPackageFile( szFileName ) != fheader.nCRC )
	{
		_DEBUG_( "invalid crc.");
		return -1;
	}
	_DEBUG_("CRC ok!");

	return 0;
}

int CFlashRW::ReadHeader( char *szFileName,  FileHeader &fheader )
{
	FILE *hFile = fopen(szFileName, "r" );
	if ( hFile == NULL)
	{
		_DEBUG_( "open file [%s] error. ", szFileName );
		return -1;
	}

	int nRd = fread( (void*)&fheader,1, sizeof(fheader),  hFile );
    fclose( hFile );

	if ( nRd != sizeof(fheader))
	{
		_DEBUG_("error: read file error.");
		return -1;
	}

	if ( strstr( FILEHEADERSYNC, fheader.szSync ) == NULL )
	{
		_DEBUG_("invalid file sync.");
		return -1;
	}

	return 0;
}


int CFlashRW::WriteMtd( char *szFileName )
{
	
	FileHeader fheader;
	if( ReadHeader( szFileName, fheader ) < 0 )
	{
		return -1;
	}

	switch( fheader.nPackType )
	{
		case (int)PACKTYPE_ROOTFS:
			WriteRootfsMtd( m_szRootfsMtd, szFileName );
			break;
		default:
			_DEBUG_("invalid package type:%d", fheader.nPackType );
			return -1;
	};
	
	return 0;
}

int CFlashRW::WriteRootfsMtd( char *szMtd, char *szFileName )
{
	if ( CheckFileInvalid( szMtd, szFileName) < 0 ) 
	{
		return -1;
	}

	FILE *hFile = fopen(szFileName, "r" );
	if ( hFile == NULL)
	{
		_DEBUG_( "open file [%s] error. ", szFileName );
		return -1;
	}

	if ( EraseMtd( szMtd ) < 0 )
	{
		_DEBUG_("erase mtd:%s error", szMtd );
        fclose(hFile);
		return -1;
	}
	
	int nDevFd = open( szMtd, O_SYNC | O_RDWR );	
	if ( nDevFd < 0 )
	{
		_DEBUG_("open %s mtd error.", szMtd );
        fclose(hFile);
		return -1;
	}

	_DEBUG_("start write...");
	char szBuf[2048] = {0};
	ssize_t nRead = 0;
	ssize_t nWrite = 0;
	fseek( hFile, sizeof(FileHeader), SEEK_SET );
	while( 1)
	{
		nRead = fread( (void*)szBuf, 1, 1024, hFile );
		if ( nRead > 0 )
		{
			nWrite += write( nDevFd, szBuf, nRead );
		}
		else
		{
			break;
		}
	}

	_DEBUG_( "%d bytes are written.", nWrite);

	fclose(hFile);
	close (nDevFd );
    return 0;	
}


long CFlashRW::GetFileSize( char *szFileName )
{
	FILE *hFile = fopen(szFileName, "r" );
	if ( hFile == NULL)
	{
		_DEBUG_( "open file [%s] error. ", szFileName );
		return -1;
	}

	fseek( hFile, 0L, SEEK_END ); 
	long length = ftell( hFile );
	fclose( hFile );
	return length;
}

long CFlashRW::GetMtdSize( char *szMtd )
{
	mtd_info_user mtd;
	memset( &mtd, 0, sizeof(mtd));	
	
	int nDevFd = open( szMtd, O_SYNC | O_RDWR );	
	if ( nDevFd < 0 )
	{
		_DEBUG_("open %s mtd error.", szMtd );
		return -1;
	}
	
	ioctl(nDevFd, MEMGETINFO, (void*)&mtd);
	_DEBUG_( "mtd size:%d erase size:%d", mtd.size, mtd.erasesize );

	close(nDevFd );
	return mtd.size;
}

int CFlashRW::EraseMtd( char *szMtd )
{
	mtd_info_user mtd;
	memset( &mtd, 0, sizeof(mtd));	
	
	int nDevFd = open( szMtd, O_SYNC | O_RDWR );
	if ( nDevFd < 0 )
	{
		_DEBUG_("open %s mtd error.", szMtd );
		return -1;
	}
	
	ioctl(nDevFd, MEMGETINFO, (void*)&mtd);
	_DEBUG_( "mtd size:%d erase size:%d", mtd.size, mtd.erasesize );

	erase_info_user erase;
	memset( &erase, 0, sizeof(erase));

	erase.start = 0;
	erase.length = ((int)(mtd.size / mtd.erasesize)) * mtd.erasesize;
	/*if(strcmp(szMtd, "/dev/mtd/5")==0)
	{// 第5个分区只擦写100K
		erase.length=500*1024;
	}*/

	_DEBUG_("start erase[%s], start:%d length:%d", szMtd, erase.start, erase.length );
	ioctl( nDevFd, MEMERASE, (void*)&erase);
	_DEBUG_( "erase end.");

	close( nDevFd );
	return erase.length;
}





