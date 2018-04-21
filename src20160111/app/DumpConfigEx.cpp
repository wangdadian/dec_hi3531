#include <stdio.h>
#include <stdlib.h>
#include "FlashRW.h"
#include "ConfigMtdFile.h"

int main( int argc, char **argv )
{

	if ( argc < 2 ) 
	{
		printf("%s [dest dir]\n", argv[0] );
		return 1;
	}
    long lRet = 0;
    char ** szFileList;
    char *szTmpMtdFile = (char*)"/tmp/tmp_mtd_file";
	char *szTmpMtd5File = (char*)"/tmp/tmp_mtd5_file";
    unsigned int uiCount = 0;
    unsigned int i=0;
	CFlashRW *pFlashRW= new CFlashRW();
    CConfigMtdFile *pCfgMtdFile = new CConfigMtdFile;

    // 导出配置分区至文件
	lRet = pFlashRW->DumpConfigMtd(szTmpMtdFile);
    if( lRet <= 0 )
    {
        delete pFlashRW;
        delete pCfgMtdFile;
        printf("Dump config mtd to file failed!\n");
        return 1;
    }
    
    // 析出分区文件中的配置文件到指定目录
    lRet = pCfgMtdFile->ExtracConfigFiles(szTmpMtdFile, argv[1], 
                                 szFileList, uiCount);
    if( lRet != 0 )
    {
        delete pFlashRW;
        delete pCfgMtdFile;
        printf("Extrac Config Files failed!\n");
        return 1;
    }
    printf("Extraced Config Files: \n");
    for(i=0; i<uiCount; i++)
    {
        printf("--%s\n", szFileList[i]);
    }

    // 释放内存、删除临时文件
    for(i=0; i<uiCount; i++)
    {
        delete[] szFileList[i];
        szFileList[i] = NULL;
    }
    delete [] szFileList;
    szFileList = NULL;
    remove(szTmpMtdFile);

	///////////////////////
	//mtd5

    // 导出配置分区至文件
	lRet = pFlashRW->DumpConfigMtd5(szTmpMtd5File);
    if( lRet <= 0 )
    {
        delete pFlashRW;
        delete pCfgMtdFile;
        printf("Dump config mtd to file failed!\n");
        return 1;
    }
    
    // 析出分区文件中的配置文件到指定目录
    lRet = pCfgMtdFile->ExtracConfigFiles(szTmpMtd5File, argv[1], 
                                 szFileList, uiCount);
    if( lRet != 0 )
    {
        delete pFlashRW;
        delete pCfgMtdFile;
        printf("Extrac Config Files failed!\n");
        return 1;
    }
    printf("Extraced Config Files mtd5: \n");
    for(i=0; i<uiCount; i++)
    {
        printf("--%s\n", szFileList[i]);
    }

    // 释放内存、删除临时文件

    for(i=0; i<uiCount; i++)
    {
        delete[] szFileList[i];
        szFileList[i] = NULL;
    }
    delete [] szFileList;
    szFileList = NULL;
    remove(szTmpMtd5File);

	
	delete pFlashRW;
	pFlashRW = NULL;
    delete pCfgMtdFile;
    pCfgMtdFile = NULL;

	return 0;
}




