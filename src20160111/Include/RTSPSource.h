#pragma once

#include "RTSPStreamClient.h"
#include <time.h>
#include "Source.h"
#include <pthread.h>
#include <string>

using namespace std;

class CRTSPSource : public CSource
{

public:
	CRTSPSource( char *szUrl );
	~CRTSPSource();
	
	int m_iExitFlag;
	void CheckConnect();

private:
	
	CRTSPStreamClient *m_pRtspClient[16];
	string m_strUrl[16];
	
	
	bool CheckOnvifUrlInvalid( char * strUrl );

	time_t m_tmLastData[16];

	
	static int rtspDataCallBack(void *pInputParam, 
									char *streamURL, 
									char* mediumName, 
									unsigned char* pbData, 
									int nDataSize, 
									struct timeval presentationTime );

	
	long StartRtspSend( unsigned int uiStreamNo );

	

	
	long ParseRtspUserAndPasswd( char *szUrl, char *szUserName, char *szPasswd );

	
	bool ParseRTSPUrl( string streamUri, char *szUrlIP, int &nRtspPort, char *szSurfix );


	
	void SendRtspStream( char *streamURL, 
									unsigned char* pbData, 
									int nDataSize );

	static void *CheckConnectTh( void * arg );

	pthread_t m_th;

};





