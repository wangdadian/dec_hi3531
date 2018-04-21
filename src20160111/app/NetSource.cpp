#include "NetSource.h"
#include <unistd.h>
#include <stdio.h>
#include "PublicDefine.h"

CNetSource::CNetSource(char *szAddr ) : CSource( szAddr ) 
{
	try
	{
		strcpy( m_szAddr, szAddr );
		m_pCommPort = CreateCommPort(szAddr, NULL);
		if ( m_pCommPort == NULL ) return;
		//start();
		m_iExit = 0;
		m_th = 0;
		m_bClient = true;
	}
	catch( ... )
	{

	}
	
	char sztmpaddr[100]={0};
	strcpy( sztmpaddr, szAddr );
	
	if (strstr(sztmpaddr, "tcp/") !=NULL )
	{
		char *ptmp = strchr(sztmpaddr, ':');
		
		if ( ptmp!=NULL )
		{
			m_bClient = true;
		}
		else
		{
			m_bClient = false;
		}
	}

	if ( m_bClient )
	{
		_DEBUG_("start recv port [%s]", szAddr);
		pthread_create( &m_th, NULL, RecvProc, (void*)this );
	}
	else
	{
		_DEBUG_("set call back");
		m_pCommPort->SetReceivedCallback(ReceivedCB );
		void *pThis = (void*)this;
		m_pCommPort->SetServerObj(pThis);
	}
}

CNetSource::~CNetSource()
{
	stop();
	usleep(10*1000);
	if ( m_th != NULL)
	{
		pthread_join( m_th, NULL );
		m_th = 0;
		_DEBUG_( "recv soure exit." );
	}
	if ( m_pCommPort != NULL )
	{
		delete m_pCommPort;
		m_pCommPort = NULL;
	}
}

void CNetSource::ReceivedCB( char *buf, int iSize, char* szIP, int nPort, void* pObj)
{

	CNetSource *pThis = ( CNetSource *)pObj;
	if ( iSize > 0 )
	{	
		//printf("received:%d from %s:%d\n", iSize, szIP, nPort );

		if ( pThis->m_funcOnRecvCB != NULL )
		{
			pThis->m_funcOnRecvCB((unsigned char*) buf, iSize, pThis->m_pCbObj );
		}
	}
	
}

void *CNetSource::RecvProc( void *arg )
{
	
	CNetSource *pThis = (CNetSource*)arg;
    int nMaxSize = 2*1024;
	unsigned char *pbuf = (unsigned char*)malloc( 1024 * 1024 );
	//int nMaxSize = 200*1024;
    //unsigned char *pbuf = (unsigned char*)malloc( nMaxSize );
    
	while( pThis->m_iExit == 0 )
	{
		if ( pThis->m_pCommPort->IsOk() )
		{
			int len =	pThis->m_pCommPort->ReadT( pbuf, nMaxSize, 20);
			//_DEBUG_("RECV DATA LEN:%d\n", len );
			if( len > 0)
			{
                if ( pThis->m_funcOnRecvCB != NULL )
				{
					pThis->m_funcOnRecvCB( pbuf, len, pThis->m_pCbObj );
				}
			}
			else if ( len < 0 )
			{
                //_DEBUG_("!!!!!!!!!!!!!!!!!!!!! RECV LEN < 0");
				pThis->m_pCommPort->Close();
			}
			else// if ( len == 0 )
			{
				usleep( 1 );
				//_DEBUG_("!!!!!!!!!!! NO DATA RECV, CONTINUE!");
			}
		}
		else
		{
            //_DEBUG_("!!!!!!!! NET NOT OK, REOPEN!");
            pThis->m_pCommPort->Open();
			usleep( 20000 );
            //
		}
	};

	free( pbuf);
	//pthread_detach( pthread_self());
	return NULL;
}

void CNetSource::start()
{
	
	m_pCommPort->Open( );
}

void CNetSource::stop()
{
	m_iExit = 1;
	m_pCommPort->Close();
}


