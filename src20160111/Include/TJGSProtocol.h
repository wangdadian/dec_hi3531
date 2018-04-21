#pragma once 

#include <string.h>
#include <string>
#include "Protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"

#include "TJGSLib.h"
#include <map>
using namespace std;
using namespace NSTJGS;

class CTJGSProtocol : public CProtocol 
{
public:
	CTJGSProtocol(int nPort, CChannelManager *pChnManager  );
	virtual ~CTJGSProtocol();
	
	void Login( char *szServerIp, 
						char *szUserName, 
						char *szPasswd );

	
	int OnAlarm( char *szAlarmInId, int nAlarminId, int nStatus );
    int StopEncoder(int chnno);
private:
	CTJGSLib *m_pTJGSLib;


	static int TJGSCB_LiveStart(char *szPeerIp, 
									 char *szDeviceId, 
									 char *szDestIp, 
									 int uiDestPort, 
									 int nStreamNo, 
									 char *Resolution, 
									 char *nCastType, 
									 int nBitRate,
									 int nFrameRate, 
									 char *szGopM, 
									 int nGopN,
									 char *szAudio,
									 int nLiveHandle,
									 void* pCBObj );

	static int TJGSCB_AlarmControl(char* szPeerIp, 
									char* szDeviceId, 
									int nControlCmd, 
									void* pCBObj);


	
	static int TJGSCB_QueryDeviceStatus(char* szPeerIp, 
										char* szDeviceId, 
										sTJGSDeviceStatus *pDevStatus, 
										void* pCBObj);

	

	static int TJGSCB_DecoderStart(char *szPeerIp, 
									char *szDeviceId, 
									char *szDestIp, 
									int uiDestPort, 
									char *szCastType, 
									int nLiveHandle,
									void *pCBObj );


	static int TJGSCB_PTZ(char* szPeerIp, 
									char* szDeviceId, 
									int ePtzType, 
									int nSpeed, 
									void* pCBObj);

	//关闭实时点播

	static int TJGSCB_LiveEnd(char* szPeerIp, 
										 char* szDeviceId, 
										 int nLiveHandle, 
										 void* pCBObj);

	static void *WorkerProc( void *arg );
    char m_szDevicId[256];
	char m_szServerIp[50];
	char m_szUserName[50];
	char m_szPasswd[50];
    pthread_t m_th;
	int m_iExitFlag;
	
	map< int, int>m_mapEncLiveHandle;
	//map< int, int>m_mapDecLiveHandle;
};


