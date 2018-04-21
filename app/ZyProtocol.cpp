#include "ZyProtocol.h"

CZyProtocol::CZyProtocol( int nPort, CChannelManager *pChnManager )  : CProtocol( nPort, pChnManager )
{
	m_iExitFlag = 0;
	pthread_mutex_init( &m_lock, NULL );
	m_pServer = new CSyncNetServer( CZyProtocol::THWorker , (void*)this );
	m_pServer->StartServer( nPort );
}

CZyProtocol::~CZyProtocol()
{
	m_iExitFlag = 1;
	if ( m_pServer != NULL)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	pthread_mutex_destroy( &m_lock );
}

int CZyProtocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

void CZyProtocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	printf("client connect:%s\n", net->getRemoteAddr());
	ZYMsg msg, ack;

	CZyProtocol *pThis = ( CZyProtocol*)p;
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
	printf("client disconnect:%s\n", net->getRemoteAddr());
}

int CZyProtocol::ProcessCtrlRequest(CSyncServerNetEnd *net, ZYMsg &msg, ZYMsg &ack)
{
	int iRet = 0;
	switch( msg.head.bMsgType_h << 8 | msg.head.bMsgType_l )
	{
		case 0x400:
			iRet = ProcessGetSendInfo( msg, ack );
			break;
		case 0x401:
			iRet = ProcessStartSendInfo(msg, ack);
			break;
		case 0x403:
			iRet = ProcessStopSendInfo(msg, ack);
			break;
		case 0x0501:
			iRet = ProcessPtz( msg, ack );
			break;
		case 0x0202:
			iRet = ProcessSetCodecParam(msg, ack);
			break;
		case 0x0107:
			iRet = ProcessSetTime(msg,ack);
			break;
		default:
			break;
	}
	return iRet;
}


bool CZyProtocol::IsMulticastIP (unsigned long dwIP)
{
    unsigned char chMinClassD_IP[4] = {224,0,0,0};
    unsigned char chMaxClassD_IP[4] = {239,255,255,255};
    return (((unsigned char *) & dwIP) [0] >= chMinClassD_IP [0] &&
            ((unsigned char *) & dwIP) [0] <= chMaxClassD_IP [0]) ;
}

int CZyProtocol::ProcessGetSendInfo( ZYMsg msg, ZYMsg &ack )
{
	int iRet = 0;
	int iStreamNo = msg.body.streamctl.bStreamNo;
	
	int iChannelNo=0, iSubStreamNo=0;
	iChannelNo = iStreamNo >> 8; 
	iSubStreamNo = iStreamNo & 0xff;
	
	if ( iChannelNo > g_nMaxEncChn / 2)
	{
		_DEBUG_("not supported channelno:%d", iChannelNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 1 );
		return iRet;
	}

	if ( iSubStreamNo > 3 || iSubStreamNo  <= 0 ) 
	{
		_DEBUG_("not supported streamno:%d", iSubStreamNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 2 );
		return iRet;
	}


	EncNetParam encnet;
	m_pChnManager->GetEncNet( iChannelNo * 2 + iSubStreamNo-1, 0, encnet);
	
	memset( &ack, 0, sizeof( ack));
	BuildHead( ack.head );
	ack.head.bFlag = 0x02;
	ack.head.bMsgType_h = msg.head.bMsgType_h;
	ack.head.bMsgType_l = msg.head.bMsgType_l;
	ack.head.bLength_h = msg.head.bLength_h;
	ack.head.bLength_l = msg.head.bLength_l;
	FillHeadCheckSum(ack.head);
	ack.body.streamctl.bStreamNo = iStreamNo;
	ack.body.streamctl.bVideoFormat = 2;


	if ( encnet.iMux != 0 )
	{
		ack.body.streamctl.iHeadFlag = 2;
	}
	else
	{	
		ack.body.streamctl.iHeadFlag = 1;
	}


	switch( encnet.iNetType )
	{

		case (int)NetType_TCP:
		{
			ack.body.streamctl.iProtocol = (int)ZY_NET_TCP;
			break;
		}
		case (int)NetType_UDP:
		{
			if ( IsMulticastIP( inet_addr(encnet.szPlayAddress)))
			{
				ack.body.streamctl.iProtocol = (int)ZY_NET_MULTICAST;
				
			}
			else 
			{
				ack.body.streamctl.iProtocol = (int)ZY_NET_UDP;
			}
			break;
		}
		default:
			ack.body.streamctl.iProtocol = (int)ZY_NET_TCP;
			break;
		

	};

	ack.body.streamctl.iPeerIP = inet_addr( encnet.szPlayAddress);
	ack.body.streamctl.iPeerPort = encnet.iPlayPort;

	return ack.head.bLength_l;
}

int CZyProtocol::ProcessSetTime( ZYMsg msg, ZYMsg &ack )
{
	m_pChnManager->SetSysTime( 	msg.body.timeset.bYear + 2000, 
								msg.body.timeset.bMonth, 
								msg.body.timeset.bDay, 
								msg.body.timeset.bHour, 
								msg.body.timeset.bMinute, 
								msg.body.timeset.bSecond );


	BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 0 );
	return ack.head.bLength_l;
}


int CZyProtocol::ProcessPtz( ZYMsg msg, ZYMsg &ack )
{
	unsigned int uiAddressCode, uiAction, uiParam1 = 0;
	uiAddressCode = msg.body.ptz.iReserved & 0xff;
	uiAction = (msg.body.ptz.iReserved >> 8 & 0xff);
	uiParam1 = (msg.body.ptz.iReserved >> 16 & 0xff);

	_DEBUG_( "process ptz chan:%d, action:%d speed:%d", 
		uiAddressCode, 
		uiAction, 
		uiParam1 );
	
	if ( uiAddressCode <= 0 ) 
	{
		_DEBUG_("not supported channelno:%d", msg.body.ptz.iReserved );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 3 );
	    return 0;
	}
	m_pChnManager->Ptz( uiAddressCode-1, 
		uiAction, 
		uiParam1, 
		0 );

	BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 0 );
	return ack.head.bLength_l;
}

int CZyProtocol::ProcessStartSendInfo( ZYMsg msg, ZYMsg &ack )
{
	int iRet = 0;
	int iStreamNo = msg.body.streamctl.bStreamNo;

	struct in_addr in;
	in.s_addr = msg.body.streamctl.iPeerIP;

	_DEBUG_("receive start encoder info. streamno:0x%x format:0x%x headflag:0x%x ip:%s port:%d protol:%d", 
		msg.body.streamctl.bStreamNo,
		msg.body.streamctl.bVideoFormat, 
		msg.body.streamctl.iHeadFlag,
		inet_ntoa(in), 
		msg.body.streamctl.iPeerPort, 
		msg.body.streamctl.iProtocol );

	int iChannelNo=0, iSubStreamNo=0;
	iChannelNo = iStreamNo >> 8; 
	iSubStreamNo = iStreamNo & 0xff;

	if ( iChannelNo > g_nMaxEncChn / 2 )
	{
		_DEBUG_("not supported channelno:%d", iChannelNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 1 );
		return iRet;
	}

	if ( iSubStreamNo > 3 || iSubStreamNo  <= 0 ) 
	{
		_DEBUG_("not supported streamno:%d", iSubStreamNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 2 );
		return iRet;
	}

	EncNetParam net;
	net.iEnable = 1;
	net.iIndex = 0;
	switch( msg.body.streamctl.iProtocol )
	{
	case 1: //TCP
		net.iNetType = 	1;
		break;
	case 2://UDP
	case 3:
		net.iNetType = 0;
		break;
	default:
		break;
	};

	strcpy(  net.szPlayAddress, inet_ntoa(in));
	net.iPlayPort = msg.body.streamctl.iPeerPort;

	if ( msg.body.streamctl.iHeadFlag == 1 ) //es
		net.iMux = 0;
	else 
		net.iMux = 2;

	m_pChnManager->SetEncNet( iChannelNo * 2 + iSubStreamNo-1, net );
	BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 0 );
	return ack.head.bLength_l;
}

int CZyProtocol::ProcessSetCodecParam( ZYMsg msg, ZYMsg &ack )
{
	BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 0 );
	return msg.head.bMsgType_l;
}

int CZyProtocol::ProcessStopSendInfo( ZYMsg msg, ZYMsg &ack )
{
	int iRet = 0;
	int iStreamNo = msg.body.streamctl.bStreamNo;

	struct in_addr in;
	in.s_addr = msg.body.streamctl.iPeerIP;

	_DEBUG_("receive stop encoder info. streamno:0x%x format:0x%x headflag:0x%x ip:%s port:%d protol:%d", 
		msg.body.streamctl.bStreamNo,
		msg.body.streamctl.bVideoFormat, 
		msg.body.streamctl.iHeadFlag,
		inet_ntoa(in), 
		msg.body.streamctl.iPeerPort, 
		msg.body.streamctl.iProtocol );

	int iChannelNo=0, iSubStreamNo=0;
	iChannelNo = iStreamNo >> 8; 
	iSubStreamNo = iStreamNo & 0xff;

	if ( iChannelNo >= g_nMaxEncChn / 2)
	{
		_DEBUG_("not supported channelno:%d", iChannelNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 1 );
		return iRet;
	}

	if ( iSubStreamNo > 3 || iSubStreamNo  <= 0 ) 
	{
		_DEBUG_("not supported streamno:%d", iSubStreamNo );
		BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 2 );
		return iRet;
	}

	m_pChnManager->StopEncNet( iChannelNo * 2 + iSubStreamNo-1, 0 );
	BuildCmdResponse(  ack, msg.head.bMsgType_l, msg.head.bMsgType_h, 0 );
	return ack.head.bLength_l;
}



int CZyProtocol::SendMsg(CSyncNetEnd *net, ZYMsg &msg)
{
	_DEBUG_("-----------SENDTO:%s>>>>>>>>>>>>>>>>>>\n", net->getRemoteAddr());
	int length = msg.head.bLength_l + sizeof(msg.head);

	MSGHTON( msg );
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

long CZyProtocol::BuildHead(ZYHEAD& Head)
{
	memset(&Head, 0, sizeof(ZYHEAD));
	Head.bHead1 = 0xFF;
	Head.bHead2 = 0xFF;
	Head.bHead3 = 0x5A;
	Head.bHead4 = 0x59;
	Head.bHead5 = 0x44;
	Head.bHead6 = 0x5A;
	Head.bVersion = 0x01;
	return 0;
}

void CZyProtocol::FillHeadCheckSum( ZYHEAD &Head )
{
	Head .bChecksum = ((Head .bHead1 + Head .bHead2 + Head .bHead3 + Head .bHead4 + Head .bHead5 + Head .bHead6  + Head .bVersion + Head .bFlag \
			+ Head .bMsgType_h + Head .bMsgType_l + Head .bLength_h + Head .bLength_l) & 0xff);
}

long CZyProtocol::CheckHead( ZYHEAD Head)
{
	if (Head.bHead1 != 0xFF )
	{
		_DEBUG_( "MDCodec:CheckHead head1 is 0x%x", Head.bHead1);
		return -1;
	}

	if (Head.bHead2 != 0xFF)
	{
		_DEBUG_( "MDCodec:CheckHead head2 is 0x%x", Head.bHead2);
		return -1;
	}

	if ((Head.bVersion & 0xf) != 0x1)
	{		
		_DEBUG_( "MDCodec:CheckHead version is 0x%x", Head.bVersion);
		return -1;
	}

	if (((Head.bHead1 + Head.bHead2 + Head.bHead3 + Head.bHead4 + Head.bHead5 + Head.bHead6  + Head.bVersion + Head.bFlag \
		+ Head.bMsgType_h + Head.bMsgType_l + Head.bLength_h + Head.bLength_l) & 0xff) != Head.bChecksum)
	{
		_DEBUG_("MDCodec:CheckHead checksum is 0x%x", Head.bChecksum);
		return -1;
	}

	return 0;
}

long CZyProtocol::BuildCmdResponse( ZYMsg &ack, int msgType_l, int msgType_h, int nRet )
{
	memset( &ack, 0, sizeof( ack));
	BuildHead( ack.head );
    ack.head.bFlag = 0x03;
	ack.head.bMsgType_h = msgType_h;
	ack.head.bMsgType_l = msgType_l;
	ack.head.bLength_h = 0;
	ack.head.bLength_l = sizeof(ack.body.ret);
	FillHeadCheckSum(ack.head);

	ack.body.ret.iRet = nRet;
	return 0;
}


void CZyProtocol::MSGNTOH( ZYMsg &msg )
{
	switch( msg.head.bMsgType_h << 8 | msg.head.bMsgType_l)
	{
	case 0x400:
	case 0x401:
	case 0x403:	
		NTOHS( msg.body.streamctl.bStreamNo);
		NTOHS( msg.body.streamctl.bVideoFormat);
		NTOHL( msg.body.streamctl.iHeadFlag);
		NTOHL( msg.body.streamctl.iProtocol);
		//NTOHL( msg.body.streamctl.iPeerIP );
		NTOHL( msg.body.streamctl.iPeerPort );
		break;
	case 0x501:
		NTOHL( msg.body.ptz.iReserved );
		break;
	default:
		break;	
	};
}

void CZyProtocol::MSGHTON( ZYMsg &msg )
{
	switch( msg.head.bMsgType_h << 8 | msg.head.bMsgType_l)
	{
	case 0x400:
	case 0x401:
		HTONS( msg.body.streamctl.bStreamNo);
		HTONS( msg.body.streamctl.bVideoFormat);
		HTONL( msg.body.streamctl.iHeadFlag);
		HTONL( msg.body.streamctl.iProtocol);
		HTONL( msg.body.streamctl.iPeerPort );
		//HTONL( msg.body.streamctl.iPeerIP );
		break;
	default:
		break;	
	};
}

int CZyProtocol::GetMsg( CSyncNetEnd *net, ZYMsg &msg, int timeout)
{
	unsigned int len;
	char buf[1024]={0};

	memset(&msg, 0, sizeof(ZYMsg) );

	if ( timeout==0 )
		len = net->Recv( (void*)&msg, sizeof( ZYHEAD) );
	else
		len = net->RecvT( (void*)&msg, sizeof( ZYHEAD), timeout );

	if ( len!=sizeof( ZYHEAD) )
	{
		printf("Receive Header Error\n");
		return -1;
	}

	if ( CheckHead( msg.head ) < 0 )
	{
		printf("check Header Error\n");
		return -1;
	}

	unsigned int length = msg.head.bLength_l;
	if ( length>sizeof(msg.body)   )
	{
		printf("Receive Incorrect length=%x\n", length);
		return -1;
	}

	if ( timeout==0 )
		len = net->Recv( (void*) &msg.body, length );
	else
		len = net->RecvT( (void*) &msg.body, length, timeout);

	if ( len!=length )
	{
		printf("Receive Length:%d!=Respect_Length:%d\n", len, length);
		return -1;
	}

	printf( "recvmsg 0x%x.\n", msg.head.bMsgType_h << 8 | msg.head.bMsgType_l );
	MSGNTOH( msg);
	return msg.head.bMsgType_h << 8 | msg.head.bMsgType_l ;
}




