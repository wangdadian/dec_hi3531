#ifndef DECODER_SETTING_H
#define  DECODER_SETTING_H


#include "DWConfig.h"
#include "FlashRW.h"
#include "define.h"
#include "Facility.h"
#include "IniFile.h"
#include "ConfigMtdFile.h"
#include "dwstype.h"
#include "sample_comm.h"
#include "PublicDefine.h"
#include "mdrtc.h"

class CDecoderSetting
{
public:
    CDecoderSetting();
    ~CDecoderSetting();
    long Init();
    // 获取解码通道URL，iChn-解码通道号，szUrl-保存获取的URL(调用者释放内存)
    // 失败返回-1，成功返回0，并将获取的URL储存在szUrl中
    long GetDynamicDecUrl(int iChn, char*& szUrl);
    // 
    // 获取NTP信息
    long GetNtpInfo(struNtpInfo* ntp);
    // 获取通道个数
    long GetChannelCount(int iMode, int &iCount);
    // 设置于回调中的函数
    // 用户登录及验证
    static long UserLogin(char* szUserName, char* szPassword, void* param);
    //获取用户信息，密码填空
    static long GetUserInfo(struUserInfo* &pUserInfo, int &iUserCount, void* param);
    //获取、设置OSD信息
    static long GetDecOsdInfo(struDecOsdInfo* pDecOsdInfo, void *param);
    static long SetDecOsdInfo(struDecOsdInfo* pDecOsdInfo, void *param);
    //获取、设置OSD信息---WEB
    static long GetDecOsdInfoWeb(struDecOsdInfo* pDecOsdInfo, void *param);
    static long SetDecOsdInfoWeb(struDecOsdInfo* pDecOsdInfo, void *param);
    // 修改用户信息
    static long EditUserInfo(const struUserInfo* pUserInfo, void* param);
    static long GetNetworkInfo(struNetworkInfo* pNI, void* param);
    static long SetNetworkInfo(const struNetworkInfo* pNI, void* param);
    // 获取、设置SIP服务器信息
    static long GetSipServer(struSipServer* pSS, void* param);
    static long SetSipServer(const char* szBuf, void* param);
    static long UpgradeDevice(const char* szData, unsigned long ulLen, void* param);
    static long GetDeviceMac(char *szMAC, void* param);
    static long SetDeviceMac(const char *szMAC, void* param);
    // 获取设备型号
    static long GetDeviceModel(int& iModel, void* param);
	static long SetDeviceModel(int iModel, void* param);
    // 获取设备序列号
    static long GetDeviceSN(char* szSN, void* param);
    // 设置设备序列号
    static long SetDeviceSN(const char* szSN, void* param);
    // 获取设备主控软件版本号
    static long GetDeviceVer(char* szVer, void* param);
    static long GetDevBuildtime(char* szTime, void* param);
    // 获取、设置解码通道信息
    static long GetDecChannelInfo(struDecChannelInfo* pDCI, void* param);
    static long SetDecChannelInfo(const struDecChannelInfo* pDCI, void* param);
    // 获取、设置解码分辨率
    static long GetResolution(struDisplayResolution* pDR, void* param);
    static long SetResolution(const struDisplayResolution* pDR, void* param);
    // 获取、设置动态解码URL
    static long GetDynamicDecUrl(struDynamicDecUrl* pDDU, void* param);
    static long SetDynamicDecUrl(const struDynamicDecUrl* pDDU, void* param);
    static long ResetParam(int iType, void* param);
    static long GetHostname(char* szHostname, void* param);
    static long SetHostname(const char* szHostname, void* param);
    static long SetSysTime(const char* szTime, void* param);
    static long GetSysTime(char* szTime, void* param);
    // 获取、设置NTP服务器IP地址
    static long GetNtp(char* szBuf, void* param);
    static long SetNtp(const char* szBuf, void* param);
    // OSD

    // 获取通道个数
    static long GetChannelCount(int iMode, int &iCount, void* param);

	void SetCB_DDU( SetDynamicDecUrlCB_T callback, void *pCBObj );
    void SetCB_OsdShow( DecOsdShowCB_T callback, void *pCBObj );
	
	void SetCB_Resolution( SetResolutionCB_T callback, void *pCBObj );

	
	long ReadDynamicDecUrl(int iChn, char*& szUrl);
  	long ReadDynamicDecUrl(int iChn, struDynamicDecUrl* pDDU);

    // 网络
    long SetNetIp(char* ip, char* netmask);
    long SetGateway(char* gw);
    long GetGateway(char* gw);
private:
    long CopyFile(const char* szSrcFile, const char* szDstFile);
    long MoveFile(const char* szSrcFile, const char* szDstFile);
    long WriteCfgFileToCfgMtd(char* szFile=NULL);
	long WriteCfgFileToCfgMtd5(char* szFile=NULL);
    long check_auth(const char* szUserName, const char* szPassword);
    long load_userinfo();
    static void* ThNtpWorker(void* param);
	static void* ThWriteMtdWorker(void* param);
    long NtpSetTime(struNtpInfo *ntp);
    long SetResolution(const struDisplayResolution* pDR);
private: 
    CDWConfig* m_pDWConfig;
    CWebDecoder* m_pWebDecoder;
    CFlashRW*  m_pFlashRW;
    CConfigMtdFile* m_pConfigMtdFile;
    long m_lConfigMtdLen;
	SetDynamicDecUrlCB_T m_cbSetDDU;
	SetResolutionCB_T m_cbSetResolution;

    DecOsdShowCB_T m_cbSetOsdShow;

	char m_szStr1[1024];
	void *m_pDDUCaller;
    struUserInfo* m_pUserInfo;
    int m_iUserCount;
    struNtpInfo m_objNtpInfo;
    pthread_t m_thNtpWorker;
    bool m_bNtpWorkerExit;
    pthread_mutex_t m_ntpMutexLock;

	pthread_mutex_t m_mtdlock;
	bool m_bCfgFileChanged;// 配置文件是否更改
	bool m_bWriteMtdNow;// 是否立即写入配置分区
	bool m_bWriteMtdExit;
    pthread_t m_thWriteMtdWorker;
};


#endif

