#ifndef ONVIFSOURCE_H_
#define ONVIFSOURCE_H_


#include "OnvifProtocol.h"
#include "RTSPStreamClient.h"
#include <time.h>
#include "Source.h"
#include <pthread.h>

class COnvifSource : public CSource
{

public:
	COnvifSource( char *szUrl );
	~COnvifSource();
    int StopEncoder(int chnno);

private:
	
	CRTSPStreamClient *m_pRtspClient[16];
	string m_strOnvifUrl;
	string m_strUrl[16];
	COnvifProtocol *m_pOnvif;
	
	long  InitOnvif();	
	
	bool CheckOnvifUrlInvalid( char * strUrl );

	time_t m_tmLastData[16];

	
	static int rtspDataCallBack(void *pInputParam, 
									char *streamURL, 
									char* mediumName, 
									unsigned char* pbData, 
									int nDataSize, 
									struct timeval presentationTime );

	
	long StartRtspSend( unsigned int uiStreamNo );

	
	static void OnvifDeviceStatus(void *pObject, int bOnline);

	void CheckConnect();
	
	long ParseRtspUserAndPasswd( char *szUrl, char *szUserName, char *szPasswd );

	
	bool ParseRTSPUrl( string streamUri, char *szUrlIP, int &nRtspPort, char *szSurfix );


	
	void SendRtspStream( char *streamURL, 
									unsigned char* pbData, 
									int nDataSize );

	
	static void* ThWorker(void* param);
	pthread_t m_th;
	int m_iExitFlag;
};

#endif


