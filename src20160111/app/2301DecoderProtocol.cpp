
#include "2301DecoderProtocol.h"
#include <string.h>

C2301DecoderProtocol::C2301DecoderProtocol( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager )
{
	m_iExitFlag = 0;
	m_iCmdIndex = 0;
	pthread_mutex_init( &m_lock, NULL );
	m_pServer = new CSyncNetServer( C2301DecoderProtocol::THWorker , (void*)this );
	m_pServer->StartServer( nPort);
}

C2301DecoderProtocol::~C2301DecoderProtocol()
{
	m_iExitFlag = 1;
	
	if ( m_pServer != NULL)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	pthread_mutex_destroy( &m_lock );
}

int C2301DecoderProtocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

void C2301DecoderProtocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s", net->getRemoteAddr());

	C2301DecoderProtocol *pThis = ( C2301DecoderProtocol*)p;

	char szBuf[1024]={0};
	int nBufLen = 0;
	while( pThis->m_iExitFlag == 0 )
	{
		if ( !net->IsOK() ) break;
		char tmp[60]={0};
	
		int ret = net->RecvT( tmp, 1, 5000 );
		_DEBUG_("recv :%d\n", ret );
		if ( ret > 0 )
		{
			if ( nBufLen + ret >= 1024 )
			{
				nBufLen = 0;
				break;
			}
			
			memcpy( szBuf+nBufLen, tmp, ret );
			nBufLen += ret;
			pThis->ProcessBuffer(szBuf, nBufLen);
		}
		if ( ret <= 0 ) break;
	};

	_DEBUG_("client disconnect:%s", net->getRemoteAddr());

}

int C2301DecoderProtocol::CheckSwitch(char *cmd)
{
	unsigned int nIP1=0, nIP2=0, nIP3=0, nIP4=0, nPort=0,iret = -1;
	uint64_t in = 0ULL;
	char pszRtn[50];

	printf( "CheckSwitch:cmd={%s}", cmd);


	
	int ret = sscanf(cmd, "start %d.%d.%d.%d %d 160 80 160 1\n\r", 
		&nIP1, 
		&nIP2, 
		&nIP3, 
		&nIP4, 
		&nPort );	//切换本地干线

	if ( ret != 5 )
	{
		printf("parse cmd failed.\n");
		return iret; 
	}
	else
	{
		char szUrl[256]={0};
		sprintf( szUrl, "udp/%d.%d.%d.%d:%d", nIP1, nIP2, nIP3, nIP4, nPort );
		m_pChnManager->SaveUrl(1, szUrl, 25);
		m_pChnManager->Open( 1, szUrl, 0, 25 );
		
		return 1;
	}

	return iret;
}

void C2301DecoderProtocol::ProcessBuffer(char *buf, int &iSize)
{
	int iRv=0;
	//g_pLog->Info(m_iLogModule,"ProcessBuffer:buf=%s", buf);

	
	int index = 0;
	char sztmp[1024]={0};
	for (int i=0;i<iSize;i++)
	{
		if (buf[i] == 0x0d )
		{
			memset( sztmp, 0, sizeof(sztmp));
			memcpy( (void*)sztmp, (void*)(buf+index), i - index );
			index = i;
			_DEBUG_("check switch:%s\n", sztmp );
			CheckSwitch( sztmp );
		}
	}

	if ( index == 0 ) return;
	
	if ( iSize - (index+1) > 0 )
	{
		memmove( (void*)buf, (void*)(buf+index), iSize - (index + 1 ) );
		iSize -= (index+1);
	}
	else
	{
		iSize = 0;
	}
	
}





