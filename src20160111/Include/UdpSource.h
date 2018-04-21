#ifndef _UDPSOURCE_H_
#define _UDPSOURCE_H_


#include <pthread.h>
#include <stdlib.h>
#include "Source.h"
#include "NetUdp.h" 

struct RTPHeader
{
#ifdef RTP_BIG_ENDIAN
	uint8_t version:2;
	uint8_t padding:1;
	uint8_t extension:1;
	uint8_t csrccount:4;
	
	uint8_t marker:1;
	uint8_t payloadtype:7;
#else // little endian
	uint8_t csrccount:4;
	uint8_t extension:1;
	uint8_t padding:1;
	uint8_t version:2;
	
	uint8_t payloadtype:7;
	uint8_t marker:1;
#endif // RTP_BIG_ENDIAN
	
	uint16_t sequencenumber;
	uint32_t timestamp;
	uint32_t ssrc;
};


class CUdpSource : public CSource
{
public:
	CUdpSource( char *szAddr );
	virtual ~CUdpSource();

	void start();
	void stop();


private:

	char m_szAddr[100];

	CNetUdp *m_pNetUdp;
	pthread_t m_th;
	static void *RecvProc( void *arg );
	int m_iExit;


	
	void RemoveRtpHeader( void *pData, int nDataLen, int &nRtpTailIndex );
	
	RTPHeader m_rtpheader[5];
	int m_nRtpHeaderIndex;
	int m_nRtpFlag;

};

#endif

