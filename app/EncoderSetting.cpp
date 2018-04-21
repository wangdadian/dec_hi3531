#include "EncoderSetting.h"

CEncoderSetting::CEncoderSetting()
{
    m_pEWConfig = new CEWConfig;
    m_pFlashRW = new CFlashRW;
    m_pConfigMtdFile = new CConfigMtdFile;
    m_lConfigMtdLen = 0;

    ////////////////
    m_cbOnOsdChange = NULL;
    m_pobjOnOsdChange = NULL;
    m_cbOnNetChange = NULL;
    m_pobjOnNetChange = NULL;
    m_cbOnParamChange = NULL;
    m_pobjOnParamChange = NULL;
    m_cbOnChannelChange = NULL;
    m_pobjOnChannelChange = NULL;
    m_cbOnPtzChange = NULL;
    m_pobjOnPtzChange = NULL;
    m_cbOnPtzPortChange = NULL;
    m_pobjOnPtzPortChange = NULL;
    m_cbOnAlarminChange = NULL;
    m_pobjOnAlarminChange = NULL;
    m_cbOnAlarmoutChange = NULL;
    m_pobjOnAlarmoutChange = NULL;
}

CEncoderSetting::~CEncoderSetting()
{
    PTR_DELETE(m_pEWConfig);
    PTR_DELETE(m_pFlashRW);
    PTR_DELETE(m_pConfigMtdFile);
    m_lConfigMtdLen = 0;

    ///////////////////////////////////////////
    m_cbOnOsdChange = NULL;
    m_pobjOnOsdChange = NULL;
    m_cbOnNetChange = NULL;
    m_pobjOnNetChange = NULL;
    m_cbOnParamChange = NULL;
    m_pobjOnParamChange = NULL;
    m_cbOnChannelChange = NULL;
    m_pobjOnChannelChange = NULL;
    m_cbOnPtzChange = NULL;
    m_pobjOnPtzChange = NULL;
    m_cbOnPtzPortChange = NULL;
    m_pobjOnPtzPortChange = NULL;
    m_cbOnAlarminChange = NULL;
    m_pobjOnAlarminChange = NULL;
    m_cbOnAlarmoutChange = NULL;
    m_pobjOnAlarmoutChange = NULL;

}

long CEncoderSetting::Init()
{
    m_pEWConfig->SetCB_GetEncForwardport(GetEncForwardport, (void*)this);
    m_pEWConfig->SetCB_SetEncForwardport(SetEncForwardport, (void*)this);
    m_pEWConfig->SetCB_GetEncPtzPort(GetEncPtzPort, (void*)this);
    m_pEWConfig->SetCB_SetEncPtzPort(SetEncPtzPort, (void*)this);
    m_pEWConfig->SetCB_GetEncAlarmin(GetEncAlarmin, (void*)this);
    m_pEWConfig->SetCB_GetEncAlarmout(GetEncAlarmout, (void*)this);
    m_pEWConfig->SetCB_SetEncAlarm(SetEncAlarm, (void*)this);
    m_pEWConfig->SetCB_GetEncChannel(GetEncChannel, (void*)this);
    m_pEWConfig->SetCB_SetEncChannel(SetEncChannel, (void*)this);
    m_pEWConfig->SetCB_GetEncDisplay(GetEncDisplay, (void*)this);
    m_pEWConfig->SetCB_SetEncDisplay(SetEncDisplay, (void*)this);
    m_pEWConfig->SetCB_GetEncTransfer(GetEncTransfer, (void*)this);
    m_pEWConfig->SetCB_SetEncTransfer(SetEncTransfer, (void*)this);
    m_pEWConfig->SetCB_GetEncVideo(GetEncVideo, (void*)this);
    m_pEWConfig->SetCB_SetEncVideo(SetEncVideo, (void*)this);
    return 0;
}

// 通道号内存操作从0开始，网页、文件从1开始
// 获取配置文件中的数据信息
long CEncoderSetting::GetEncoderSettings(EncSettings *sEncs)
{
    if(sEncs == NULL)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    memset(sEncs, 0, sizeof(EncSettings));
    int i = 0;
    int iLoop = 0;
    /////////////////////////////////////////////
    // 通道个数
    /////////////////////////////////////////////
    sEncs->iChannelCount = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(sEncs->iChannelCount > int(MAXENCCHAN/2))
    {
        sEncs->iChannelCount = int(MAXENCCHAN/2);
    }
    /////////////////////////////////////////////
    // 所有通道从0开始，记入文件的是从1开始
    /////////////////////////////////////////////
    // ENC配置信息
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    //********** 所有通道内存中操作从0开始，记入文件、网页上操作的是从1开始
    /////////////////////////////////////////////
    iLoop = sEncs->iChannelCount * 2;
    int iChnno = 0;
    for(i=0; i<iLoop; i++)
    {
        // osd信息
        GetEncDisplay(&(sEncs->encset[i].osd), i);
        // net信息
        GetEncTransfer(&(sEncs->encset[i].net), i);
        // param信息、PTZ信息
        GetEncVideo(&(sEncs->encset[i].enc), &(sEncs->ptz[i/2]) , i);
        
        // name、device id、enable  
        sEncs->encset[i].iEnable = 1;
        if(i%2 == 0)
        {
            iChnno = i/2;
        }
        else
        {
            iChnno = (i+1)/2-1;
        }
        GetEncChannel(sEncs->encset[i].szName, sEncs->encset[i].szDeviceId, iChnno);
    }
    
    /////////////////////////////////////////////
    // PTZ端口配置信息
    /////////////////////////////////////////////
    GetEncPtzPort(&(sEncs->ptzport));
    
    /////////////////////////////////////////////
    // alarm配置信息
    /////////////////////////////////////////////
    // Alarm in
    //获取alarm in 通道个数
    int iAlarminCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarminCount", 0);
    if(iAlarminCnt > MAXALARMINCHN)
    {
        iAlarminCnt = MAXALARMINCHN;
    }
    for(i=0; i<iAlarminCnt; i++)
    {
      GetEncAlarmin(&(sEncs->alarm.alarmin[i]), i);
    }
    
    // Alarm out
    // 获取报警输出个数
    int iAlarmoutCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarmoutCount", 0);
    if(iAlarmoutCnt > MAXALARMOUTCHN)
    {
        iAlarmoutCnt = MAXALARMOUTCHN;
    }
    for(i=0; i<iAlarmoutCnt; i++)
    {
        GetEncAlarmout(&(sEncs->alarm.alarmout[i]), i);
    }

    return 0;
}
long CEncoderSetting::GetEncForwardport(ForwardportSetting* pObj)
{
    if(pObj == NULL)
    {
        return -1;
    }
    memset(pObj, 0, sizeof(ForwardportSetting));
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    pObj->iEnable = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Enable", 0);
    pObj->iType = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Type", 0);
    pObj->iPort = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Port", FORWARD_PORT);
    if(pObj->iPort<=0 || pObj->iPort>65535)
    {
        pObj->iPort = FORWARD_PORT;
    }
    return 0;
}
long CEncoderSetting::SetEncForwardport(ForwardportSetting* pObj)
{
    if(pObj == NULL)
    {
        return -1;
    }
    long lRet = 0;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    int port = pObj->iPort;
    if(port<=0 || port>65535)
    {
        port = FORWARD_PORT;
    }

    // 写入文件
    pro.SetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Enable", pObj->iEnable);
    pro.SetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Type", pObj->iType);
    pro.SetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Port", port);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);

    // 数据改变
    
    
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return lRet;
}

long CEncoderSetting::GetEncPtzPort(PtzPortSetting* pObj)
{
    if(pObj == NULL)
    {
        return -1;
    }
    memset(pObj, 0, sizeof(PtzPortSetting));
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    pObj->iBaude= pro.GetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_Baude", 0);
    pObj->iDataBits= pro.GetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_DataBits", 0);
    pObj->iParity= pro.GetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_Parity", 0);
    pObj->iStopBits= pro.GetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_StopBits", 0);

    return 0;
}
long CEncoderSetting::SetEncPtzPort(PtzPortSetting* pObj)
{
    if(pObj == NULL)
    {
        return -1;
    }
    long lRet = 0;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }

    // 写入文件
    pro.SetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_Baude", pObj->iBaude);
    pro.SetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_DataBits", pObj->iDataBits);
    pro.SetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_Parity", pObj->iParity);
    pro.SetProperty((char*)"ENC_PTZ_PORT", (char*)"ENC_StopBits", pObj->iStopBits);    
    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);

    // 数据改变
    OnPtzPortChange(0, (void*)pObj);
    
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();

    return lRet;
}
long CEncoderSetting::GetEncAlarmin(AlarmInSetting* pObj, int no)
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取报警输入个数
    int iAlarminCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarminCount", 0);
    if(iAlarminCnt<=0 || no>iAlarminCnt)
    {
        return -1;
    }

    char szSectionStr[80] = {0};
    sprintf(szSectionStr, "ENC_ALARMIN_%d", no+1);
    
    pObj->iEnable = pro.GetProperty(szSectionStr, (char*)"ENC_Enable", 0);
    pro.GetProperty(szSectionStr, (char*)"ENC_ID", pObj->szId, 100, (char*)"");
    pObj->iNormalStatus= pro.GetProperty(szSectionStr, (char*)"ENC_Type", 0);
    pObj->iStableTime = pro.GetProperty(szSectionStr, (char*)"ENC_Stabletime", 0);
    pObj->iTrigerAlarmOut = pro.GetProperty(szSectionStr, (char*)"ENC_Alarmout", 0);
    pObj->iTrigerAlarmOut-=1;
    pObj->iOutDelay = pro.GetProperty(szSectionStr, (char*)"ENC_Outdelay", 0);

    return 0;
}
long CEncoderSetting::GetEncAlarmout(AlarmOutSetting* pObj, int no)
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取报警输出个数
    int iAlarmoutCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarmoutCount", 0);
    if(iAlarmoutCnt <=0 || no >= iAlarmoutCnt)
    {
        return -1;
    }
    
    char szSectionStr[80] = {0};
    sprintf(szSectionStr, "ENC_ALARMOUT_%d", no+1);
    pObj->iEnable = pro.GetProperty(szSectionStr, (char*)"ENC_Enable", 0);
    pro.GetProperty(szSectionStr, (char*)"ENC_ID", pObj->szId, 100, (char*)"");

    return 0;
}
long CEncoderSetting::SetEncAlarm(AlarmInSetting* pObjIn, int inno, AlarmOutSetting* pObjOut, int outno)
{
    if(pObjIn == NULL || inno < 0)
    {
        return -1;
    }
    
    //_DEBUG_("[%s]", szBuf);
    long lRet = 0;
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取报警输入个数
    int iAlarminCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarminCount", 0);
    if(iAlarminCnt <= 0 || inno >= iAlarminCnt)
    {
        return -1;
    }
    // 获取报警输出个数
    int iAlarmoutCnt = pro.GetProperty((char*)"COUNT", (char*)"EncAlarmoutCount", 0);
    if(iAlarmoutCnt <= 0 || outno >= iAlarmoutCnt)
    {
        return -1;
    }

    char szAlarminSec[80] = {0};
    char szAlarmoutSec[80] = {0};
    sprintf(szAlarminSec, "ENC_ALARMIN_%d", inno+1);
    if( outno >= 0 )
    {
        sprintf(szAlarmoutSec, "ENC_ALARMOUT_%d", outno+1);
    }
    pro.SetProperty(szAlarminSec, (char*)"ENC_Enable", pObjIn->iEnable);
    pro.SetProperty(szAlarminSec, (char*)"ENC_ID", pObjIn->szId);
    pro.SetProperty(szAlarminSec, (char*)"ENC_Type", pObjIn->iNormalStatus);
    pro.SetProperty(szAlarminSec, (char*)"ENC_Stabletime", pObjIn->iStableTime);
    if(pObjIn->iTrigerAlarmOut >= 0)
    {
        pro.SetProperty(szAlarminSec, (char*)"ENC_Alarmout", pObjIn->iTrigerAlarmOut+1);
    }
    else
    {
        pro.SetProperty(szAlarminSec, (char*)"ENC_Alarmout", 0);
    }
    pro.SetProperty(szAlarminSec, (char*)"ENC_Outdelay", pObjIn->iOutDelay);
    if( outno >= 0 )
    {
        pro.SetProperty(szAlarmoutSec, (char*)"ENC_Enable", 1/*pObjOut->iEnable*/);
        pro.SetProperty(szAlarmoutSec, (char*)"ENC_ID", pObjOut->szId);
    }

    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);

    // 数据改变
    OnAlarminChange(inno, (void *)pObjIn);
    if( outno >= 0 )
    {
        OnAlarmoutChange(outno, (void *)pObjOut);
    }    
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return lRet;

}
long CEncoderSetting::GetEncChannel(char* szName, char* szId, int no)
{
    if(szName == NULL || szId == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0 || no >=iChnCnt)
    {
        return -1;
    }
    
    char szSectionStr[80] = {0};
    sprintf(szSectionStr, "ENC_CHANNEL_%d", no+1);
    
    pro.GetProperty(szSectionStr, (char*)"ENC_Name", szName, 256, (char*)"");
    pro.GetProperty(szSectionStr, (char*)"ENC_ID", szId, 256, (char*)"");

    return 0;
}
long CEncoderSetting::SetEncChannel(char* szName, char* szId, int no)
{
    if(szName == NULL || szId == NULL || no < 0)
    {
        return -1;
    }
    long lRet = 0;
    //_DEBUG_("[%s]", szBuf);
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0 )
    {
        return -1;
    }
    // 写入文件
    char szSectionStr[80] = {0};
    sprintf(szSectionStr, "ENC_CHANNEL_%d", no+1);
    pro.SetProperty(szSectionStr, (char*)"ENC_Name", szName);
    pro.SetProperty(szSectionStr, (char*)"ENC_ID", szId);
    
    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);

    //数据改变
    EncSetting sEnc;
    memset(&sEnc, 0, sizeof(EncSetting));
    sprintf(sEnc.szName, "%s", szName);
    sprintf(sEnc.szDeviceId, "%s", szId);
    OnChannelChange(no, (void *)&sEnc);
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return 0;
}
long CEncoderSetting::GetEncDisplay(EncOsdSettings* pObj, int no)
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0)
    {
        return -1;
    }
    
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 - 1;
        iStreamno = 2;
    }
    memset(pObj, 0, sizeof(EncOsdSettings));
    pObj->iChannelNo = no;
    char szOsdSettingSection[80]={0};
    char szOsdParamSection[80]={0};
    sprintf(szOsdSettingSection, "ENC_OSD_PARAM_%d_%d", iChnno+1, iStreamno);
    pro.GetProperty(szOsdSettingSection, (char*)"OSD_Id", pObj->szId, 256, (char*)"");
    pObj->iFont=pro.GetProperty(szOsdSettingSection, (char*)"OSD_Font", 0);
    for(int i=0; i<MAXOSD_COUNT; i++)
    {
        sprintf(szOsdParamSection, "ENC_OSD_PARAM_%d_%d_%d", iChnno+1, iStreamno, (i+1));
        pObj->osd[i].iEnable=pro.GetProperty(szOsdParamSection, (char*)"OSD_Enable", 0);
        pObj->osd[i].iX=pro.GetProperty(szOsdParamSection, (char*)"OSD_X", 0);
        pObj->osd[i].iY=pro.GetProperty(szOsdParamSection, (char*)"OSD_Y", 0);
        pObj->osd[i].iType=pro.GetProperty(szOsdParamSection, (char*)"OSD_Type", 0);
        pro.GetProperty(szOsdParamSection, (char*)"OSD_String", pObj->osd[i].szOsd, 256, (char*)"");
    }

    return 0;
}
long CEncoderSetting::SetEncDisplay(EncOsdSettings* pObj, int no)
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }

    long lRet = -1;
    CProfile pro;
    //_DEBUG_("%s", szBuf);
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return lRet;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0)
    {
        return -1;
    }
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 -1;
        iStreamno = 2;
    }

    // 写入配置文件
    char szOsdSettingSection[80]={0};
    char szOsdParamSection[80]={0};
    sprintf(szOsdSettingSection, "ENC_OSD_PARAM_%d_%d", iChnno+1, iStreamno);
    pro.SetProperty(szOsdSettingSection, (char*)"OSD_Id", pObj->szId);
    pro.SetProperty(szOsdSettingSection, (char*)"OSD_Font", pObj->iFont);
    pro.SetProperty(szOsdSettingSection, (char*)"OSD_MaxCount", MAXOSD_COUNT);
    for(int i=0; i<MAXOSD_COUNT; i++)
    {
        sprintf(szOsdParamSection, "ENC_OSD_PARAM_%d_%d_%d", iChnno+1, iStreamno, (i+1));
        pro.SetProperty(szOsdParamSection, (char*)"OSD_Enable", pObj->osd[i].iEnable);
        pro.SetProperty(szOsdParamSection, (char*)"OSD_X", pObj->osd[i].iX);
        pro.SetProperty(szOsdParamSection, (char*)"OSD_Y", pObj->osd[i].iY);
        pro.SetProperty(szOsdParamSection, (char*)"OSD_Type", pObj->osd[i].iType);
        pro.SetProperty(szOsdParamSection, (char*)"OSD_String", pObj->osd[i].szOsd);
    }    
    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);
    
    // 数据更改
    OnOsdChange(pObj->iChannelNo, (void *)pObj);
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return lRet;
}
long CEncoderSetting::GetEncTransfer(EncNetSettings* pObj, int no)
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <= 0)
    {
        return -1;
    }
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 -1;
        iStreamno = 2;
    }

    char szSectionStr[80] = {0};
    memset(pObj, 0 , sizeof(EncNetSettings));
    // 获取码流传输参数
    pObj->iChannelNo = no;
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        sprintf(szSectionStr, "ENC_NET_PARAM_%d_%d_%d", iChnno+1, iStreamno, (i+1));
        pObj->net[i].iEnable = pro.GetProperty(szSectionStr, (char*)"NET_Enable", 0);
        //pObj->net[i].iIndex= pro.GetProperty(szSectionStr, "NET_Index", 0);
        pObj->net[i].iIndex = i;
        pObj->net[i].iNetType= pro.GetProperty(szSectionStr, (char*)"NET_NetType", 0);
        pro.GetProperty(szSectionStr,(char*) "NET_PlayAddr", pObj->net[i].szPlayAddress, 256, (char*)"");
        pObj->net[i].iPlayPort= pro.GetProperty(szSectionStr, (char*)"NET_PlayPort", 0);
        pObj->net[i].iMux= pro.GetProperty(szSectionStr, (char*)"NET_Mux", 0);
    }

    return 0;
}
long CEncoderSetting::SetEncTransfer(EncNetSettings* pObj, int no, bool bCallback )
{
    if(pObj == NULL || no < 0)
    {
        return -1;
    }
    //和元数据比较，相同就不再写入
    bool bSame = false;
    EncNetSettings tmpEncSet;
    memset(&tmpEncSet, 0, sizeof(EncNetSettings));
    GetEncTransfer(&tmpEncSet, no);
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        if( pObj->net[i].iEnable==tmpEncSet.net[i].iEnable &&
            //pObj->net[i].iIndex==tmpEncSet.net[i].iIndex &&
            pObj->net[i].iNetType==tmpEncSet.net[i].iNetType &&
            pObj->net[i].iPlayPort==tmpEncSet.net[i].iPlayPort &&
            pObj->net[i].iMux==tmpEncSet.net[i].iMux &&
            (
            (strlen(pObj->net[i].szPlayAddress) < 7 && //(0.0.0.0)
            strlen(tmpEncSet.net[i].szPlayAddress) < 7) ||
            strcmp(pObj->net[i].szPlayAddress, tmpEncSet.net[i].szPlayAddress)==0
            )
        )
        {
            bSame = true;
        }
        else
        {
            bSame = false;
            break;
        }
    }
    if(bSame) 
    {
        _DEBUG_("Encoder Transfer Infos SAME, no needed to save!");
        return 0;
    }
    ///////////////////////////////////////////////////////////////////////////
    long lRet = -1;
    CProfile pro;
    //_DEBUG_("%s", szBuf);
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return lRet;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <= 0)
    {
        return -1;
    }
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 -1;
        iStreamno = 2;
    }

    // 写入配置文件
    char szSectionStr[80]={0};
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        sprintf(szSectionStr, "ENC_NET_PARAM_%d_%d_%d", iChnno+1, iStreamno, (i+1));
        pro.SetProperty(szSectionStr, (char*)"NET_Enable", pObj->net[i].iEnable);
        //pro.SetProperty(szSectionStr, "NET_Index", pObj->net[i].iIndex);
        pro.SetProperty(szSectionStr, (char*)"NET_Index", i+1);
        pro.SetProperty(szSectionStr, (char*)(char*)"NET_NetType", pObj->net[i].iNetType);
        pro.SetProperty(szSectionStr, (char*)"NET_PlayAddr", pObj->net[i].szPlayAddress);
        pro.SetProperty(szSectionStr, (char*)"NET_PlayPort", pObj->net[i].iPlayPort);
        pro.SetProperty(szSectionStr, (char*)"NET_Mux", pObj->net[i].iMux);
    }

    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);


	if (bCallback)
	{
    	// 数据更改
    	OnNetChange(pObj->iChannelNo, (void*)pObj);
	}
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return lRet;
}


long CEncoderSetting::GetEncVideo(EncoderParam* pObj, PtzSetting* ptz, int no)
{
    if(pObj == NULL || ptz == NULL || no < 0)
    {
        return -1;
    }
    CProfile pro;
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return -1;
    }
    
    // 取得通道号、码流号
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 -1;
        iStreamno = 2;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0)
    {
        return -1;
    }
    memset(pObj, 0, sizeof(EncoderParam));
    memset(ptz, 0, sizeof(PtzSetting));
    char szSectionStr[80] = {0};
    char szPtzSecStr[80] = {0};
    sprintf(szSectionStr, "ENC_PARAM_%d_%d", iChnno+1, iStreamno);
    sprintf(szPtzSecStr, "ENC_CHANNEL_%d", iChnno+1);    

    pObj->iProfile = pro.GetProperty(szSectionStr, (char*)"ENC_Profile", 0);
    pObj->iBitRate = pro.GetProperty(szSectionStr, (char*)"ENC_Bitrate", 0);
    pObj->iFPS = pro.GetProperty(szSectionStr, (char*)"ENC_Fps", 0);
    pObj->iResolution = pro.GetProperty(szSectionStr, (char*)"ENC_Resolution", 0);
    pObj->iCbr = pro.GetProperty(szSectionStr, (char*)"ENC_Cbr", 0);
    pObj->iGop = pro.GetProperty(szSectionStr, (char*)"ENC_Gop", 0);
    pObj->iIQP = pro.GetProperty(szSectionStr, (char*)"ENC_IQP", 0);
    pObj->iPQP = pro.GetProperty(szSectionStr, (char*)"ENC_PQP", 0);
    pObj->iMinQP = pro.GetProperty(szSectionStr, (char*)"ENC_MinQP", 0);
    pObj->iMaxQP = pro.GetProperty(szSectionStr, (char*)"ENC_MaxQP", 0);
    pObj->iAudioEnable = pro.GetProperty(szSectionStr, (char*)"ENC_AudioEnable", 0);
    pObj->iAudioFormat = pro.GetProperty(szSectionStr, (char*)"ENC_AudioFormat", 0);
    pObj->iAudioRate = pro.GetProperty(szSectionStr, (char*)"ENC_AudioRate", 0);
    pObj->iBits = pro.GetProperty(szSectionStr, (char*)"ENC_Bits", 0);

    //PTZ信息
    ptz->iPtzType = pro.GetProperty(szPtzSecStr, (char*)"ENC_TYPE", 0);
    ptz->iAddressCode = pro.GetProperty(szPtzSecStr, (char*)"ENC_AddressCode", 0);

    return 0;
}
long CEncoderSetting::SetEncVideo(EncoderParam* pObj, PtzSetting* ptz, int no, bool bCallback )
{
    if(pObj == NULL || ptz == NULL || no < 0)
    {
        return -1;
    }
    long lRet = -1;
    CProfile pro;
    //_DEBUG_("%s", szBuf);
    if (pro.load((char*)CFG_FILE_ENCODER) < 0)
    {
        _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        return lRet;
    }
    // 获取通道个数
    int iChnCnt = pro.GetProperty((char*)"COUNT", (char*)"EncChannelCount", 0);
    if(iChnCnt <=0)
    {
        return -1;
    }

    // 取得通道号、码流号
    // no=0表示通道1主码流，1表示通道1子码流，依次类推
    int iChnno = 0;
    int iStreamno = 0;
    if( no % 2 == 0) // 主码流
    {
        iChnno = no / 2;
        iStreamno = 1;
    }
    else // 子码流
    {
        iChnno = (no + 1) / 2 - 1;
        iStreamno = 2;
    }
    char szSectionStr[80] = {0};
    sprintf(szSectionStr, "ENC_PARAM_%d_%d", iChnno+1, iStreamno);
    char szPtzSecStr[80] = {0};
    sprintf(szPtzSecStr, "ENC_CHANNEL_%d", iChnno+1);

    pro.SetProperty(szSectionStr, (char*)"ENC_Profile", pObj->iProfile);
    pro.SetProperty(szSectionStr, (char*)"ENC_Resolution", pObj->iResolution);
    pro.SetProperty(szSectionStr, (char*)"ENC_Fps", pObj->iFPS);
    pro.SetProperty(szSectionStr, (char*)"ENC_Gop", pObj->iGop);
    pro.SetProperty(szSectionStr, (char*)"ENC_Cbr", pObj->iCbr);
    pro.SetProperty(szSectionStr, (char*)"ENC_Bitrate", pObj->iBitRate);
    pro.SetProperty(szPtzSecStr, (char*)"ENC_TYPE", ptz->iPtzType);
    pro.SetProperty(szPtzSecStr, (char*)"ENC_AddressCode", ptz->iAddressCode);
    // 再次加载以保存
    pro.load((char*)CFG_FILE_ENCODER);
    
	if ( bCallback)
	{
	    // 数据更改
	    OnParamChange(no, (void *)pObj);
	    OnPtzChange(iChnno, (void *)ptz);
	}
    // 写入配置分区
    lRet = WriteCfgFileToCfgMtd();
    return lRet;
}

// 设置于回调中的函数
long CEncoderSetting::GetEncForwardport(char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    // 获取透明端口信息
    ForwardportSetting sfp;
    memset(&sfp, 0, sizeof(ForwardportSetting));
    pThis->GetEncForwardport(&sfp);
    sprintf(szBuf, "%d;%d;%d;", sfp.iEnable, sfp.iType, sfp.iPort);
    _DEBUG_("Get Forward port info: %d, %d, %d", sfp.iEnable, sfp.iType, sfp.iPort);

    return 0;
}
long CEncoderSetting::SetEncForwardport(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    _DEBUG_(" Forward port [%s]", szBuf);
    long lRet = -1;
    char *szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[1024]={0};
    strcpy(szTmpBuf, szBuf);
    // 解析信息
    ForwardportSetting sfp;
    memset(&sfp, 0, sizeof(ForwardportSetting));

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return lRet;
    }
    sfp.iEnable= atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return lRet;
    }
    sfp.iType= atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return lRet;
    }
    sfp.iPort= atoi(szPtr);

    // 写入文件
    lRet = pThis->SetEncForwardport(&sfp);

    return lRet;
}

long CEncoderSetting::GetEncPtzPort(char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;

    // 获取PTR PORT信息
    PtzPortSetting sPtzPort;
    memset(&sPtzPort, 0, sizeof(PtzPortSetting));
    pThis->GetEncPtzPort(&sPtzPort);
    sprintf(szBuf, "%d;%d;%d;%d;", sPtzPort.iBaude, sPtzPort.iDataBits, \
            sPtzPort.iParity, sPtzPort.iStopBits);
    return 0;
}
long CEncoderSetting::SetEncPtzPort(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%s]", szBuf);
    long lRet = 0;
    char *szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[1024]={0};
    strcpy(szTmpBuf, szBuf);
    // 解析PTR PORT信息
    PtzPortSetting sPtzPort;
    memset(&sPtzPort, 0, sizeof(PtzPortSetting));

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzPort.iBaude = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzPort.iDataBits = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzPort.iParity = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzPort.iStopBits = atoi(szPtr);

    // 写入文件
    lRet = pThis->SetEncPtzPort(&sPtzPort);

    return lRet;
}
long CEncoderSetting::GetEncAlarmin(char* szBuf, int no, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%d]", no);
    
	// "1;1;10000000000001;1;100;1;10;";	
	//报警输入通道号;是否使能;编号;类型;去抖动时间;关联报警输出通道号;输出延时时间;
    AlarmInSetting sAlarmin;
    memset(&sAlarmin, 0, sizeof(AlarmInSetting));
    pThis->GetEncAlarmin(&sAlarmin, no-1);
    int iOut = sAlarmin.iTrigerAlarmOut;
    if(iOut<0)
    {
        iOut = 0;
    }
    else
    {
        iOut+=1;
    }
    sprintf(szBuf, "%d;%d;%s;%d;%d;%d;%d;", no, sAlarmin.iEnable, sAlarmin.szId, \
                    sAlarmin.iNormalStatus, sAlarmin.iStableTime, iOut, \
                    sAlarmin.iOutDelay);
    return 0;
}
long CEncoderSetting::GetEncAlarmout(char* szBuf, int no, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%d]", no);
    // "1;1;20000000000001";//报警输出通道号;是否使能;编号;
    AlarmOutSetting sAlarmout;
    memset(&sAlarmout, 0, sizeof(AlarmOutSetting));
    pThis->GetEncAlarmout(&sAlarmout, no-1);

    sprintf(szBuf, "%d;%d;%s;", no, sAlarmout.iEnable, sAlarmout.szId);
    
    return 0;
}
long CEncoderSetting::SetEncAlarm(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    long lRet = 0;
    //_DEBUG_("[%s]", szBuf);
    AlarmInSetting sAlarmin;
    memset(&sAlarmin, 0, sizeof(AlarmInSetting));
    AlarmOutSetting sAlarmout;
    memset(&sAlarmout, 0, sizeof(AlarmOutSetting));

    // "1;1;10000000000001;1;100;1;20000000000001;10;";
    //报警输入通道号;是否使能;编号;类型;去抖动时间;关联报警输出通道号;报警输出通道编号;输出延时时间;
    char *szPtr = NULL;
    char *szDelim1 = (char*)";";
    int  iAlarminNo = 0;
    char szTmpBuf[4096]={0};
    strcpy(szTmpBuf, szBuf);

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    iAlarminNo = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sAlarmin.iEnable = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(sAlarmin.szId, "%s", szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sAlarmin.iNormalStatus= atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sAlarmin.iStableTime= atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    int iOut = atoi(szPtr);
    if( iOut <= 0)
    {
        iOut = -1;
    }
    else
    {
        iOut-=1;
    }
    sAlarmin.iTrigerAlarmOut = iOut;

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(sAlarmout.szId, "%s", szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sAlarmin.iOutDelay= atoi(szPtr);
    sAlarmout.iEnable = 1;
    pThis->SetEncAlarm(&sAlarmin, iAlarminNo-1, &sAlarmout, iOut);

    return lRet;
}

long CEncoderSetting::GetEncChannel(char* szBuf, int chnno, void* param)
{
     if( param==NULL )
     {
         _DEBUG_("param==NULL!");
         return -1;
     }
     CEncoderSetting * pThis = (CEncoderSetting*)param;
     //_DEBUG_("[%d]", chnno);
     char szChnName[256] = {0};
     char szId[256] = {0};
     pThis->GetEncChannel(szChnName, szId, chnno-1);

     sprintf(szBuf, "%d;%s;%s;", chnno, szChnName, szId);

     return 0;

}
long CEncoderSetting::SetEncChannel(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%s]", szBuf);   
    long lRet = 0;
    char szSectionStr[80] = {0};
    char szChnName[256] = {0};
    char szId[256] = {0};
    int iChnNo = 0;
    char *szPtr = NULL;
    char *szDelim1 = (char*)";";
    char szTmpBuf[4096]={0};
    strcpy(szTmpBuf, szBuf);

    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return -1;
    }
    iChnNo = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(szChnName, "%s", szPtr);
        
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sprintf(szId, "%s", szPtr);

    pThis->SetEncChannel(szChnName, szId, iChnNo-1);
    return lRet;
}

long CEncoderSetting::GetEncDisplay(char* szBuf, int chnno, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%d]", chnno);
    EncOsdSettings sOsdSetting;
    memset(&sOsdSetting, 0, sizeof(EncOsdSettings));
    pThis->GetEncDisplay(&sOsdSetting, chnno-1);
    // 穿换成字符串返回
    // 1;1;1,1,2,0, ;0,0,0,1, ;1,1,1,2, ;1,4,5,4,IPC-HD-00001;0,12,15,3,this is test!;1,10,10,3,hello;1,0,0,3,;
    int iLen = 0;
    sprintf(szBuf+iLen, "%d;%d;", chnno, sOsdSetting.iFont);
    for(int i=0; i<MAXOSD_COUNT; i++)
    {
        iLen = strlen(szBuf);
        sprintf(szBuf+iLen, "%d,%d,%d,%d,%s;", sOsdSetting.osd[i].iEnable, sOsdSetting.osd[i].iX, \
                sOsdSetting.osd[i].iY, sOsdSetting.osd[i].iType, sOsdSetting.osd[i].szOsd);     
    }
    //_DEBUG_("szBuf = [ %s ]", szBuf);
    return 0;
}
 
long CEncoderSetting::SetEncDisplay(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    long lRet = -1;
    char *szDelim1 = (char*)";";
    char *szDelim2 = (char*)",";
    char* szPtr = NULL;
    // 1;1;1,1,2, ;0,0,0, ;1,1,1, ;1,4,5,IPC-HD-00001;0,12,15,this is test!;1,10,10,hello;1,0,0,;
    //_DEBUG_("%s", szBuf);
    
    // 解析字符串
    EncOsdSettings sOsdSetting;
    memset(&sOsdSetting, 0, sizeof(EncOsdSettings));
    char szTmpBuf[4096]={0};
    strcpy(szTmpBuf, szBuf);
    char szValues[MAXOSD_COUNT][400];

    // 通道
    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return lRet;
    }
    sOsdSetting.iChannelNo = atoi(szPtr);
    sOsdSetting.iChannelNo-=1;

    // 字体
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return lRet;
    }
    sOsdSetting.iFont = atoi(szPtr);

    int i=0;
    while(i<MAXOSD_COUNT && (szPtr = strtok(NULL, szDelim1)) != NULL)
    {
        memset(szValues[i], 0 , 400);
        sprintf(szValues[i], "%s", szPtr);
        //_DEBUG_("%d : %s\n", i, szValues[i]);
        i++;
    }
    i=0;
    while(i<MAXOSD_COUNT)
    {
        //日期
        //时间
        //日期时间
        //名称
        //内容一
        //内容二
        //内容三
        //0,12,15,3,this is test!
        //iEnbale
        if( (szPtr = strtok(szValues[i], szDelim2)) == NULL)
        {
            break;
        }
        sOsdSetting.osd[i].iEnable = atoi(szPtr);
        //iX
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sOsdSetting.osd[i].iX= atoi(szPtr);
        //iY
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sOsdSetting.osd[i].iY = atoi(szPtr);
        //iType
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sOsdSetting.osd[i].iType= atoi(szPtr);
        //szOsd
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sprintf(sOsdSetting.osd[i].szOsd, "%s", szPtr);

        i++;    
    }
 
    pThis->SetEncDisplay(&sOsdSetting, sOsdSetting.iChannelNo);
    return lRet;
}

long CEncoderSetting::GetEncTransfer(char* szBuf, int chnno, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%d]", chnno);
    EncNetSettings sEncNetSettings;
    memset(&sEncNetSettings, 0 , sizeof(EncNetSettings));
    pThis->GetEncTransfer(&sEncNetSettings, chnno-1);
    
    //"1;1,1,1,192.168.1.1,4511,2;1,2,0,192.168.1.2,4511,2;1,3,3,192.168.1.3,4512,0;"
    // 组合成字符串返回
    int iLen = 0;
    sprintf(szBuf+iLen, "%d;", chnno);
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        iLen = strlen(szBuf);
        sprintf(szBuf+iLen, "%d,%d,%d,%s,%d,%d;", sEncNetSettings.net[i].iEnable, \
                    sEncNetSettings.net[i].iIndex, \
                    sEncNetSettings.net[i].iNetType, \
                    sEncNetSettings.net[i].szPlayAddress, \
                    sEncNetSettings.net[i].iPlayPort, \
                    sEncNetSettings.net[i].iMux);
    }

    return 0;
}
long CEncoderSetting::SetEncTransfer(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    long lRet = -1;
    char *szDelim1 = (char*)";";
    char *szDelim2 = (char*)",";
    char* szPtr = NULL;
    // "1;1,1,1,192.168.1.1,4511,2;1,2,0,192.168.1.2,4511,2;1,3,3,192.168.1.3,4512,0;"
    //_DEBUG_("%s", szBuf);
    // 解析字符串
    EncNetSettings sEncNetSetting;
    memset(&sEncNetSetting, 0, sizeof(EncNetSettings));
    char szTmpBuf[4096]={0};
    strcpy(szTmpBuf, szBuf);
    char szValues[MAXNET_COUNT][400];
    // 通道
    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return lRet;
    }
    sEncNetSetting.iChannelNo = atoi(szPtr);
    sEncNetSetting.iChannelNo-=1;
    int i=0;
    while(i<MAXNET_COUNT && (szPtr = strtok(NULL, szDelim1)) != NULL)
    {
        memset(szValues[i], 0 , 400);
        sprintf(szValues[i], "%s", szPtr);
        //_DEBUG_("%d : %s\n", i, szValues[i]);
        i++;
    }
    i=0;
    while(i<MAXNET_COUNT)
    {
        //1,3,3,192.168.1.3,4512,0;
        //iEnbale
        if( (szPtr = strtok(szValues[i], szDelim2)) == NULL)
        {
            break;
        }
        sEncNetSetting.net[i].iEnable = atoi(szPtr);
        //iIndex
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sEncNetSetting.net[i].iIndex= atoi(szPtr);
        sEncNetSetting.net[i].iIndex-=1;
        //iNetType
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sEncNetSetting.net[i].iNetType= atoi(szPtr);
        //szPlayAddress
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        if(szPtr==" "||strlen(szPtr)<7)
        {
            sprintf(sEncNetSetting.net[i].szPlayAddress, "%s", "");
        }
        else
        {
            sprintf(sEncNetSetting.net[i].szPlayAddress, "%s", szPtr);
        }
        //iPlayPort
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sEncNetSetting.net[i].iPlayPort = atoi(szPtr);
        //iMux
        if( (szPtr = strtok(NULL, szDelim2)) == NULL)
        {
            break;
        }
        sEncNetSetting.net[i].iMux = atoi(szPtr);

        i++;
    }
    // 写入配置文件

    pThis->SetEncTransfer(&sEncNetSetting, sEncNetSetting.iChannelNo);

    return lRet;

}

long CEncoderSetting::GetEncVideo(char* szBuf, int chnno, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    //_DEBUG_("[%d]", chnno);
    EncoderParam sEncParam;
    memset(&sEncParam, 0, sizeof(EncoderParam));
    //PTZ信息
    PtzSetting sPtzSet;
    memset(&sPtzSet, 0, sizeof(PtzSetting));
    pThis->GetEncVideo(&sEncParam, &sPtzSet, chnno-1);
    //通道号（1-通道1、主码流，2-通道1、子码流，3-通道2、主码流，。。。依次类推），
    //分辨率，帧率，I帧间隔，码率控制，码流大小，，PTZ类型，PTZ地址码
	//"2;3;25;10;1;4000;0;1;"
    sprintf(szBuf, "%d;%d;%d;%d;%d;%d;%d;%d;%d", chnno, sEncParam.iResolution, sEncParam.iFPS, \
                    sEncParam.iGop, sEncParam.iCbr, sEncParam.iBitRate, sEncParam.iProfile, \
                    sPtzSet.iPtzType, sPtzSet.iAddressCode);
    //_DEBUG_("%s", szBuf);
    return 0;
}

long CEncoderSetting::SetEncVideo(const char* szBuf, void* param)
{
    if( param==NULL )
    {
        _DEBUG_("param==NULL!");
        return -1;
    }
    CEncoderSetting * pThis = (CEncoderSetting*)param;
    long lRet = -1;
    //_DEBUG_("%s", szBuf);
    //var str1="2;3;25;10;1;4000;2;0;1;";
	//通道号（1-通道1、主码流，2-通道1、子码流，3-通道2、主码流，。。。依次类推），分辨率，帧率，I帧间隔，码率控制，码流大小，编码规格，PTZ类型，PTZ地址码
    // 解析字符串
    EncoderParam sEncParam;
    PtzSetting sPtzSet;
    char *szDelim1 = (char*)";";
    char* szPtr = NULL;
    char szTmpBuf[1024]={0};
    strcpy(szTmpBuf, szBuf);
    memset(&sPtzSet, 0, sizeof(PtzSetting));
    memset(&sEncParam, 0, sizeof(EncoderParam));
    // 通道
    if( (szPtr = strtok(szTmpBuf, szDelim1)) == NULL)
    {
        return lRet;
    }
    int iNum = atoi(szPtr);

    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iResolution= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iFPS= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iGop= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iCbr= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iBitRate= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sEncParam.iProfile = atoi(szPtr); 
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzSet.iPtzType= atoi(szPtr);
    if( (szPtr = strtok(NULL, szDelim1)) == NULL)
    {
        return -1;
    }
    sPtzSet.iAddressCode= atoi(szPtr);
    lRet = pThis->SetEncVideo(&sEncParam, &sPtzSet, iNum-1);

    return lRet;
}
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
// 设置回调
void CEncoderSetting::SetCB_OnOsdChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnOsdChange   = cbFun;
    m_pobjOnOsdChange = pCBobj;

}
void CEncoderSetting::SetCB_OnNetChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnNetChange   = cbFun;
    m_pobjOnNetChange = pCBobj;
}
void CEncoderSetting::SetCB_OnParamChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnParamChange   = cbFun;
    m_pobjOnParamChange = pCBobj;
}
void CEncoderSetting::SetCB_OnChannelChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnChannelChange   = cbFun;
    m_pobjOnChannelChange = pCBobj;
}
void CEncoderSetting::SetCB_OnPtzChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnPtzChange   = cbFun;
    m_pobjOnPtzChange = pCBobj;
}
void CEncoderSetting::SetCB_OnPtzPortChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnPtzPortChange   = cbFun;
    m_pobjOnPtzPortChange = pCBobj;
}
void CEncoderSetting::SetCB_OnAlarminChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnAlarminChange   = cbFun;
    m_pobjOnAlarminChange = pCBobj;
}
void CEncoderSetting::SetCB_OnAlarmoutChange(OnEncDataChangeCB_T cbFun, void* pCBobj)
{
    if (cbFun == NULL) {
        _DEBUG_("CALLBACK NULL!");
        return;
    }
    m_cbOnAlarmoutChange   = cbFun;
    m_pobjOnAlarmoutChange = pCBobj;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
long CEncoderSetting::OnOsdChange(int chnno, void* pData)
{
    if ( m_cbOnOsdChange != NULL ) {
        long lRet =  m_cbOnOsdChange(chnno, pData, m_pobjOnOsdChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnNetChange(int chnno, void* pData)
{    
    if ( m_cbOnNetChange != NULL ) {
        long lRet =  m_cbOnNetChange(chnno, pData, m_pobjOnNetChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnParamChange(int chnno, void* pData)
{
    if ( m_cbOnParamChange != NULL ) {
        long lRet =  m_cbOnParamChange(chnno, pData, m_pobjOnParamChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnChannelChange(int chnno, void* pData)
{
    if ( m_cbOnChannelChange != NULL ) {
        long lRet =  m_cbOnChannelChange(chnno, pData, m_pobjOnChannelChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnPtzChange(int chnno, void* pData)
{
    if ( m_cbOnPtzChange != NULL ) {
        long lRet =  m_cbOnPtzChange(chnno, pData, m_pobjOnPtzChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnPtzPortChange(int chnno, void* pData)
{
    if ( m_cbOnPtzPortChange != NULL ) {
        long lRet =  m_cbOnPtzPortChange(chnno, pData, m_pobjOnPtzPortChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnAlarminChange(int chnno, void* pData)
{
    if ( m_cbOnAlarminChange != NULL ) {
        long lRet =  m_cbOnAlarminChange(chnno, pData, m_pobjOnAlarminChange);
        return lRet;
    }
    return -1;
}
long CEncoderSetting::OnAlarmoutChange(int chnno, void* pData)
{
    if ( m_cbOnAlarmoutChange != NULL ) {
        long lRet =  m_cbOnAlarmoutChange(chnno, pData, m_pobjOnAlarmoutChange);
        return lRet;
    }
    return -1;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

long CEncoderSetting::CopyFile(const char* szSrcFile, const char* szDstFile)
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
long CEncoderSetting::MoveFile(const char* szSrcFile, const char* szDstFile)
{
    long lRet = -1;
    lRet = CopyFile(szSrcFile, szDstFile);
    if( lRet != 0){
        return lRet;
    }
    remove(szSrcFile);
    return 0;
}

long CEncoderSetting::WriteCfgFileToCfgMtd(char* szFile)
{
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







