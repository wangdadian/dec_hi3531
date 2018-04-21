#include "DecoderSetting.h"
#include <math.h>

CDecoderSetting::CDecoderSetting()
{
    m_pDWConfig = new CDWConfig;
    m_pWebDecoder = new CWebDecoder;
    m_pFlashRW  = new CFlashRW;
    m_pConfigMtdFile = new CConfigMtdFile;
    m_lConfigMtdLen = 0;
	m_cbSetResolution = NULL;
    m_cbSetOsdShow = NULL;
	m_cbSetDDU = NULL;
    m_pUserInfo = NULL;
    m_iUserCount = 0;
    memset(&m_objNtpInfo, 0, sizeof(struNtpInfo));
    memset(&m_objARInfo, 0, sizeof(struARInfo));
    m_bNtpWorkerExit = false;
    pthread_mutex_init(&m_ntpMutexLock, NULL);
    m_bARWorkerExit = false;
    pthread_mutex_init(&m_arMutexLock, NULL);
    
	m_bWriteMtdExit = false;
	m_bCfgFileChanged = false;
	m_bWriteMtdNow = false;
	pthread_mutex_init(&m_mtdlock, NULL);
}
CDecoderSetting::~CDecoderSetting()
{
    m_bNtpWorkerExit = true;
    if( m_thNtpWorker != 0 )
    {
        pthread_join(m_thNtpWorker, NULL);
        m_thNtpWorker = 0;
    }
    pthread_mutex_destroy(&m_ntpMutexLock);

	m_bWriteMtdExit = true;
    if( m_thWriteMtdWorker != 0 )
    {
        pthread_join(m_thWriteMtdWorker, NULL);
        m_thWriteMtdWorker = 0;
    }

    m_bARWorkerExit = true;
    if( m_thARWorker != 0 )
    {
        pthread_join(m_thARWorker, NULL);
        m_thARWorker = 0;
    }
    pthread_mutex_destroy(&m_arMutexLock);
    
	m_bCfgFileChanged = false;
	m_bWriteMtdNow = false;
    pthread_mutex_destroy(&m_mtdlock);

    PTR_DELETE(m_pDWConfig);
    PTR_DELETE(m_pWebDecoder);
    PTR_DELETE(m_pFlashRW);
    PTR_DELETE(m_pConfigMtdFile);    
    PTR_DELETE_A(m_pUserInfo);
    m_iUserCount = 0;
    m_lConfigMtdLen = 0;
}

long CDecoderSetting::Init()
{
    m_pDWConfig->SetCB_UserLogin(UserLogin, (void*)this);
    m_pDWConfig->SetCB_GetUserInfo(GetUserInfo, (void*)this);
    m_pDWConfig->SetCB_EditUserInfo(EditUserInfo, (void*)this);
    m_pDWConfig->SetCB_GetNetworkInfo(GetNetworkInfo, (void*)this);
    m_pDWConfig->SetCB_SetNetworkInfo(SetNetworkInfo, (void*)this);
    m_pDWConfig->SetCB_GetSipServer(GetSipServer, (void*)this);
    m_pDWConfig->SetCB_SetSipServer(SetSipServer, (void*)this);
    m_pDWConfig->SetCB_UpgradeDev(UpgradeDevice, (void*)this);
    m_pDWConfig->SetCB_GetDeviceMac(GetDeviceMac, (void*)this);
    m_pDWConfig->SetCB_SetDeviceMac(SetDeviceMac, (void*)this);
    m_pDWConfig->SetCB_GetDecChannelInfo(GetDecChannelInfo, (void*)this);
    m_pDWConfig->SetCB_SetDecChannelInfo(SetDecChannelInfo, (void*)this);
    m_pDWConfig->SetCB_GetResolution(GetResolution, (void*)this);
    m_pDWConfig->SetCB_SetResolution(SetResolution, (void*)this);
    m_pDWConfig->SetCB_GetDynamicDecUrl(GetDynamicDecUrl, (void*)this);
    m_pDWConfig->SetCB_SetDynamicDecUrl(SetDynamicDecUrl, (void*)this);
    m_pDWConfig->SetCB_GetDecOsdShow(GetDecOsdInfoWeb,(void*)this);
    m_pDWConfig->SetCB_SetDecOsdShow(SetDecOsdInfoWeb,(void*)this);    
    m_pDWConfig->SetCB_ResetParam(ResetParam, (void*)this);
    m_pDWConfig->SetCB_GetHostname(GetHostname, (void*)this);
    m_pDWConfig->SetCB_SetHostname(SetHostname, (void*)this);
    m_pDWConfig->SetCB_GetDeviceModel(GetDeviceModel, (void*)this);
    m_pDWConfig->SetCB_SetDeviceModel(SetDeviceModel, (void*)this);
    m_pDWConfig->SetCB_GetDeviceSN(GetDeviceSN, (void*)this);
    m_pDWConfig->SetCB_SetDeviceSN(SetDeviceSN, (void*)this);
    m_pDWConfig->SetCB_GetDeviceVer(GetDeviceVer, (void*)this);
    m_pDWConfig->SetCB_GetDevBuildtime(GetDevBuildtime, (void*)this);
    m_pDWConfig->SetCB_SetSysTime(SetSysTime, (void*)this);
    m_pDWConfig->SetCB_GetSysTime(GetSysTime, (void*)this);
    m_pDWConfig->SetCB_GetNtp(GetNtp, (void*)this);
    m_pDWConfig->SetCB_SetNtp(SetNtp, (void*)this); 
    m_pDWConfig->SetCB_GetAR(GetAR, (void*)this);
    m_pDWConfig->SetCB_SetAR(SetAR, (void*)this); 
    // 获取通道个数
    m_pDWConfig->SetCB_GetChannelCount(GetChannelCount, (void*)this);

    //获取NTP信息
    GetNtpInfo(&m_objNtpInfo);
    // 启动NTP校时线程
    pthread_create(&m_thNtpWorker, NULL, ThNtpWorker, (void*)this);

    // 获取自动重启设置信息
    GetARInfo(&m_objARInfo);
    pthread_create(&m_thARWorker, NULL, ThARWorker, (void*)this);

	// 启动写MTD配置分区线程
	
    pthread_create(&m_thWriteMtdWorker, NULL, ThWriteMtdWorker, (void*)this);
    return 0;
}
void* CDecoderSetting::ThWriteMtdWorker(void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return NULL;
    }
    _DEBUG_("start!");
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    long lRet = 0;
    char szCfgFile[128]={0};
	sprintf(szCfgFile, "%s", "/tmp/mytmp_config_mtd5_file");;

	CConfigMtdFile mtdfile;
	CFlashRW flashrw;
	int iInterval = 0;
	while (pThis->m_bWriteMtdExit == false)
	{
		iInterval += 1;
		if(pThis->m_bCfgFileChanged == true &&
			(iInterval >= 15 ||
			pThis->m_bWriteMtdNow == true)
		   )
		{
			// 组成config分区文件
		    lRet = mtdfile.JoinConfigFilesToMtd5(szCfgFile);
		    if(lRet != 0)
		    {
		    	_DEBUG_("Join Config file failed!");
		        continue;
		    }
		    // 写入config分区
		    lRet = (long)(flashrw.ImportConfigToMtd5(szCfgFile));

		    // 移除临时文件
		    remove(szCfgFile);

			//状态变量重置
			pthread_mutex_lock(&pThis->m_mtdlock);
			pThis->m_bCfgFileChanged = false;
			pThis->m_bWriteMtdNow = false;
			pthread_mutex_unlock(&pThis->m_mtdlock);
			iInterval = 0;
		}
		CFacility::SleepSec(1);
	}
    _DEBUG_("exit!");
	return NULL;
}
void* CDecoderSetting::ThNtpWorker(void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return NULL;
    }
    _DEBUG_("start ThNtpWorker!");
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    int iTime = 0;
    int iInterval = 0;
    while(pThis->m_bNtpWorkerExit != true)
    {
        pthread_mutex_lock(&pThis->m_ntpMutexLock);
        iInterval = pThis->m_objNtpInfo.iInterval * 60;
        if(iInterval < 1)//至少1分钟
        {
            iInterval = 1;
        }
        if(iInterval > 600)//最多10小时
        {
            iInterval = 600;
        }
		pThis->m_objNtpInfo.iEnable = 0;
        if(pThis->m_objNtpInfo.iEnable == 1 && iTime >=iInterval &&
            CFacility::bIpAddress(pThis->m_objNtpInfo.ip))
        {
            // 校时
            pThis->NtpSetTime(&(pThis->m_objNtpInfo));
            iTime = 0;
        }
        else
        {
            //_DEBUG_("NTPDATE DISABLED!");
        }
        pthread_mutex_unlock(&pThis->m_ntpMutexLock);
        CFacility::SleepSec(1);
        iTime++;
    }
    _DEBUG_("exit ThNtpWorker!");
    return NULL;
}	
long CDecoderSetting::NtpSetTime(struNtpInfo *ntp)
{
    if(ntp == NULL || ntp->iEnable != 1)
    {
        return -1;
    }
    char szCmd[100] = {0};
	strcpy(ntp->ip, "127.0.0.1" );
    sprintf(szCmd, "/opt/dws/ntpdate -u %s", ntp->ip);
	// 命令执行失败
	int iRet = system(szCmd);
	if(iRet != 0)
	{
		return iRet;
	}
    //DEBUG_("NTPDATE %s", ntp->ip);
    // 默认开启UTC，手动修改时间
    time_t iTime = time(NULL) + 8*3600; // +8北京时间
    CRtc::write_rtc(iTime);
	
    // 更新系统时间
    char szTime[64] = {0};
    sprintf(szTime, "%s", CFacility::FormatTime(iTime));
    sprintf(szCmd, "date -s \"%s\"", szTime);
    iRet = system(szCmd);
    
    return iRet;
}

//获取NTP信息
long CDecoderSetting::GetNtpInfo(struNtpInfo* ntp)
{
    if(ntp == NULL)
        return -1;
    
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 获取NTP信息
    ntp->iEnable = pro.GetProperty((char*)"NTP_SERVER", (char*)"ENABLE", 0);
    pro.GetProperty((char*)"NTP_SERVER", (char*)"SERVERIP", ntp->ip, 32, (char*)"");
    ntp->iInterval = pro.GetProperty((char*)"NTP_SERVER", (char*)"INTERVAL", 0);
    return 0;
}

void* CDecoderSetting::ThARWorker(void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return NULL;
    }
    _DEBUG_("start ThARWorker!");
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    time_t t = 0;
    tm Time;
    // 当前时间信息
    int iWday = 0; // 一周中的第几天
    int iHour = 0; // 小时
    int iMin  = 0; // 分钟
    int iSec  = 0; // 秒

    // 自动重启设置的时间信息
    int iARHour = 0; 
    int iARMin  = 0;
    int iARSec  = 0;

     char* szPtr = NULL;
     char *szDelim1 = (char*)":";
     char szTmpBuf[256]={0};
    
    while(pThis->m_bARWorkerExit != true)
    {
        pthread_mutex_lock(&pThis->m_arMutexLock);
        if(pThis->m_objARInfo.iEnable == 0) //不生效
        {
            pthread_mutex_unlock(&pThis->m_arMutexLock);
            CFacility::SleepSec(1);
            continue;
        }
        
        // 已生效的自动重启处理
        // 获取当前时间
        //t = time(NULL); //获取的时间不准
        t = CRtc::read_rtc();
        //setenv("TZ", "GMT-8", 1);
        //tzset();
    	localtime_r((time_t*)&t, &Time);
    	
        iWday = Time.tm_wday;// 一周中的第几天，从周日开始0-6
        //转换成从周一开始1-7
        if(iWday == 0)
        {
            iWday = 7;
        }        
        
        
        iHour = Time.tm_hour;
        iMin  = Time.tm_min;
        iSec  = Time.tm_sec;
        // 解析设置重启时间的时分秒
        szPtr = NULL;
        memset(szTmpBuf, 0, 256);
        strcpy(szTmpBuf, pThis->m_objARInfo.szTime); // 02:00:00

        szPtr = strtok(szTmpBuf, szDelim1);
        iARHour = atoi(szPtr);

        szPtr = strtok(NULL, szDelim1);
        iARMin = atoi(szPtr);
        
        szPtr = strtok(NULL, szDelim1);
        iARSec = atoi(szPtr);
        
        // 检测是否需要重启
        if(pThis->m_objARInfo.iType == 0 //每天
           ||
           pThis->m_objARInfo.iType == iWday //达到设定的每周的星期#
           )
        {
            if(iHour == iARHour && iARMin==iMin)
            {
                // 临近5秒内重启，避免因sleep不精确导致错过设置的精确的秒
                if( abs(iARSec - iSec) <= 5) 
                {
                    pthread_mutex_unlock(&pThis->m_arMutexLock);
                    // 重启
                    _DEBUG_("reboot device!");
                    system("/sbin/reboot");
                }
            }
        }

        pthread_mutex_unlock(&pThis->m_arMutexLock);
        CFacility::SleepSec(1);

    }
    _DEBUG_("exit ThARWorker!");
    return NULL;
}	




//获取自动重启信息
long CDecoderSetting::GetARInfo(struARInfo* ar)
{
    if(ar == NULL)
        return -1;
    
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 获取自动重启信息
    ar->iEnable = pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_ENABLE", 0);
    ar->iType= pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_TYPE", 0);
    pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_TIME", ar->szTime, 32, (char*)"02:00:00");
    //_DEBUG_("AutoReboot Info: %d;%d;%s;", ar->iEnable, ar->iType, ar->szTime);
    return 0;
}


// 获取通道个数
long CDecoderSetting::GetChannelCount(int iMode, int &iCount)
{
    return GetChannelCount(iMode, iCount, (void *)this);
}

long CDecoderSetting::ReadDynamicDecUrl(int iChn, char*& szUrl)
{
    szUrl = NULL;
    int iLen = 0;
    long lRet = 0;
    struDynamicDecUrl sDDU;
    memset(&sDDU, 0, sizeof(struDynamicDecUrl));
    sDDU.channel = iChn;
    lRet = GetDynamicDecUrl(&sDDU, (void*)this);
    if(lRet == 0 )
    {
        iLen = strlen(sDDU.url);
        if(iLen > 0)
        {
            szUrl = new char[iLen+1];
            strncpy(szUrl, sDDU.url, iLen);
            szUrl[iLen]='\0';
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return lRet;
    }
}

long CDecoderSetting::ReadDynamicDecUrl(int iChn, struDynamicDecUrl* pDDU)
{
    long lRet = 0;
    memset(pDDU, 0, sizeof(struDynamicDecUrl));
    pDDU->channel = iChn;
    lRet = GetDynamicDecUrl(pDDU, (void*)this);
    return lRet;
}

////////////////////////////////////////////////////////////////////////////////////
//用于回调的函数
long CDecoderSetting::UserLogin(char* szUserName, char* szPassword, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    pThis->load_userinfo();
    return pThis->check_auth(szUserName, szPassword);
}

long CDecoderSetting::GetUserInfo(struUserInfo* &pUserInfo, int &iUserCount, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    pThis->load_userinfo();
    pUserInfo = NULL;
    iUserCount = 0;

    if ( pThis->m_pUserInfo == NULL || pThis->m_iUserCount <= 0) 
    {
        return -1;
    }
    pUserInfo = new struUserInfo[pThis->m_iUserCount];
    if (pUserInfo == NULL) 
    {
        return -1;
    }
    iUserCount = pThis->m_iUserCount;
    for(int i=0; i<iUserCount; i++)
    {
        memcpy(&pUserInfo[i], &(pThis->m_pUserInfo[i]), sizeof(struUserInfo));
        memset((char*)pUserInfo[i].pass, 0, sizeof(char)*32);
    }
    return 0;
}

// 修改用户信息
long CDecoderSetting::EditUserInfo(const struUserInfo* pUserInfo, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    pThis->load_userinfo();
    long lRet = 0;
    CProfile pro;
    char szSection[32] = {0};
    char szName[32] = {0};
    
    // 加载配置文件
    if (pro.load((char*)CFG_FILE_USER) < 0)
    {
        printf("load file failed [FileName=%s]\n", CFG_FILE_USER);
        return -1;
    }
    for( int i=0; i<pThis->m_iUserCount; i++)
    {
        sprintf(szSection, "USER_%d", (i+1));
        pro.GetProperty(szSection, (char*)"name", szName, 32, (char*)"0");
        if ( strcmp(szName, pUserInfo->name) ==0 )
        {
            pro.SetProperty(szSection, (char*)"pass", (char*)pUserInfo->pass);
            pro.SetProperty(szSection, (char*)"flag", (long)pUserInfo->flag);
            // 再次加载以保存
            pro.load((char*)CFG_FILE_USER);
            // 写入分区保存
            lRet = pThis->WriteCfgFileToCfgMtd();
            return lRet;
        }
    }
    // 没有找到用户，添加
    pro.SetProperty((char*)"COUNT", (char*)"UserCount", (pThis->m_iUserCount+1));
    sprintf(szSection, "USER_%d", (pThis->m_iUserCount+1));

    pro.SetProperty(szSection, (char*)"name", (char*)pUserInfo->name);
    pro.SetProperty(szSection, (char*)"pass", (char*)pUserInfo->pass);
    pro.SetProperty(szSection, (char*)"flag", (long)pUserInfo->flag);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_USER);
    pThis->load_userinfo();

    // 写入分区保存
    lRet =  pThis->WriteCfgFileToCfgMtd();
    return lRet;
}

long CDecoderSetting::GetDeviceMac(char *szMAC, void* param)
{
     FILE* fp = fopen(CFG_FILE_APPLYMAC, "r");
     if(fp == NULL)
     {
         _DEBUG_("open file\"%s\" failed!", CFG_FILE_APPLYMAC);
         return -1;
     }
     char szBuf[1024] = {0};
     while( 1 )
     {
         memset(szBuf, 0, 1024);
         if( fgets(szBuf, 1024, fp) == NULL) 
             break;
         // 注释行、空行
         if( szBuf[0] == '#' || szBuf[0] == '\0')
             continue;
         
         // ifconfig eth0 hw ether 00:16:17:30:A1:68
         if( strstr(szBuf, "ifconfig eth0 hw ether") != NULL) 
         {
             sscanf(szBuf, "ifconfig eth0 hw ether %s", szMAC);
         }
         else
         {
             continue;
         }
     }
     fclose(fp);
     return 0;

}

long CDecoderSetting::SetDeviceMac(const char *szMAC, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    char *szFile = (char*)CFG_FILE_APPLYMAC;
    char *szTmpFile = (char*)"/tmp/tmp_applymac.sh";
    FILE *fp, *tmp_fp;
    char szBuff[1024] = {0};
    char szTmpBuff[1024] = {0};
    long lRet = 0;
    
    fp = fopen(szFile, "r");
    tmp_fp = fopen(szTmpFile, "w+b");
    if( fp == NULL || tmp_fp == NULL )
    {
        fclose(fp);
        fclose(tmp_fp);
        return -1;
    }

    while( 1 )
    {
        memset(szBuff, 0, 1024);
        memset(szTmpBuff, 0, 1024);
        if( fgets(szBuff, 1024, fp) == NULL) 
            break;
        // 注释行
        if( szBuff[0] == '#')
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }
        // ifconfig eth0 hw ether 00:16:17:30:A1:68
        if( strstr(szBuff, "ifconfig eth0 hw ether") != NULL)
        {
            sprintf(szTmpBuff, "ifconfig eth0 hw ether %s\n", szMAC);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
        else
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }

    }
    fclose(fp);
    fclose(tmp_fp);

    // 覆盖原文件
    pThis->MoveFile(szTmpFile, szFile);
    // 写入分区保存
    _DEBUG_("Set MAC: [%s]", szMAC);
    lRet =  pThis->WriteCfgFileToCfgMtd();
    if( lRet == 0 )
    {
        // 使修改生效
        char szCmd[256] = {0};
        sprintf(szCmd, "chmod a+x %s && %s", szFile, szFile);
        system(szCmd);
        return 0;
    }
    return -1;
    
}
// 获取设备型号
long CDecoderSetting::GetDeviceModel(int& iModel, void* param)
{

   // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}
    iModel = pro.GetProperty((char*)"SYS_INFO", (char*)"Model", 0);
 
    return 0;
}
long CDecoderSetting::SetDeviceModel(int iModel, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    int iCurModel = 0;
    long lRet = GetDeviceModel(iCurModel, param);
    if(lRet==0 && iCurModel==iModel)
    {
        return -1;
    }
    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}
    pro.SetProperty((char*)"SYS_INFO", (char*)"Model", iModel);
    _DEBUG_("set model = %d", iModel);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_SYS);
    // 写入分区保存
    pThis->WriteCfgFileToCfgMtd();
    return 0;

}

// 获取设备序列号
long CDecoderSetting::GetDeviceSN(char* szSN, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    
    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}

    pro.GetProperty((char*)"SYS_INFO", (char*)"SerialNo", szSN, 64, (char*)"0");
    return 0;

}

// 设置设备序列号
long CDecoderSetting::SetDeviceSN(const char* szSN, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    
    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}
    char szTmp[64] = {0};
    sprintf(szTmp, "%s", szSN);
    pro.SetProperty((char*)"SYS_INFO", (char*)"SerialNo", szTmp);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_SYS);
    // 写入分区保存
    pThis->WriteCfgFileToCfgMtd();
    return 0;

}

// 获取设备主控软件版本号
long CDecoderSetting::GetDeviceVer(char* szVer, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    
#ifdef VERSION
    sprintf(szVer, "%s", VERSION);
    return 0;
#endif

    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}
    pro.GetProperty((char*)"SYS_INFO", (char*)"MasterVer", szVer, 64, (char*)"0");
    return 0;

}
long CDecoderSetting::GetDevBuildtime(char* szTime, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    
#ifdef BUILDDATE
    sprintf(szTime, "%s", BUILDDATE);
    return 0;
#endif
    
    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_SYS) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
		return -1;
	}
    // 获取
    pro.GetProperty((char*)"SYS_INFO", (char*)"BuildTime", szTime, 64, (char*)"0");
    return 0;

}


long CDecoderSetting::GetNetworkInfo(struNetworkInfo* pNI, void* param)
{
    FILE* fp = fopen(CFG_FILE_NETWORK, "r");
    if(fp == NULL)
    {
        _DEBUG_("open file\"%s\" failed!\n", CFG_FILE_NETWORK);
        return -1;
    }
    char szBuf[1024] = {0};
    while( 1 )
    {
        memset(szBuf, 0, 1024);
        if( fgets(szBuf, 1024, fp) == NULL) 
            break;
        // 注释行、空行
        if( szBuf[0] == '#' || szBuf[0] == '\0')
            continue;
        
        // ifconfig eth0 192.168.1.127 netmask 255.255.255.0
        if( strstr(szBuf, "ifconfig eth0") != NULL &&
            strstr(szBuf, "netmask") != NULL ) 
        {
            sscanf(szBuf, "ifconfig eth0 %s netmask %s",pNI->ip, pNI->netmask);
        }
        // route add default gw 192.168.1.1
        else if( strstr(szBuf, "route add default gw") != NULL )
        {
            sscanf(szBuf, "route add default gw %s",pNI->gateway);
        }
        else
        {
            continue;
        }
    }
    fclose(fp);
    return 0;
}
long CDecoderSetting::SetNetIp(char* ip, char* netmask)
{
    char *szFile = (char*)CFG_FILE_NETWORK;
    char *szTmpFile = (char*)"/tmp/tmp_netip.sh";
    FILE *fp, *tmp_fp;
    char szBuff[400] = {0};
    char szTmpBuff[400] = {0};
    long lRet = 0;
    
    fp = fopen(szFile, "r");
    if( fp == NULL)
    {
        return -1;
    }

    tmp_fp = fopen(szTmpFile, "w+b");
    if( tmp_fp == NULL )
    {
        fclose(fp);
        return -1;
    }

    while( 1 )
    {
        memset(szBuff, 0, 400);
        memset(szTmpBuff, 0, 400);
        if( fgets(szBuff, 400, fp) == NULL) 
            break;
		//printf("%s\n", szBuff );
        // 注释行
        if( szBuff[0] == '#')
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }
        // ifconfig eth0 192.168.1.127 netmask 255.255.255.0
        if( strstr(szBuff, "ifconfig eth0") != NULL &&
            strstr(szBuff, "netmask") != NULL ) 
        {
            sprintf(szTmpBuff, "ifconfig eth0 %s netmask %s\n", ip, netmask);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
        else
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }

    }
    fclose(fp);
    fclose(tmp_fp);

    // 覆盖原文件
    MoveFile(szTmpFile, szFile);
    // 写入分区保存
    lRet =  WriteCfgFileToCfgMtd();
    if( lRet == 0 )
    {
        // 使修改生效
        char szCmd[256] = {0};
        sprintf(szCmd, "chmod a+x %s && %s", szFile, szFile);
        system(szCmd);
        return 0;
    }
    return 0;

}
long CDecoderSetting::GetGateway(char* gw)
{
    struNetworkInfo tmpNetInfo;
    if(GetNetworkInfo(&tmpNetInfo, (void *)this) != 0)
    {
        return -1;
    }
    sprintf(gw, "%s", tmpNetInfo.gateway);
    return 0;
}

long CDecoderSetting::SetGateway(char* gw)
{
    char *szFile = (char*)CFG_FILE_NETWORK;
    char *szTmpFile = (char*)"/tmp/tmp_netgw.sh";
    FILE *fp, *tmp_fp;
    char szBuff[400] = {0};
    char szTmpBuff[400] = {0};
    char szMulticast[80]={"route add -net 224.0.0.0 netmask 224.0.0.0 dev eth0"};
    long lRet = 0;
    
    fp = fopen(szFile, "r");
    if( fp == NULL )
    {
        return -1;
    }

    tmp_fp = fopen(szTmpFile, "w+b");
    if( tmp_fp == NULL )
    {
        fclose(fp);
        return -1;
    }

    while( 1 )
    {
        memset(szBuff, 0, 400);
        memset(szTmpBuff, 0, 400);
        if( fgets(szBuff, 400, fp) == NULL) 
            break;
		//printf("%s\n", szBuff );
        // 注释行
        if( szBuff[0] == '#')
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }
        // ifconfig eth0 192.168.1.127 netmask 255.255.255.0
        if( strstr(szBuff, "route add default gw") != NULL )
        {
            sprintf(szTmpBuff, "route add default gw %s metric 10\nroute\n", gw);
            //写入组播路由
            fwrite(szMulticast, 1, strlen(szMulticast), tmp_fp);
            fputc('\n', tmp_fp);// 写入换行符
            //写入路由
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
            //写入ping网关，解决网络不通
            memset(szTmpBuff, 0, 1024);
            sprintf(szTmpBuff, "ping %s -c 3 -w 3 &\n", gw);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
        else if( strstr(szBuff, szMulticast) != NULL )
        {
            //不写入，因为在写入网关时已写入。防止重复写入 
            continue;
        }
        else if( strstr(szBuff, "ping ") != NULL )
        {
            //不写入，因为在写入网关时已写入。防止重复写入 
            continue;
        }
        else
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }

    }
    fclose(fp);
    fclose(tmp_fp);

    // 覆盖原文件
    MoveFile(szTmpFile, szFile);
    // 写入分区保存
    lRet =  WriteCfgFileToCfgMtd();
    if( lRet == 0 )
    {
        // 使修改生效
        char szCmd[256] = {0};
        sprintf(szCmd, "chmod a+x %s && %s", szFile, szFile);
        system(szCmd);
        return 0;
    }
    return 0;

}

long CDecoderSetting::SetNetworkInfo(const struNetworkInfo* pNI, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    char *szFile = (char*)CFG_FILE_NETWORK;
    char *szTmpFile = (char*)"/tmp/tmp_network.sh";
    FILE *fp, *tmp_fp;
    char szBuff[1024] = {0};
    char szTmpBuff[1024] = {0};
    char szMulticast[80]={"route add -net 224.0.0.0 netmask 224.0.0.0 dev eth0"};
    long lRet = 0;
    
    fp = fopen(szFile, "r");
    if( fp == NULL )
    {
        return -1;
    }

    tmp_fp = fopen(szTmpFile, "w+b");
    if( tmp_fp == NULL )
    {
        fclose(fp);
        return -1;
    }

    while( 1 )
    {
        memset(szBuff, 0, 1024);
        memset(szTmpBuff, 0, 1024);
        if( fgets(szBuff, 1024, fp) == NULL) 
            break;
		printf("%s\n", szBuff );
        // 注释行
        if( szBuff[0] == '#')
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }
        // ifconfig eth0 192.168.1.127 netmask 255.255.255.0
        if( strstr(szBuff, "ifconfig eth0") != NULL &&
            strstr(szBuff, "netmask") != NULL ) 
        {
            sprintf(szTmpBuff, "ifconfig eth0 %s netmask %s\n", pNI->ip, pNI->netmask);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
		else if ( strstr(szBuff, "hw ether") != NULL )
		{
			continue;
		}
		else if ( strstr(szBuff, "eth0 down") != NULL )
		{
			continue;
		}
		else if ( strstr(szBuff, "eth0 up") != NULL )
		{
			continue;
		}
        // route add default gw 192.168.1.1
        else if( strstr(szBuff, "route add default gw") != NULL )
        {
            sprintf(szTmpBuff, "route add default gw %s metric 10\nroute\n", pNI->gateway);
            //写入组播路由
            fwrite(szMulticast, 1, strlen(szMulticast), tmp_fp);
            fputc('\n', tmp_fp);// 写入换行符
            //写入路由
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
            //写入ping网关，解决网络不通
            memset(szTmpBuff, 0, 1024);
            sprintf(szTmpBuff, "ping %s -c 3 -w 3 &\n", pNI->gateway);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
        else if( strstr(szBuff, szMulticast) != NULL )
        {
            //不写入，因为在写入网关时已写入。防止重复写入 
            continue;
        }
        else if( strstr(szBuff, "ping ") != NULL )
        {
            //不写入，因为在写入网关时已写入。防止重复写入 
            continue;
        }
        else
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }

    }
    fclose(fp);
    fclose(tmp_fp);

    // 覆盖原文件
    pThis->MoveFile(szTmpFile, szFile);
    // 写入分区保存
    lRet =  pThis->WriteCfgFileToCfgMtd();
    if( lRet == 0 )
    {
        // 使修改生效
        char szCmd[256] = {0};
        sprintf(szCmd, "chmod a+x %s && %s", szFile, szFile);
        system(szCmd);
        return 0;
    }
    return 0;
}

long CDecoderSetting::GetSipServer(struSipServer* pSS, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 获取SIP服务器信息
    if(pSS == NULL)
    {
        return -1;
    }
    pro.GetProperty((char*)"SIP_SERVER", (char*)"IP", pSS->ip, 32, (char*)"");
    pro.GetProperty((char*)"SIP_SERVER", (char*)"ID", pSS->id, 100, (char*)"");
    return 0;
}
long CDecoderSetting::SetSipServer(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    //_DEBUG_("[%d]", chnno);
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 解析SIP服务器信息
    struSipServer sSS;
    memset(&sSS, 0, sizeof(struSipServer));
    char* szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[256]={0};
    strcpy(szTmpBuf, szBuf);

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(sSS.ip, "%s", szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(sSS.id, "%s", szPtr);

    pro.SetProperty((char*)"SIP_SERVER", (char*)"IP", sSS.ip);
    pro.SetProperty((char*)"SIP_SERVER", (char*)"ID", sSS.id);

    // 再次加载以保存
    pro.load((char*)CFG_FILE_SYS);
    pThis->WriteCfgFileToCfgMtd();
    return 0;

}

long CDecoderSetting::UpgradeDevice(const char* szData, unsigned long ulLen, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    // 写入文件
    char *szUpdateFile = (char*)"/tmp/update_file";
    FILE* fp = fopen(szUpdateFile, "w+b");
    if( fp==NULL )
    {
        _DEBUG_("fp==NULL");
        return -1;
    }

    fwrite((const void*)szData, 1, ulLen, fp);
    fclose(fp);

    long lRet = pThis->m_pFlashRW->WriteMtd(szUpdateFile);
    remove(szUpdateFile);
    return lRet;
}

long CDecoderSetting::GetDecChannelInfo(struDecChannelInfo* pDCI, void* param)
{
    CProfile pro;
    char szTarget[64] = {0};
    int iDecChannelCount = 0;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取通道个数
    //iDecChannelCount = pro.GetProperty("COUNT", "DecChannelCount", 0);
    GetChannelCount(CHN_DECODER, iDecChannelCount, param);
    if(iDecChannelCount <= 0)
    {
    	_DEBUG_("dec channel count invalid:%d", iDecChannelCount );
        return -1;
    }
    sprintf(szTarget, "CHANNEL_INFO_%d", pDCI->channel);
    pro.GetProperty(szTarget, (char*)"Code", pDCI->code, 80, (char*)"");
	
    _DEBUG_("Get Channel Info: chn_no=%d code=%s", pDCI->channel, pDCI->code);

    return 0;
}
long CDecoderSetting::SetDecChannelInfo(const struDecChannelInfo* pDCI, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("data==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    CProfile pro;
    char szTarget[64] = {0};
    char szCode[80] = {0};
    int iDecChannelCount = 0;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取通道个数
    //iDecChannelCount = pro.GetProperty("COUNT", "DecChannelCount", 0);
    GetChannelCount(CHN_DECODER, iDecChannelCount, param);
    if(iDecChannelCount <= 0)
    {
        _DEBUG_("GetChannelCount error[count=%d]", iDecChannelCount);
        return -1;
    }

    sprintf(szTarget, "CHANNEL_INFO_%d", pDCI->channel);
    strncpy(szCode, pDCI->code, 80*sizeof(char));
    pro.SetProperty(szTarget, (char*)"Code", szCode);
    _DEBUG_("Set Chanel Info: chn_no=[%d] code=[%s]", pDCI->channel, pDCI->code);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_DECODER);

    // 仅写入内存文件，不再写入分区
    //pThis->WriteCfgFileToCfgMtd5();

    return 0;
}

long CDecoderSetting::GetResolution(struDisplayResolution* pDR, void* param)
{
	//#define VO_INTF_CVBS    (0x01L<<0)
	//#define VO_INTF_YPBPR   (0x01L<<1)
	//#define VO_INTF_VGA     (0x01L<<2)
	//#define VO_INTF_BT656   (0x01L<<3)
	//#define VO_INTF_BT1120  (0x01L<<4)
	//#define VO_INTF_HDMI    (0x01L<<5)

    int iDisplaymode = pDR->displaymode;
    int iDisplayno   = pDR->displayno;
    char szTarget[32] = {0};
    int iDisplaymodeCount = 0;
    int iDisplayChnCount = 0;
    // 加载配置文件
    CProfile pro;
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取显示个数、通道个数
    //iDisplaymodeCount = pro.GetProperty("COUNT", "DisplaymodeCount", 0);
    GetChannelCount(CHN_DISPMODE, iDisplaymodeCount, param);
    if(iDisplaymode <=0 || iDisplaymode>iDisplaymodeCount)
    {
    	printf( "idisplaymode error:%d\n", iDisplaymode );
        return -1;
    }
    switch(iDisplaymode)
    {
    case 1:// VGA
        //iDisplayChnCount = pro.GetProperty("COUNT", "VgaChannelCount", 0);
        GetChannelCount(CHN_DISPVGA, iDisplayChnCount, param);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "VGA_DISPLAY_%d", iDisplayno);
        }
        else
        {
        	printf( "idisplay channnel error:%d\n", iDisplayno );
            return -1;
        }
        break;
    case 2:// BNC
        //iDisplayChnCount = pro.GetProperty("COUNT", "BncChannelCount", 0);
        GetChannelCount(CHN_DISPBNC, iDisplayChnCount, param);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "BNC_DISPLAY_%d", iDisplayno);
        }
        else
        {
        	printf( "idisplay channnel error:%d\n", iDisplayno );
            return -1;
        }

        break;
    case 3:// HDMI
        //iDisplayChnCount = pro.GetProperty("COUNT", "HdmiChannelCount", 0);
        GetChannelCount(CHN_DISPHDMI, iDisplayChnCount, param);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "HDMI_DISPLAY_%d", iDisplayno);
        }
        else
        {
        	printf( "idisplay channnel error:%d\n", iDisplayno );
            return -1;
        }

        break;
    default:
        _DEBUG_("Invalid display mode[%d]", iDisplaymode);
        return -1;
    };


    //获取分辨率信息
	pDR->enabled = pro.GetProperty(szTarget, (char*)"Enabled", 0);
	pDR->resolution = pro.GetProperty(szTarget, (char*)"Resolution", 0);
    // 通道信息
    pDR->channel = pro.GetProperty(szTarget, (char*)"BindChn", 0);
    
    return 0;
}

long CDecoderSetting::SetResolution(const struDisplayResolution* pDR, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("data==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    long lRet = 0;
    struDisplayResolution sDR;
    memcpy(&sDR, pDR, sizeof(struDisplayResolution));
    if(pDR->displaymode==0) // 设置所有的显示接口
    {
        //1-VGA, 2-BNC,3-HDMI
        for(int i=1; i<=3;i++)
        {
            sDR.displaymode = i;
            sDR.resolution = 0;
            pThis->SetResolution(&sDR);
        }
    }
    else
    {
        pThis->SetResolution(&sDR);
    }

    // 分辨率修改，使生效
	if ( pThis->m_cbSetResolution != NULL )
	{
		pThis->m_cbSetResolution( pDR, pThis->m_pDDUCaller );
	}
    //pThis->m_bWriteMtdNow = true;
    lRet = pThis->WriteCfgFileToCfgMtd5();
    return lRet;
}

long CDecoderSetting::SetResolution(const struDisplayResolution* pDR)
{
    if( pDR==NULL )
    {
        _DEBUG_("pDR==NULL!");
        return -1;
    }
    long lRet = 0;
    int iResToSet = 0;

    // 写入配置文件
    CProfile pro;
    int iDisplaymode = pDR->displaymode;
    int iDisplayno   = pDR->displayno;
    char szTarget[32] = {0};
    int iDisplaymodeCount = 0;
    int iDisplayChnCount = 0;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取显示个数、通道个数
    iDisplaymodeCount = pro.GetProperty((char*)"COUNT", (char*)"DisplaymodeCount", 0);
    if(iDisplaymode <=0 || iDisplaymode>iDisplaymodeCount)
    {
        return -1;
    }
    switch(iDisplaymode)
    {
    case 1:// VGA
        iDisplayChnCount = pro.GetProperty((char*)"COUNT", (char*)"VgaChannelCount", 0);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "VGA_DISPLAY_%d", iDisplayno);
        }
        else
        {
            return -1;
        }
        break;
    case 2:// BNC
        iDisplayChnCount = pro.GetProperty((char*)"COUNT", (char*)"BncChannelCount", 0);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "BNC_DISPLAY_%d", iDisplayno);
        }
        else
        {
            return -1;
        }

        break;
    case 3:// HDMI
        iDisplayChnCount = pro.GetProperty((char*)"COUNT", (char*)"HdmiChannelCount", 0);
        if(iDisplayno>0 && iDisplayno<=iDisplayChnCount)
        {
            sprintf(szTarget, "HDMI_DISPLAY_%d", iDisplayno);
        }
        else
        {
            return -1;
        }

        break;
    default:
        _DEBUG_("Invalid display mode[%d]", iDisplaymode);
        return -1;
    };

	pro.SetProperty(szTarget, (char*)"Enabled", pDR->enabled);
    if(pDR->resolution != 0)
    {
	    pro.SetProperty(szTarget, (char*)"Resolution", pDR->resolution);
    }
    pro.SetProperty(szTarget, (char*)"BindChn", pDR->channel);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_DECODER);
    _DEBUG_("chn: [%d]", iDisplaymode);

    return lRet;
}

//获取、设置OSD信息
long CDecoderSetting::GetDecOsdInfo(struDecOsdInfo* pDecOsdInfo, void *param)
{
    CProfile pro;
    //char szTarget[64] = {0};
    
    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    
    //pro.GetProperty("DEC_OSD", (char*)"Text", pDecOsdInfo->text, 64, (char*)"");
    //pDecOsdInfo->text[63]='\0';
    sprintf(pDecOsdInfo->text, "%s" , (char*)" ");

	pDecOsdInfo->show= pro.GetProperty((char*)"DEC_OSD", (char*)"Show", 0);
	
	
    //_DEBUG_("Get OSD: [%s] show=%d", pDecOsdInfo->text, pDecOsdInfo->show);

    return 0;
}
long CDecoderSetting::SetDecOsdInfo(struDecOsdInfo* pDecOsdInfo, void *param)
{
    // 不作处理
    return 0;
    /////////////////////////
    CProfile pro;
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
	char szText[64] = {0};
	memcpy(szText, pDecOsdInfo->text, 64*sizeof(char));
	szText[63]='\0';

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	} 
	if(pDecOsdInfo->show == 0)
    	pro.SetProperty((char*)"DEC_OSD", (char*)"Text", (char*)"");
    else
        pro.SetProperty((char*)"DEC_OSD", (char*)"Text", szText);
	pro.SetProperty((char*)"DEC_OSD", (char*)"Show", pDecOsdInfo->show);
    _DEBUG_("Set OSD: [%s] show=%d", szText, pDecOsdInfo->show);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_DECODER);

    pThis->WriteCfgFileToCfgMtd5();
    return 0;
}
long CDecoderSetting::GetDecOsdInfoWeb(struDecOsdInfo* pDecOsdInfo, void *param)
{
    CProfile pro;
    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    memset(pDecOsdInfo->text, 0, 64);
    pDecOsdInfo->show = pro.GetProperty((char*)"DEC_OSD_WEB", (char*)"Show", 0);

	_DEBUG_("###GET DEC OSD SHOW WEB [%d]!", pDecOsdInfo->show);
	
    return 0;
}
long CDecoderSetting::SetDecOsdInfoWeb(struDecOsdInfo* pDecOsdInfo, void *param)
{
    CProfile pro;
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
	pro.SetProperty((char*)"DEC_OSD_WEB", (char*)"Show", pDecOsdInfo->show);
    _DEBUG_("Set OSD WEB: show=%d", pDecOsdInfo->show);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_DECODER);

    if(pThis->m_cbSetOsdShow != NULL)
    {
        pThis->m_cbSetOsdShow(pDecOsdInfo, pThis->m_pDDUCaller);
    }
    //pThis->m_bWriteMtdNow = true;
	pThis->WriteCfgFileToCfgMtd5();

    return 0;
}


// 获取、设置动态解码URL
long CDecoderSetting::GetDynamicDecUrl(struDynamicDecUrl* pDDU, void* param)
{
    CProfile pro;
    char szTarget[64] = {0};
    int iDecChannelCount = 0;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取通道个数
    //iDecChannelCount = pro.GetProperty("COUNT", "DecChannelCount", 0);
    GetChannelCount(CHN_DECODER, iDecChannelCount, param);
    if(iDecChannelCount <= 0)
    {
    	_DEBUG_("dec channel count invalid:%d", iDecChannelCount );
        return -1;
    }
    sprintf(szTarget, "CHANNEL_%d", pDDU->channel);
    pro.GetProperty(szTarget, (char*)"Url", pDDU->url, 256, (char*)"");


	char szFPS[64]={0};
	pro.GetProperty(szTarget, (char*)"Fps", szFPS, 64, (char*)"30");
	pDDU->fps = atoi(szFPS);
	
	
    _DEBUG_("Get URL: %d [%s] %d", pDDU->channel, pDDU->url, pDDU->fps);

    return 0;
}

long CDecoderSetting::SetDynamicDecUrl(const struDynamicDecUrl* pDDU, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    CProfile pro;
    char szTarget[64] = {0};
    char szUrl[MAX_URL_LEN] = {0};
	char szFps[32] = {0};
    int iDecChannelCount = 0;

    // 加载配置文件
	if (pro.load((char*)CFG_FILE_DECODER) < 0)
	{
		_DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_DECODER);
		return -1;
	}
    // 获取通道个数
    //iDecChannelCount = pro.GetProperty("COUNT", "DecChannelCount", 0);
    GetChannelCount(CHN_DECODER, iDecChannelCount, param);
    if(iDecChannelCount <= 0)
    {
        return -1;
    }

    sprintf(szTarget, "CHANNEL_%d", pDDU->channel);
    strncpy(szUrl, pDDU->url, sizeof(pDDU->url));
	sprintf(szFps, "%d", pDDU->fps );
    pro.SetProperty(szTarget, (char*)"Url", szUrl);
	pro.SetProperty( szTarget, (char*)"Fps", szFps );
    _DEBUG_("Set URL: %d [%s]", pDDU->channel, pDDU->url);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_DECODER);
    if( pThis->m_cbSetDDU != NULL )
    {
        pThis->m_cbSetDDU(pDDU, pThis->m_pDDUCaller);
    }

    // 仅写入内存文件，不再写入分区
    //pThis->WriteCfgFileToCfgMtd5();

    return 0;
}
void CDecoderSetting::SetCB_DDU( SetDynamicDecUrlCB_T callback, void *pCBObj )
{
	m_cbSetDDU = callback;
	m_pDDUCaller = pCBObj;
}
void CDecoderSetting::SetCB_OsdShow( DecOsdShowCB_T callback, void *pCBObj )
{
    m_cbSetOsdShow= callback;
	m_pDDUCaller = pCBObj;
}

void CDecoderSetting::SetCB_Resolution( SetResolutionCB_T callback, void *pCBObj )
{
	m_cbSetResolution = callback;
	m_pDDUCaller = pCBObj;
}

long CDecoderSetting::ResetParam(int iType, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    long lRet = 0;
    char *szCfgFile = (char*)"/tmp/tmp_toreset_config_mtd_file";

    // 将默认配置的网络信息拷贝到/root目录下，用于获取最后的IP地址
    pThis->CopyFile(CFG_FILE_TORESET_NETWORK, CFG_FILE_NETWORK);
	//mtd2

    // 组成config分区文件
    lRet = pThis->m_pConfigMtdFile->JoinConfigFiles(szCfgFile, iType);
    if(lRet != 0)
    {
        return lRet;
    }

    // 写入config分区
    lRet = (long)(pThis->m_pFlashRW->ImportConfigToMtd(szCfgFile));
	//mtd5
    lRet = pThis->m_pConfigMtdFile->JoinConfigFilesToMtd5(szCfgFile, iType);
    if(lRet != 0)
    {
        return lRet;
    }
	lRet = (long)(pThis->m_pFlashRW->ImportConfigToMtd5(szCfgFile));

	
    return 0;
}

long CDecoderSetting::GetHostname(char* szHostname, void* param)
{
    FILE* fp = fopen(CFG_FILE_HOSTNAME, "r");
    if(fp == NULL)
    {
        _DEBUG_("open file\"%s\" failed!", CFG_FILE_HOSTNAME);
        return -1;
    }
    char szBuf[256] = {0};
    while( 1 )
    {
        memset(szBuf, 0, sizeof(szBuf));
        if( fgets(szBuf, sizeof(szBuf), fp) == NULL) 
            break;
        // 注释行、空行
        if( szBuf[0] == '#' || szBuf[0] == '\0')
            continue;
        
        if( strstr(szBuf, "hostname ") != NULL) 
        {
            sscanf(szBuf, "hostname %s", szHostname);
        }
        else
        {
            continue;
        }
    }
    fclose(fp);
    return 0;

}

long CDecoderSetting::SetHostname(const char* szHostname, void* param)
{
    if( param==NULL || szHostname==NULL)
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;

    char *szFile = (char*)CFG_FILE_HOSTNAME;
    char *szTmpFile = (char*)"/tmp/tmp_hostname.sh";
    FILE *fp, *tmp_fp;
    char szBuff[1024] = {0};
    char szTmpBuff[1024] = {0};
    long lRet = 0;
    
    fp = fopen(szFile, "r");
    tmp_fp = fopen(szTmpFile, "w+b");
    if( fp == NULL || tmp_fp == NULL )
    {
        fclose(fp);
        fclose(tmp_fp);
        return -1;
    }

    while( 1 )
    {
        memset(szBuff, 0, 1024);
        memset(szTmpBuff, 0, 1024);
        if( fgets(szBuff, 1024, fp) == NULL) 
            break;
        // 注释行
        if( szBuff[0] == '#')
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }
        // hostname MD3000
        if( strstr(szBuff, "hostname ") != NULL) 
        {
            sprintf(szTmpBuff, "hostname %s\n", szHostname);
            fwrite(szTmpBuff, 1, strlen(szTmpBuff), tmp_fp);
        }
        else
        {
            fwrite(szBuff, 1, strlen(szBuff), tmp_fp);
            continue;
        }

    }
    fclose(fp);
    fclose(tmp_fp);

    // 覆盖原文件
    pThis->MoveFile(szTmpFile, szFile);
    // 写入分区保存
    lRet =  pThis->WriteCfgFileToCfgMtd();
    if( lRet == 0 )
    {
        // 使修改生效
        char szCmd[256] = {0};
        sprintf(szCmd, "chmod a+x %s && %s", szFile, szFile);
        system(szCmd);
        return 0;
    }
    return 0;
}

long CDecoderSetting::GetSysTime(char* szTime, void* param)
{
    if( param==NULL || szTime==NULL)
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;

    int iTime = (int)(  CRtc::read_rtc() );
    sprintf(szTime, "%s", CFacility::FormatTime(iTime));
    _DEBUG_("Get Systime Time: %s", szTime);

    return 0;
}


long CDecoderSetting::SetSysTime(const char* szTime, void* param)
{
    if( param==NULL || szTime==NULL)
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    //"2015-02-10 18:30:59"
    _DEBUG_("Set Systime Time: %s", szTime);
    time_t time = (time_t)( CFacility::FormatTime(szTime) );
    CRtc::write_rtc(time);
    // 更新系统时间
    char szCmd[100] = {0};
    sprintf(szCmd, "date -s \"%s\"", szTime);
    system(szCmd);
    return 0;
}
long CDecoderSetting::GetNtp(char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 获取NTP信息
    struNtpInfo ntp;
    memset(&ntp, 0, sizeof(struNtpInfo));

    ntp.iEnable = pro.GetProperty((char*)"NTP_SERVER", (char*)"ENABLE", 0);
    pro.GetProperty((char*)"NTP_SERVER", (char*)"SERVERIP", ntp.ip, 32, (char*)"");
    ntp.iInterval = pro.GetProperty((char*)"NTP_SERVER", (char*)"INTERVAL", 0);

    // "1;192.168.2.3;5;" 使能;NTP服务器IP;校时间隔
    sprintf(szBuf, "%d;%s;%d;", ntp.iEnable, ntp.ip, ntp.iInterval);
    //_DEBUG_("%s", szBuf);
    return 0;
}


long CDecoderSetting::SetNtp(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    //_DEBUG_("[%s]", szBuf);
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 解析NTP信息
    struNtpInfo ntp;
    memset(&ntp, 0, sizeof(struNtpInfo));

    char* szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[256]={0};
    strcpy(szTmpBuf, szBuf);

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    ntp.iEnable = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(ntp.ip, "%s", szPtr);
    
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    ntp.iInterval = atoi(szPtr);
    if(ntp.iInterval < 1)//至少1分钟
    {
        ntp.iInterval = 1;
    }
    if(ntp.iInterval > 600)//最多10小时
    {
        ntp.iInterval = 600;
    }

    pro.SetProperty((char*)"NTP_SERVER", (char*)"ENABLE", ntp.iEnable);
    pro.SetProperty((char*)"NTP_SERVER", (char*)"SERVERIP", ntp.ip);
    pro.SetProperty((char*)"NTP_SERVER", (char*)"INTERVAL", ntp.iInterval);

    // 再次加载以保存
    pro.load((char*)CFG_FILE_SYS);
    long lRet = pThis->WriteCfgFileToCfgMtd();
    // 设置NTP信息
    pthread_mutex_lock(&pThis->m_ntpMutexLock);
    memcpy(&(pThis->m_objNtpInfo), &ntp, sizeof(struNtpInfo));
    pthread_mutex_unlock(&pThis->m_ntpMutexLock);
    return lRet;

}


long CDecoderSetting::GetAR(char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 获取NTP信息
    struARInfo ar;
    memset(&ar, 0, sizeof(struARInfo));

    ar.iEnable = pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_ENABLE", 0);
    ar.iType= pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_TYPE", 0);
    pro.GetProperty((char*)"AUTO_REBOOT", (char*)"AR_TIME", ar.szTime, 32, (char*)"02:00:00");

    // "1;0;02:00:00;" 使能;模式;时间
    sprintf(szBuf, "%d;%d;%s;", ar.iEnable, ar.iType, ar.szTime);
    //_DEBUG_("%s", szBuf);
    return 0;
}


long CDecoderSetting::SetAR(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CDecoderSetting * pThis = (CDecoderSetting*)param;
    //_DEBUG_("[%s]", szBuf);
    CProfile pro;
    if (pro.load((char*)CFG_FILE_SYS) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_SYS);
        return -1;
    }
    // 解析NTP信息
    struARInfo ar;
    memset(&ar, 0, sizeof(struARInfo));

    char* szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[256]={0};
    strcpy(szTmpBuf, szBuf);

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    ar.iEnable = atoi(szPtr);
    if(ar.iEnable <= 0) 
    {
        ar.iEnable = 0;
    }
    else if(ar.iEnable >= 1)
    {
        ar.iEnable = 1;
    }

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    ar.iType= atoi(szPtr);
    if(ar.iType <= 0) 
    {
        ar.iType =0;
    }
    else if(ar.iType > 7)
    {
        ar.iType =7;
    }

    
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(ar.szTime, "%s", szPtr);
    // 解析AR_TIME格式，如果不是"时:分:秒"，则给默认值"02:00:00"
    char* szPtr2 = NULL;
    char *szDelim2 = (char*)":";
    char szTmpBuf2[256]={0};
    strcpy(szTmpBuf2, ar.szTime);
    int iARHour = 2;
    int iARMin = 0;
    int iARSec = 0;

    szPtr2 = strtok(szTmpBuf2, szDelim2);
    if(szPtr2 !=  NULL)
    {
        iARHour = atoi(szPtr2);
        if(iARHour<0 || iARHour>23)
            iARHour = 2;
    }

    szPtr2 = strtok(NULL, szDelim2);
    if(szPtr2 !=  NULL)
    {
        iARMin = atoi(szPtr2);
        if(iARMin<0 || iARMin>59)
            iARMin = 0;
    }

    szPtr2 = strtok(NULL, szDelim2);
    if(szPtr2 !=  NULL)
    {
        iARSec = atoi(szPtr2);
        if(iARSec<0 || iARSec>59)
            iARSec = 0;
    }

    pro.SetProperty((char*)"AUTO_REBOOT", (char*)"AR_ENABLE", ar.iEnable);
    pro.SetProperty((char*)"AUTO_REBOOT", (char*)"AR_TYPE", ar.iType);
    sprintf(ar.szTime, "%02d:%02d:%02d", iARHour, iARMin, iARSec);
    pro.SetProperty((char*)"AUTO_REBOOT", (char*)"AR_TIME", ar.szTime);

    // 再次加载以保存
    pro.load((char*)CFG_FILE_SYS);
    long lRet = pThis->WriteCfgFileToCfgMtd();
    // 设置NTP信息
    pthread_mutex_lock(&pThis->m_arMutexLock);
    memcpy(&(pThis->m_objARInfo), &ar, sizeof(struARInfo));
    pthread_mutex_unlock(&pThis->m_arMutexLock);
    return lRet;

}


// 获取通道个数
long CDecoderSetting::GetChannelCount(int iMode, int &iCount, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    //CDecoderSetting * pThis = (CDecoderSetting*)param;
    iCount = 0;
    switch(iMode)
    {
        case CHN_DECODER:
            iCount = MAXDECCHANN;
            break;
        case CHN_ENCODER:
            iCount = 4;
            break;
        case CHN_ALARMIN:
            iCount = 4;
            break;
        case CHN_ALARMOUT:
            iCount = 1;
            break;
        case CHN_DISPMODE:
            iCount = 3;
            break;
        case CHN_DISPVGA:
            iCount = 1;
            break;
        case CHN_DISPBNC:
            iCount = 1;
            break;
        case CHN_DISPHDMI:
            iCount = 1;
            break;
        default:
            break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// private
long CDecoderSetting::CopyFile(const char* szSrcFile, const char* szDstFile)
{
    long lRet = -1;
    if(szSrcFile == NULL || szDstFile == NULL ){
        return lRet;
    }
    FILE *fp_src, *fp_dst;
    int iBufferLen = 4096;
    char szBuffer[iBufferLen];
    int iRead = 0;
    fp_src = fopen(szSrcFile, "r");
    if( fp_src == NULL){
        return lRet;
    }
    fp_dst = fopen(szDstFile, "w+b");
    if( fp_dst == NULL){
        return lRet;
    }
    while( true )
    {
        iRead = fread(szBuffer, 1, iBufferLen, fp_src);
        if( iRead > 0 ){
            fwrite(szBuffer, 1, iRead, fp_dst);
        }
        if( iRead < iBufferLen ){
            if( feof(fp_src) > 0 ){
                break;
            }
            else{
                // 发生错误
                fclose(fp_src);
                fclose(fp_dst);
                remove(szDstFile);
                return -1;
            }
        }
    }
    fclose(fp_src);
    fclose(fp_dst);
    
    return 0;
}
long CDecoderSetting::MoveFile(const char* szSrcFile, const char* szDstFile)
{
    long lRet = -1;
    lRet = CopyFile(szSrcFile, szDstFile);
    if( lRet != 0){
        return lRet;
    }
    remove(szSrcFile);
    return 0;
}

long CDecoderSetting::WriteCfgFileToCfgMtd5(char* szFile)
{

	pthread_mutex_lock(&m_mtdlock);
	m_bCfgFileChanged = true;
	//m_bWriteMtdNow = false;
	pthread_mutex_unlock(&m_mtdlock);
	return 0;
	//////////////////////////////////
/*
    long lRet = 0;
    char szCfgFile[256]={0};
    if( szFile == NULL )
    {
        sprintf(szCfgFile, "%s", "/tmp/tmp_config_mtd55_file");
    }
    else
    {
        if( szFile[0] != '\0' )
        {
            sprintf(szCfgFile, "%s", szFile);
        }
        else
        {
            return -1;
        }
    }

	CConfigMtdFile mtdfile;
	CFlashRW flashrw;
	
    // 组成config分区文件
    lRet = mtdfile.JoinConfigFilesToMtd5(szCfgFile);
    if(lRet != 0)
    {
        return lRet;
    }
    // 写入config分区
    _DEBUG_("Write to mtd5");
    lRet = (long)(flashrw.ImportConfigToMtd5(szCfgFile));
    // 移除临时文件
    remove(szCfgFile);
    return 0;
*/
}

long CDecoderSetting::WriteCfgFileToCfgMtd(char* szFile)
{
	/*pthread_mutex_lock(&m_mtdlock);
	m_bCfgFileChanged = true;
	m_bWriteMtdNow = true;
	pthread_mutex_unlock(&m_mtdlock);
	return 0;
	*/
	/////////////////////////////////////////

    long lRet = 0;
    char szCfgFile[256]={0};
    if( szFile == NULL )
    {
        sprintf(szCfgFile, "%s", "/tmp/tmp_config_mtd_file");;
    }
    else
    {
        if( szFile[0] != '\0' )
        {
            sprintf(szCfgFile, "%s", szFile);
        }
        else
        {
            return -1;
        }
    }

	CConfigMtdFile mtdfile;
	CFlashRW flashrw;
	
    // 组成config分区文件
    lRet = mtdfile.JoinConfigFiles(szCfgFile);
    if(lRet != 0)
    {
        return lRet;
    }
    // 写入config分区
    lRet = (long)(flashrw.ImportConfigToMtd(szCfgFile));
    // 移除临时文件
    remove(szCfgFile);
    return 0;

}

long CDecoderSetting::check_auth(const char* szUserName, const char* szPassword)
{
    if ( m_pUserInfo == NULL || m_iUserCount <= 0) {
        return -1;
    }
    
    for(int i=0; i<m_iUserCount; i++)
    {
        if (strcmp(m_pUserInfo[i].name, szUserName) == 0 &&
            strcmp(m_pUserInfo[i].pass, szPassword) == 0 &&
            m_pUserInfo[i].flag >=0 )
        {
            return m_pUserInfo[i].flag;
        }
    }
    
    return -1;
}

long CDecoderSetting::load_userinfo()
{
    if (m_pUserInfo != NULL) {
        delete [] m_pUserInfo;
        m_pUserInfo = NULL;
        m_iUserCount = 0;
    }

    CProfile pro;
    char szSection[32] = {0};

    // 加载配置文件
    if (pro.load((char*)CFG_FILE_USER) < 0)
    {
        printf("load file failed [FileName=%s]\n", CFG_FILE_USER);
        return -1;
    }
    m_iUserCount = pro.GetProperty((char*)"COUNT", (char*)"UserCount", 0);
    if (m_iUserCount<=0) {
        printf("user count = %d, error!\n", m_iUserCount);
        return -1;
    }
    m_pUserInfo = new struUserInfo[m_iUserCount];
    if ( m_pUserInfo == NULL )
    {
        return -1;
    }
    for( int i=0; i<m_iUserCount; i++)
    {
        sprintf(szSection, "USER_%d", (i+1));
        pro.GetProperty(szSection, (char*)"name", (char*)m_pUserInfo[i].name, 32, (char*)"0");
        pro.GetProperty(szSection, (char*)"pass", (char*)m_pUserInfo[i].pass, 32, (char*)"0");
        m_pUserInfo[i].flag = pro.GetProperty(szSection, (char*)"flag", -1);
    }

    return 0;
}





