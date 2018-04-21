#ifndef STREAM_TRANSFORM_H_
#define STREAM_TRANSFORM_H_
#include "NetUdp.h"
#include "NetTcp.h"


#define MAXDSTCNT 5

typedef struct _DstInfo
{
	int iStreamNo;
	int iDstType;
	void *pDst;	
	char szIp[50];
	int nPort;
	void *pReqData;
	void *pReqData2;
	int nDataLen;
	int nDataLen2;
	int nRtpFlag;
	int nSeqNo;
	time_t tmLastReceived;
}DstInfo;

typedef void (*StreamRecvCB)( int nStreamNo, 
							   char *szData, 
							   int nLen, 
							   void *pCaller );

class CStreamTransform
{

public:
	CStreamTransform();
	virtual ~CStreamTransform();

	void SetStreamDst( int iStreamNo, 
					   int nDstSendType,
					   char *szIP,
					   int nPort, 
					   int nRtpFlag=0);

     void SendStream( int iStreamNo, 
	 				  char *szData, 
	 				  int nLen );

	void SetStreamSrc( int iStreamNo, 
						int nSrcType, 
						char *szIP, 
						int nPort );

	void SetStreamRecvCB( StreamRecvCB callback, 
						void *pCaller );

	
	void StopStreamDst( int iStreamNo );


	
	void StopStreamSrc( int iStreamNo );

	void SendSrcStreamReq( int iStreamNo, 
						char *szData, 
						int nLen );
	
	void SetHeartBeatData( void *data, 
						int nLen, 
						int nTimeOut  );

	void SetSrcReqData( int iStreamNo, 
						char *szData, 
						int nLen, 
						int iDataIndex=1 );
	 
private:

	DstInfo m_dstInfo[MAXDSTCNT];
	DstInfo m_srcInfo[MAXDSTCNT];

	
	int m_nDstCount;
	int m_nSrcCount;
	CRITICAL_SECTION m_streamlock;

	StreamRecvCB m_recvCB;

	pthread_t m_thRecv;

	int m_iExitFlag;
	char *m_szBuf;
	char *m_szTmpBuf;

	void *m_pCaller;

	unsigned char *m_pHeartBeatData;
	int m_nHeartBeatDataLen ;
	int m_nHeartBeatTimeout;

	time_t tm_lastHB; 
	static void *ThreaProc( void *arg );
	void worker();
	
};
#endif

