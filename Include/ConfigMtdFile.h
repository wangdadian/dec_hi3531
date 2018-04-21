#ifndef _CONFIG_MTD_FILE_H
#define _CONFIG_MTD_FILE_H


#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libgen.h>
#include "define.h"
#include "Facility.h"
#include "IniFile.h"
#include "dwstype.h"

#include "PublicDefine.h"


#pragma pack(1)

/*
    写入config mtd分区的文件结构如下(不包括FileHeader头，见FlashRW.h)

    |---------------------------|        -->开始，整体文件头
    |----- MtdFileHeaderT ------|
    |___________________________|
    |                           |        -->第一个配置文件头
    |----- ConfigFileHeaderT ---|
    |                           |
    |----- ConfigFile#01 -------|        -->第一个配置文件内容
    |___________________________|
    |                           |
    |----- ConfigFIleHeaderT ---|        -->第二个文件
    |                           |
    |----- ConfigFile#02 -------|
    |___________________________|        -->第N个文件
    |                           |            
    |          .......          |
    |___________________________|

*/
typedef struct _mtd_file_header
{
	char sync[8]; //="MFHEADER"
    int count; // 文件个数
    //unsigned int length;// 所有文件长度
}MtdFileHeaderT;

typedef struct _config_file_header
{
	char sync[8]; //="CFHEADER"
	int size; // 文件长度
	char name[64]; // 文件名称，不包括路径
}ConfigFileHeaderT;

#define MTD_FILE_HEADER_SYNC "MFHEADER"
#define CFG_FILE_HEADER_SYNC "CFHEADER"

class CConfigMtdFile
{
public:
    CConfigMtdFile();
    ~CConfigMtdFile();

    /*
     * 将多个文件{szFilenameList}链接成一个文件{szDestFilename}用于写入config mtd
     */
    long JoinConfigFiles(const char* szDestFilename, int type=CFG_FILE_ALL);
	long JoinConfigFilesToMtd5(const char* szDestFilename, int type=CFG_FILE_ALL);
    long JoinConfigFiles(const char* szDestFilename, char** szFilenameList, \
                             unsigned int uiItemCount);

    /*
     * 解析从config mtd到处的文件{szMtdFilename}，成多个文件{szFilenameList}(包含路径)
     * 解析的文件存放在{szDestDir}目录下
     */
    long ExtracConfigFiles(const char* szMtdFilename, const char*szDestDir, \
                                 char** &szFilenameList, unsigned int &uiItemCount);

    long GetConfigFileList(char** &szFilenameList, unsigned int &uiItemCount, int iType=CFG_FILE_ALL);
    long GetConfigFileListToMtd5(char** &szFilenameList, unsigned int &uiItemCount, int iType=CFG_FILE_ALL);
     
private:
    //获取文件大小
    long GetFileSize( const char *szFileName );

private:
    
};

#pragma pack()


#endif //_CONFIG_MTD_FILE_H


