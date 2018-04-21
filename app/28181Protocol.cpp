#include "28181Protocol.h"


C28181Protocol::C28181Protocol( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager ) 
{
	m_p28181Lib = new C28181Lib( "64010000002000000001", 
									g_szLocalIp, 
									5060 );
	m_iExitFlag = 0;
	m_p28181Lib->SetLiveStartCB((CB28181_LiveStart)GB28181_LiveStartCB, (void*)this);
	m_p28181Lib->SetLiveEndCB((CB28181_LiveEnd)GB28181_LiveEndCB, (void*)this);
	m_p28181Lib->Set3thCallLiveStartCB( GB28181_DecoderStartCB, (void*)this);
	m_p28181Lib->Set3thCallLiveEndCB( GB28181_DecoderEndCB, (void*)this );

	pthread_create( &m_th, NULL, WorkerProc, (void*)this );
	//m_p28181Lib->RequestRegister(char * szPeerIp,char * szUserName,char * szPassWd)
}

void *C28181Protocol::WorkerProc( void *arg )
{
	C28181Protocol *pThis = ( C28181Protocol*)arg;
	while( pThis->m_iExitFlag == 0 )
	{
		sleep(10);
	}
	pthread_detach(pthread_self());
	return NULL;
}
int C28181Protocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

void C28181Protocol::Login( char *szServerIp, char *szUserName, char *szPasswd )
{
	strncpy( m_szServerIp, szServerIp, 50 );
	strncpy( m_szUserName, szUserName, 50 );
	strncpy( m_szPasswd, szPasswd, 50 );
	m_p28181Lib->RequestRegister( szServerIp, szUserName, szPasswd );
}

C28181Protocol::~C28181Protocol()
{
	PTR_DELETE( m_p28181Lib);
}


int C28181Protocol::GB28181_DecoderStartCB(char* szPeerIp, 
											char* &szDestIp, 
											int &nDestPort, 
											int nLiveHandle, 
											void* pCBObj)
{
	C28181Protocol *pThis = (C28181Protocol*)pCBObj;

	return 0;
}


int C28181Protocol::GB28181_DecoderEndCB(char* szPeerIp, 
											int nLiveHandle, 
											void* pCBObj )
{
	C28181Protocol *pThis = (C28181Protocol*)pCBObj;
	return 0;
}

//实时点播
int C28181Protocol::GB28181_LiveStartCB(char* szPeerIp, 
										 char* szDeviceId, 
										 char* szDestIp, 
										 char* szDestPort, 
										 int nLiveHandle, 
										 void* pCBObj)

{
	_DEBUG_("GB28181_LiveStart PeerIP:%s, sDeviceId:%s, szDestIp:%s, szDestPort:%s, nLiveHandle:%d \n", 
		szPeerIp, 
		szDeviceId, 
		szDestIp, 
		szDestPort, 
		nLiveHandle);

	return 0;
}
 

//关闭实时点播
int C28181Protocol::GB28181_LiveEndCB(char* szPeerIp, 
										 char* szDeviceId, 
										 int nLiveHandle, 
										 void* pCBObj)

{
	_DEBUG_("GB28181_LiveEnd PeerIP:%s, szDeviceId:%s, nLiveHandle:%d \n", 
		szPeerIp, 
		szDeviceId, 
		nLiveHandle );


	
	return 0;
}



