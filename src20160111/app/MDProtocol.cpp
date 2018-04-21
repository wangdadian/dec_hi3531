
#include "MDProtocol.h"
#include <string.h>


CMDProtocol::CMDProtocol( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager )
{
	for (int i = 0; i < MAXDECCHANN; i++)
	{
		memset(&m_decparam[i], 0, sizeof(COMDECODERPARAM));
		memset(&m_decexparam[i], 0, sizeof(COMDECODERPARAM_EX));
		
	}

	m_iExitFlag = 0;
	pthread_mutex_init( &m_lock, NULL );
	m_pServer = new CSyncNetServer( CMDProtocol::THWorker , (void*)this );
	m_pServer->StartServer( nPort );
}

CMDProtocol::~CMDProtocol()
{
	m_iExitFlag = 1;
	if ( m_pServer != NULL)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	pthread_mutex_destroy( &m_lock );
}

int CMDProtocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

int CMDProtocol::SendMsg(CSyncNetEnd *net, MDMsg &msg)
{
	//_DEBUG_("-----------SENDTO:%s>>>>>>>>>>>>>>>>>>\n", net->getRemoteAddr());
	int length = msg.head.bLength_l + sizeof(msg.head);
	try
	{
		net->Send( (void*)&msg, length);
		net->Flush();
		return 1;
	}
	catch(...)
	{
	    _DEBUG_("Not Send!!!");
		return 0;
	}

}

long CMDProtocol::CheckHead(VMUX6KHEAD Head)
{
	if (Head.bHead1 != 0xBF)
	{
		printf( "CheckHead head1 is 0x%x", Head.bHead1);
		return -1;
	}

	if (Head.bHead2 != 0xEC)
	{
		printf( "CheckHead head2 is 0x%x", Head.bHead2);
		return -1;
	}

	if ((Head.bVersion & 0xf) != 0x1)
	{		
		printf( "CheckHead version is 0x%x", Head.bVersion);
		return -1;
	}


	if (((Head.bHead1 + Head.bHead2 + Head.bVersion + Head.bFlag \
		+ Head.bMsgType + Head.bLength_h + Head.bLength_l) & 0xff) != Head.bChecksum)
	{
		printf( "CheckHead checksum is 0x%x", Head.bChecksum);
		return -1;
	}
	return 0;
}


long CMDProtocol::BuildHead(VMUX6KHEAD& Head)
{
	memset(&Head, 0, sizeof(VMUX6KHEAD));
	Head.bHead1 = 0xBF;
	Head.bHead2 = 0xEC;
	Head.bVersion = 0x01;
	return 0;
}

long CMDProtocol::BuildCmdResponse( MDMsg &ack, int msgType, int nRet )
{
	BuildHead( ack.head );
    ack.head.bFlag = 0x10;
	ack.head.bMsgType = msgType;
    /*
	ack.head.bLength_l = sizeof(COMDECODERPARAM);
	ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
		+ ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);
	ack.body.iAck = htonl(nRet);
	*/
	return 0;

}

long CMDProtocol::BuildDecoderParamResponse( MDMsg msg, MDMsg &ack )
{	
	pthread_mutex_lock( &m_lock );
	int nChannel = msg.body.decparam.iSlot;

	/*
	BuildHead( ack.head );
	ack.head.bMsgType = 0x04;
	ack.head.bLength_l = sizeof(COMDECODERPARAM);
	ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
		+ ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);*/
	/*
	if ( msg.body.decparam.iSlot <= 0 || nChannel > MAXDECCHANN )
	{
		printf( "error: invalid channel no %d should be [%d,%d]", 
			nChannel, 
			1,
			MAXDECCHANN );
		pthread_mutex_unlock( &m_lock );

		
		return 0;
	}
	*/
    ack.head.bLength_l = sizeof(COMDECODERPARAM);
	ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
		+ ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);
    
	_DEBUG_( " Channel no %d, but should be [1-%d]", 
			nChannel, MAXDECCHANN );
    if( nChannel <= 0 || nChannel > MAXDECCHANN)
    {
        nChannel = 1;
    }
	memcpy( &ack.body.decparam, &m_decparam[nChannel-1], sizeof(COMDECODERPARAM));
	pthread_mutex_unlock( &m_lock );
	return 1;
}

int CMDProtocol::ProcessCtrlRequest(CSyncServerNetEnd *net, MDMsg &msg, MDMsg &ack)
{
	BuildCmdResponse(ack, msg.head.bMsgType, 0 );

	int nRet = 1;
	switch( msg.head.bMsgType )
	{
	case 5:
		{
			//ProcessGetMonitorMode( msg, ack );
			nRet = ProcessStartDecoder( msg, ack );
			break;
		}
	case 7:
		{
			ProcessShowOSD( msg, ack );
			break;
		}
	case 4:
		{
			nRet = BuildDecoderParamResponse(msg,  ack);
			break;
		}
    case 8:
        {
            ProcessDisplay(msg, ack);
        }
	default:
		//return 0;
		break;
	};

	return nRet;
};


int CMDProtocol::ProcessShowOSD(  MDMsg &msg, MDMsg &ack)
{

	pthread_mutex_lock(&m_lock);
    
	memcpy((void*)&ack.body.osd, (void*)&msg.body.osd, sizeof(DecodecOsdParam));
    ack.body.response.iRet = 0;
	ack.head.bLength_l = sizeof(DecodecOsdParam);
	ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
		+ ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);
    

	int nChannel = msg.body.osd.channel;

    DecodecOsdParam* pOsd = &msg.body.osd;
    DecodecOsdParam sOsd;
    sOsd.b_showfont = pOsd->b_showfont;
    sOsd.b_showtime = pOsd->b_showtime;
    strcpy(sOsd.buffer_content, pOsd->buffer_content);
    sOsd.channel = pOsd->channel;
    sOsd.osd = ntohl(pOsd->osd);
    sOsd.slot_number = ntohs(pOsd->slot_number);
    sOsd.stream_id = pOsd->stream_id;
    sOsd.x_font = ntohl(pOsd->x_font);
    sOsd.y_font = ntohl(pOsd->y_font);
    sOsd.x_time = ntohl(pOsd->x_time);
    sOsd.y_time = ntohl(pOsd->y_time);
    sOsd.reserved = pOsd->reserved;
    m_pChnManager->SetDecOsd(nChannel, (void *)pOsd);
	pthread_mutex_unlock(&m_lock);

	return 0;
}

int CMDProtocol::ProcessDisplay(MDMsg &msg, MDMsg &ack)
{
    pthread_mutex_lock(&m_lock);

    memcpy((void*)&ack.body.display, (void*)&msg.body.display, sizeof(tDecDisplayParam));
    ack.body.response.iRet = 0;
    ack.head.bLength_l = sizeof(tDecDisplayParam);
    ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
        + ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);

    tDecDisplayParam* pdisplay = &msg.body.display;
    tDecDisplayParam obj;
    memcpy(&obj, pdisplay, sizeof(tDecDisplayParam));
    obj.iChannel = ntohl(obj.iChannel);
    obj.iDisplaymode = ntohl(obj.iDisplaymode);
    obj.iDisplayno = ntohl(obj.iDisplayno);
    obj.iResolution = ntohl(obj.iResolution);
    
    m_pChnManager->SetDecDisplay((void *)&obj);
    pthread_mutex_unlock(&m_lock);

    return 0;
}

int CMDProtocol::ProcessStartDecoder(  MDMsg &msg, MDMsg &ack)
{

	pthread_mutex_lock( &m_lock );
	int nChannel = msg.body.decparam.iSlot;

	/*if ( nChannel <= 0 || nChannel > MAXDECCHANN )
	{
		printf( "error: invalid channel no %u should be [%d,%d]", 
			nChannel, 
			1,
			MAXDECCHANN );
		pthread_mutex_unlock( &m_lock );
		return 0;
	}*/
	memcpy((void*)&ack.body.response, (void*)&msg.body.response, sizeof(CMDRESPONSE));
    ack.body.response.iRet = 0;
	ack.head.bLength_l = sizeof(CMDRESPONSE);
	ack.head.bChecksum = ((ack.head.bHead1 + ack.head.bHead2 + ack.head.bVersion + ack.head.bFlag \
		+ ack.head.bMsgType + ack.head.bLength_h + ack.head.bLength_l) & 0xff);
	_DEBUG_( "channel number %u", nChannel);
    if(nChannel<=0 || nChannel>MAXDECCHANN)
    {
        nChannel = 1;
    }
    int iFps=msg.body.decparam.uiEncodecType;//¸´ÓÃÎªfps
    //int iFps=30;
    
	if ( NET_UDP == msg.body.decparam.bProtocol ||
		NET_MULTICAST == msg.body.decparam.bProtocol )
	{
	    //_DEBUG_("Net Protocol UDP or MULTICAST [%d]!", msg.body.decparam.bProtocol);
		char szUrl[200]={0};
		struct in_addr in;
		in.s_addr = msg.body.decparam.uiPeerIP;
		sprintf( szUrl, "udp/%s:%d", inet_ntoa(in), msg.body.decparam.uiPeerPort );
		//_DEBUG_( "start decoder chann:%d url:%s\n", nChannel, szUrl);
		m_pChnManager->SaveUrl(nChannel, szUrl, iFps);
		m_pChnManager->Open( nChannel, szUrl, msg.body.decparam.uiStreamType, iFps);
		memcpy( &m_decparam[nChannel-1], &msg.body.decparam , sizeof(COMDECODERPARAM));
	}
	else if ( NET_TCP == msg.body.decparam.bProtocol )
	{
	    //_DEBUG_("Net Protocol TCP [%d]!", msg.body.decparam.bProtocol);
		char szUrl[200]={0};
		struct in_addr in;
		in.s_addr = msg.body.decparam.uiPeerIP;
		sprintf( szUrl, "tcp/%s:%d", inet_ntoa(in), msg.body.decparam.uiPeerPort );
		//_DEBUG_( "start decoder chann:%d url:%s\n", nChannel, szUrl);
		
		m_pChnManager->SaveUrl(nChannel, szUrl, iFps);
		m_pChnManager->Open( nChannel, szUrl, msg.body.decparam.uiStreamType, 
			iFps );
		memcpy( &m_decparam[nChannel-1], &msg.body.decparam , sizeof(COMDECODERPARAM));
	}
    else if ( NET_URL == msg.body.decparam.bProtocol )
    {
		//_DEBUG_("Net Protocol URL [%d]!", msg.body.decexparam.DecoderParam.bProtocol);
        char szUrl[200]={0};
		strcpy( szUrl, msg.body.decexparam.szRTSPUrl );		
		m_pChnManager->SaveUrl(nChannel, szUrl, iFps);
		m_pChnManager->Open( nChannel, szUrl, msg.body.decexparam.DecoderParam.uiStreamType, iFps);
		memcpy( &m_decexparam[nChannel-1], &msg.body.decexparam , sizeof(COMDECODERPARAM_EX));
	}
    else if ( NET_CLOSE == msg.body.decparam.bProtocol )
    {
		_DEBUG_("Stop Decoder [%d]!", nChannel);
        m_pChnManager->Close(nChannel);
	}
	else
    {
        //_DEBUG_("Net Protocol Unknown [%d]!", msg.body.decparam.bProtocol);
        char szUrl[200]={0};
		struct in_addr in;
		in.s_addr = msg.body.decparam.uiPeerIP;
		sprintf( szUrl, "tcp/%s:%d", inet_ntoa(in), msg.body.decparam.uiPeerPort );
		//_DEBUG_( "start decoder chann:%d url:%s\n", nChannel, szUrl);
		
		m_pChnManager->SaveUrl(nChannel, szUrl, iFps);
		m_pChnManager->Open( nChannel, szUrl, msg.body.decparam.uiStreamType, iFps );
		memcpy( &m_decparam[nChannel-1], &msg.body.decparam , sizeof(COMDECODERPARAM));
    }
    m_decparam[nChannel-1].uiEncodecType = iFps;
    m_pChnManager->SetMonitorId(nChannel, msg.body.decparam.usDecodecScreenId);
	pthread_mutex_unlock( &m_lock );
	
	return 1;
}


void CMDProtocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s\n", net->getRemoteAddr());
	MDMsg msg, ack;
	CMDProtocol *pThis = ( CMDProtocol*)p;
	while( pThis->m_iExitFlag == 0 )
	{
		if ( !net->IsOK() ) 
        {
            _DEBUG_("net not ok!");
            break;
        }
		if ( 0>=pThis->GetMsg(net, msg, 0) )
        {
            _DEBUG_("get msg failed!");
            break;
        }
		if ( 0>=pThis->ProcessCtrlRequest( net, msg, ack ) )
        {
            _DEBUG_("process ctrl request failed!");
            break;
        }
		pThis->SendMsg( net, ack );
	
	};

	_DEBUG_("client disconnect:%s\n", net->getRemoteAddr());

}

void CMDProtocol::MSGNTOH( MDMsg &msg )
{
	switch( msg.head.bMsgType)
	{
	case 5:
		NTOHS( msg.body.decparam.iSlot );
		NTOHS( msg.body.decparam.uiResolutionH );
		NTOHS( msg.body.decparam.uiResolutionV );
		//NTOHL( msg.body.decparam.uiPeerIP );
		NTOHL( msg.body.decparam.uiPeerPort );
		NTOHL( msg.body.decparam.uiEncodecType );
		NTOHL( msg.body.decparam.uiStreamType );
		NTOHL( msg.body.decparam.uiDvrChannel );
		NTOHS( msg.body.decparam.usDecodecScreenId );
		NTOHS( msg.body.decparam.usDecodecChannel );
		break;
	case 7:
		NTOHS( msg.body.osd.slot_number );
		NTOHL( msg.body.osd.osd );
		NTOHL( msg.body.osd.x_time );
		NTOHL( msg.body.osd.y_time );
		NTOHL( msg.body.osd.x_font );
		NTOHL( msg.body.osd.y_font );
		break;
	default:
		break;	
	};
}


int CMDProtocol::GetMsg( CSyncNetEnd *net, MDMsg &msg, int timeout)
{
	unsigned int len;
	char buf[1024]={0};

	memset(&msg, 0, sizeof(MDMsg) );

	if ( timeout==0 )
		len = net->Recv( (void*)&msg, sizeof( VMUX6KHEAD) );
	else
		len = net->RecvT( (void*)&msg, sizeof( VMUX6KHEAD), timeout );

	if ( len!=sizeof( VMUX6KHEAD) )
	{
		_DEBUG_("Receive Header Error\n");
		return -1;
	}


	if ( CheckHead( msg.head ) < 0 )
	{
		_DEBUG_("check Header Error\n");
		return -1;
	}

	unsigned int length = msg.head.bLength_h*256+msg.head.bLength_l;
	if ( length>sizeof(MDMsg) || length<sizeof(VMUX6KHEAD) )
	{
		_DEBUG_("Receive Incorrect length=%x\n", length);
		return -1;
	}

	if ( timeout==0 )
		len = net->Recv( (void*) &msg.body, length );
	else
		len = net->RecvT( (void*) &msg.body, length, timeout);

	if ( len!=length )
	{
		_DEBUG_("Receive Length:%d!=Respect_Length:%d\n", len, length);
		return -1;
	}

	//_DEBUG_( "recvmsg %d.\n", msg.head.bMsgType);

	MSGNTOH( msg );
	return msg.head.bMsgType;
}






