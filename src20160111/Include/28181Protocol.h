#pragma once

#include "Protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"

#include "28181Lib.h"


using namespace NS28181;

class C28181Protocol : public CProtocol 
{

public:
	C28181Protocol(int nPort, CChannelManager *pChnManager  );
	virtual ~C28181Protocol();


	
	void Login( char *szServerIp, 
						char *szUserName, 
						char *szPasswd );
    int StopEncoder(int chnno);
private:
	C28181Lib *m_p28181Lib;


	static int GB28181_LiveStartCB(char* szPeerIp, 
										 char* szDeviceId, 
										 char* szDestIp, 
										 char* szDestPort, 
										 int nLiveHandle, 
										 void* pCBObj);

	extern int GB28181_DecoderStartCB(char* szPeerIp, 
											char* &szDestIp, 
											int &nDestPort, 
											int nLiveHandle, 
											void* pCBObj);

	
	extern int GB28181_DecoderEndCB(char* szPeerIp, 
										int nLiveHandle, 
										void* pCBObj );

	extern int GB28181_DecoderEnd

	//关闭实时点播

	static int GB28181_LiveEndCB(char* szPeerIp, 
										 char* szDeviceId, 
										 int nLiveHandle, 
										 void* pCBObj);

	static void *WorkerProc( void *arg );

	char m_szServerIp[50];
	char m_szUserName[50];
	char m_szPasswd[50];
    pthread_t m_th;
	int m_iExitFlag;

};


