
#pragma once




/*
struct mtd_info_user{
	unsigned int size;
	unsigned int erasesize;
};


struct erase_info_user{
	unsigned int start;
	unsigned int length;
};
*/
#pragma pack(1)
enum PACKTYPE
{
	PACKTYPE_ROOTFS = 0x01,
	PACKTYPE_KERNEL = 0x02,
	PACKTYPE_CONFIG = 0x03,
	PACKTYPE_LOGO = 0x04,
	PACKTYPE_INVALID,
};

typedef struct _fileheader
{
	char szSync[8]; //="package"
	int nPackType;
	unsigned int nCRC;
	unsigned int nFileSize;
	char szReserved[100];
}FileHeader;

class CFlashRW
{
public:
	CFlashRW();
	~CFlashRW();


	int ReadConfigMtd( char *szMtd, char *&szBuf );
    int ReadConfigMtd( char *&szBuf );
	int ReadRootfsMtd( char *&szBuf );
    
	int WriteConfigMtd( char *szMtd, char *szBuf, int nLen );
	int WriteRootfsMtd( char *szMtd, char *szFileName );
    int WriteConfigMtd( char *szBuf, int nLen );
	int WriteRootfsMtd( char *szFileName );
    
	int DumpConfigMtd( char *szMtd, char *szFileName );
    int DumpConfigMtd(char *szFileName );
	int DumpConfigMtd5(char *szFileName );
    int DumpRootfsMtd( char *szFileName );

	int ImportConfigToMtd( char *szMtd, char *szFileName );
    int ImportConfigToMtd( char *szFileName );
	int ImportConfigToMtd5( char *szFileName );

	int PackFile( char *szInFile, char *szOutFile, int nPackType );

	int WriteMtd( char *szFileName );

private:
    int ImportFileToMtd( char *szMtd, char *szFileName );
    int DumpMtd( char *szMtd, char *szFileName );
	int ReadMtd(char* szMtd, char*& szBuf);
	int EraseMtd( char *szMtd );
	
	long GetFileSize( char *szFileName );
	
	long GetMtdSize( char *szMtd );

	
	unsigned int CalcCRC( char *szInFile );

	
	unsigned int CalcCRCPackageFile( char *szInFile );


	
	unsigned int CalcCRC( char *szBuf, int nLen );


	
	int CheckFileInvalid( char *szMtd, char *szFileName );


	
	int ReadHeader( char *szFileName, FileHeader &fheader );


private:
    // ????MTD
    char m_szConfigMtd[32];
    // rootfs MTD
    char m_szRootfsMtd[32];

};

#pragma pack()

