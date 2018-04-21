

#include "UdpSource.h"
#include <unistd.h>
#include <stdio.h>
#include "PublicDefine.h"

CUdpSource::CUdpSource(char *szAddr ) : CSource( szAddr ) 
{
	strcpy( m_szAddr, szAddr );
	m_pNetUdp = new CNetUdp();
	char dev[64]={0};
	int port = 0;
	strncpy( dev, m_szAddr+4, 64 );
	char *p = strchr(dev, ':');
	if ( p!=NULL )
	{
		*p = 0; port = atoi( p+1 );
	}
	else
	{
		port = atoi(dev);
	}
	
	printf( "udp addr:%s port:%d\n", dev, port );
	m_pNetUdp->Open( port, dev, false );
	
	start();
	m_iExit = 0;

	
	m_nRtpHeaderIndex = 0;
	m_nRtpFlag = 0;

	pthread_create( &m_th, NULL, RecvProc, (void*)this );
}

CUdpSource::~CUdpSource()
{
	_DEBUG_("stop udp source");
	stop();
	usleep(100*1000);
	if ( m_th != 0)
	{
		_DEBUG_("join thread");
		//pthread_cancel(m_th);
		pthread_join( m_th, NULL );
		m_th = 0;
	}

	_DEBUG_("delete udp");
	if ( m_pNetUdp != NULL )
	{
		delete m_pNetUdp;
		m_pNetUdp = NULL;
	}
	_DEBUG_( "udp source exit");
}



void CUdpSource::RemoveRtpHeader( void *pData, int nDataLen, int &nRtpTailIndex )
{
	nRtpTailIndex = 0;
	
	if ( nDataLen < 12 ) return ;

    //////////////////////////////////////////////////////////////
    if(m_iRtpFlag == 0)
    {
        return;
    }
    
    RTPHeader header;
	memcpy(&header, pData, 12 );
	if (header.version != 2) 
	{
        return;
    }
    nRtpTailIndex = sizeof( RTPHeader) + header.csrccount * 4 ;
	
	if ( nRtpTailIndex > nDataLen ) 
	{
		nRtpTailIndex = 0;
        return;
	}
    m_nRtpFlag = 0;
    return;
    //////////////////////////////////////////////////////////////

	//RTPHeader header;
	//memcpy(&header, pData, 12 );

/*
	printf("rtp ver:%d pt:%d ts:%u cc:%d ssrc:%u\n", 
		header.version, 
		header.payloadtype,
		header.timestamp, 
		header.csrccount, 
		header.ssrc );
	*/

	if (m_nRtpHeaderIndex < 4  )
	{
		memcpy( &m_rtpheader[m_nRtpHeaderIndex], &header, sizeof( RTPHeader ) );
		m_nRtpHeaderIndex++;
		return ;
	}
	else
	{
		m_nRtpHeaderIndex = 4;
		for ( int i = 0; i < 4; i++ )
		{
			memcpy( &m_rtpheader[i], &m_rtpheader[i+1], sizeof(RTPHeader ));
		}
		
		memcpy( &m_rtpheader[m_nRtpHeaderIndex], &header, sizeof( RTPHeader ) );
	}

	
	if (header.version != 2 || 
		header.csrccount * 4 > nDataLen  ) return;

	int nLastFrame = 0;
	
	int nMatchCnt = 0;
	
	for ( int i = 0; i < 5; i++ )
	{
		if ( m_rtpheader[i].sequencenumber < nLastFrame )
		{
		}
		else
		{
			nMatchCnt++;
		}
		
		nLastFrame = m_rtpheader[i].sequencenumber;
	}
	
	//printf( "sequence match cnt:%d\n", nMatchCnt );
	if ( nMatchCnt <  4 ) return;

	
	nMatchCnt=0;
	
	uint32_t nLastTimeStamp = 0;
	for ( int i = 0; i < 5; i++ )
	{
		if ( m_rtpheader[i].timestamp < nLastTimeStamp )
		{
			//return;
		}
		else
		{
			nMatchCnt++;
		}
		
		nLastTimeStamp = m_rtpheader[i].timestamp;
	}
	
	//printf( "sequence match cnt:%d\n", nMatchCnt );
	if ( nMatchCnt <  4 ) return;
	nMatchCnt = 0;
	uint32_t nLastSSRC = 0;
	nLastSSRC = m_rtpheader[0].ssrc;
	for ( int i = 0; i < 5; i++ )
	{
		if ( nLastSSRC == 0 ) continue;
		if ( m_rtpheader[i].ssrc != nLastSSRC )
		{
			
		}
		else
		{
			nMatchCnt++;
		}
		
		nLastSSRC = m_rtpheader[i].ssrc;
	}
	//printf( "ssrc match cnt:%d\n", nMatchCnt );
	if ( nMatchCnt <  4 ) return;

	nRtpTailIndex = sizeof( RTPHeader)  + header.csrccount * 4 ;
	
	if ( nRtpTailIndex > nDataLen ) 
	{
		nRtpTailIndex = 0;
	}

	m_nRtpFlag = 1;
/*
	printf("rtp ver:%d pad:%d ext:%d marker:%d pt:%d ts:%u cc:%d ssrc:%u\n", 
		header.version, 
		header.padding,
		header.extension,
		header.marker,
		header.payloadtype,
		header.timestamp, 
		header.csrccount, 
		header.ssrc );
		*/
	return ;
}


void *CUdpSource::RecvProc( void *arg )
{
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);		   //允许退出线程	
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,	 NULL);   //设置立即取消  

	CUdpSource *pThis = (CUdpSource*)arg;
	unsigned char *pbuf = (unsigned char*)malloc( 1024 * 1024 );
	int nMaxSize = 1024*1024;
	int nRtpTailPos = 0;

    //FILE* fp=fopen("/opt/dws/video_remove_rtp_header", "wb");
	while( pThis->m_iExit == 0 )
	{
		int len = pThis->m_pNetUdp->GetDataNT(pbuf, nMaxSize, 1 );
		//printf("udp source received:%d\n", len);
		if ( len < 0 ) 
		{
			break;
		}
		else if ( len > 0 )
		{
			if ( pThis->m_iExit != 0 ) break;
			nRtpTailPos = 0;
			pThis->RemoveRtpHeader( pbuf, len, nRtpTailPos );
			//_DEBUG_("nRtpTailPos = %d", nRtpTailPos);
			if ( pThis->m_funcOnRecvCB != NULL )
			{
                // 天津
                //pThis->m_funcOnRecvCB( pbuf+12, len-12, pThis->m_pCbObj );
                // 其他
				pThis->m_funcOnRecvCB( pbuf, len, pThis->m_pCbObj );
                //pThis->m_funcOnRecvCB( pbuf+nRtpTailPos, len-nRtpTailPos, pThis->m_pCbObj );
                //fwrite(pbuf+nRtpTailPos, 1, len-nRtpTailPos, fp);
			}
			//printf("callback return\n");
		}
		else
		{
			
			usleep(100);
		}
	};
	//fclose(fp);
	printf("recv proc exit\n");
	free( pbuf);
	//pthread_detach( pthread_self());
	return NULL;
}

void CUdpSource::start()
{
	
}

void CUdpSource::stop()
{
	m_iExit = 1;
	m_pNetUdp->Close();
}

