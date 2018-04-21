#ifndef ENCODER_SETTING_H
#define  ENCODER_SETTING_H


#include "EWConfig.h"
#include "FlashRW.h"
#include "define.h"
#include "Facility.h"
#include "IniFile.h"
#include "ConfigMtdFile.h"
#include "dwstype.h"
#include "sample_comm.h"
#include "PublicDefine.h"
#include "mdrtc.h"

// 数据变化回调函数类型
typedef long (*OnEncDataChangeCB_T)(int chnno, void* pData, void* pObj);

class CEncoderSetting
{
public:
    CEncoderSetting();
    ~CEncoderSetting();
    long Init();

    // 获取配置文件中的数据信息
    long GetEncoderSettings(EncSettings *sEncs);

    // 设置回调
    void SetCB_OnOsdChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnNetChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnParamChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnChannelChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnPtzChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnPtzPortChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnAlarminChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
    void SetCB_OnAlarmoutChange(OnEncDataChangeCB_T cbFun, void* pCBobj=NULL);
	//
	long GetEncForwardport(ForwardportSetting* pObj);
    long SetEncForwardport(ForwardportSetting* pObj);
	long GetEncPtzPort(PtzPortSetting* pObj);
    long SetEncPtzPort(PtzPortSetting* pObj);
    long GetEncAlarmin(AlarmInSetting* pObj, int no);
    long GetEncAlarmout(AlarmOutSetting* pObj, int no);
    long SetEncAlarm(AlarmInSetting* pObjIn, int inno, AlarmOutSetting* pObjOut, int outno);
    long GetEncChannel(char* szName, char* szId, int no);
    long SetEncChannel(char* szName, char* szId, int no);
    long GetEncDisplay(EncOsdSettings* pObj, int no);
    long SetEncDisplay(EncOsdSettings* pObj, int no);
    long GetEncTransfer(EncNetSettings* pObj, int no);
    long SetEncTransfer(EncNetSettings* pObj, int no, bool bCallback=true );
    long GetEncVideo(EncoderParam* pObj, PtzSetting* ptz, int no);
    long SetEncVideo(EncoderParam* pObj, PtzSetting* ptz, int no, bool bCallback=true);
    
    // 设置于回调中的函数
    static long GetEncForwardport(char* szBuf, void* param);
    static long SetEncForwardport(const char* szBuf, void* param);
    static long GetEncPtzPort(char* szBuf, void* param);
    static long SetEncPtzPort(const char* szBuf, void* param);
    static long GetEncAlarmin(char* szBuf, int no, void* param);
    static long GetEncAlarmout(char* szBuf, int no, void* param);
    static long SetEncAlarm(const char* szBuf, void* param);
    static long GetEncChannel(char* szBuf, int chnno, void* param);
    static long SetEncChannel(const char* szBuf, void* param);
    
    static long GetEncDisplay(char* szBuf, int chnno, void* param);
    static long SetEncDisplay(const char* szBuf, void* param);

    static long GetEncTransfer(char* szBuf, int chnno, void* param);
    static long SetEncTransfer(const char* szBuf, void* param);

    static long GetEncVideo(char* szBuf, int chnno, void* param);
    static long SetEncVideo(const char* szBuf, void* param);
    
private:
    // 数据变化调用函数
    long OnOsdChange(int chnno, void* pData);
    long OnNetChange(int chnno, void* pData);
    long OnParamChange(int chnno, void* pData);
    long OnChannelChange(int chnno, void* pData);
    long OnPtzChange(int chnno, void* pData);
    long OnPtzPortChange(int chnno, void* pData);
    long OnAlarminChange(int chnno, void* pData);
    long OnAlarmoutChange(int chnno, void* pData);
private:
    long CopyFile(const char* szSrcFile, const char* szDstFile);
    long MoveFile(const char* szSrcFile, const char* szDstFile);
    long WriteCfgFileToCfgMtd(char* szFile=NULL);
    long check_auth(const char* szUserName, const char* szPassword);
    long load_userinfo();
    
private: 
    CEWConfig* m_pEWConfig;
    CFlashRW*  m_pFlashRW;
    CConfigMtdFile* m_pConfigMtdFile;
    long m_lConfigMtdLen;


    // 回调用变量
    OnEncDataChangeCB_T m_cbOnOsdChange;
    void* m_pobjOnOsdChange;
    OnEncDataChangeCB_T m_cbOnNetChange;
    void* m_pobjOnNetChange;
    OnEncDataChangeCB_T m_cbOnParamChange;
    void* m_pobjOnParamChange;
    OnEncDataChangeCB_T m_cbOnChannelChange;
    void* m_pobjOnChannelChange;
    OnEncDataChangeCB_T m_cbOnPtzChange;
    void* m_pobjOnPtzChange;
    OnEncDataChangeCB_T m_cbOnPtzPortChange;
    void* m_pobjOnPtzPortChange;
    OnEncDataChangeCB_T m_cbOnAlarminChange;
    void* m_pobjOnAlarminChange;
    OnEncDataChangeCB_T m_cbOnAlarmoutChange;
    void* m_pobjOnAlarmoutChange;

};


#endif














