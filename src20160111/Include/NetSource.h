#ifndef _NETSOURCE_H_
#define _NETSOURCE_H_


#include "CommPort.h"
#include <pthread.h>
#include <stdlib.h>
#include "Source.h"


class CNetSource : public CSource
{
public:
	CNetSource( char *szAddr );
	virtual ~CNetSource();

	void start();
	void stop();


private:

	char m_szAddr[100];

	CCommPort *m_pCommPort;
	pthread_t m_th;
	static void *RecvProc( void *arg );

	static void ReceivedCB( char *buf, int iSize, char* szIP, int nPort, void* pObj);
	int m_iExit;
	bool m_bClient;
};

#endif


