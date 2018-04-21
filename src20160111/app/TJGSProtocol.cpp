#include "TJGSProtocol.h"


CTJGSProtocol::CTJGSProtocol( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager ) 
{

	_DEBUG_("sip serverid:%s localip:%s port:%d", g_szSipServerId, g_szLocalIp, nPort );
    memset(m_szDevicId, 0, sizeof(m_szDevicId));
    m_iExitFlag = 0;
	if ( g_nDeviceType == (int)DeviceModel_Encoder ||
		g_nDeviceType == (int)DeviceModel_HDEncoder)
	{
        char szName[256] = {0};
        char szId[256] = {0};
        m_pChnManager->m_pEncSet->GetEncChannel(szName, szId, 0);
        strncpy(m_szDevicId, szId, strlen(szId));
	}
	else
	{
        struDecChannelInfo sDCI;
        memset(&sDCI, 0, sizeof(struDecChannelInfo));
        sDCI.channel = 1;
    	m_pChnManager->m_pDecSet->GetDecChannelInfo(&sDCI, (void*)m_pChnManager->m_pDecSet);
        strncpy(m_szDevicId, sDCI.code, strlen(sDCI.code));
	}
    

	m_pTJGSLib = new CTJGSLib( m_szDevicId, g_szLocalIp, nPort );
	strcpy( m_szServerIp, g_szSipServerIp);
	
	m_pTJGSLib->SetSipDeviceInfo(g_szSipServerIp, g_szSipServerId, nPort );
	m_pTJGSLib->SetEncoderStartCB((TJGS_EncoderStart)TJGSCB_LiveStart, (void*)this);
	m_pTJGSLib->SetDecoderStartCB((TJGS_DecoderStart)TJGSCB_DecoderStart, (void*)this);
	m_pTJGSLib->SetLiveEndCB((TJGS_LiveEnd)TJGSCB_LiveEnd, (void*)this);
	m_pTJGSLib->SetPTZCB( TJGSCB_PTZ, (void *)this);
	m_pTJGSLib->SetQueryDeviceStatusCB( TJGSCB_QueryDeviceStatus, (void*)this);
	m_pTJGSLib->SetDeviceControlCB( TJGSCB_AlarmControl, (void*)this );
	pthread_create( &m_th, NULL, WorkerProc, (void*)this );
	//m_pTJGSLib->RequestRegister(char * szPeerIp,char * szUserName,char * szPassWd)
}
void *CTJGSProtocol::WorkerProc( void *arg )
{
	CTJGSProtocol *pThis = ( CTJGSProtocol*)arg;
	while( pThis->m_iExitFlag == 0 )
	{
		
		sleep(10);
		printf("send heart beat to %s\n", pThis->m_szServerIp );
		pThis->m_pTJGSLib->RequestSendKeepalive(pThis->m_szServerIp);
	}
	return NULL;
}

int CTJGSProtocol::TJGSCB_AlarmControl(char* szPeerIp, 
									char* szDeviceId, 
									int nControlCmd, 
									void* pCBObj)
{
	_DEBUG_( "alarm  control %s %d", szDeviceId, nControlCmd );

	CTJGSProtocol *pThis = ( CTJGSProtocol*)pCBObj;
	switch( nControlCmd )
	{
	case 0:
		pThis->m_pChnManager->SetGuard( szDeviceId);
		break;
	case 1:
		pThis->m_pChnManager->ResetGuard(szDeviceId);
		break;
	case 2:
		pThis->m_pChnManager->ClearAlarm( szDeviceId );
		break;
	default:
		break;

	};
	
	return 0;
}

int CTJGSProtocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

int CTJGSProtocol::OnAlarm( char *szAlarmInId, int nAlarminId, int nStatus )
{

	_DEBUG_( "alarm %s %d %d", szAlarmInId, nAlarminId, nStatus );

	/*
	struct sTJGSAlarmParam
	{
		char szDeviceId[30];
		unsigned int uiAlarmTime;
		unsigned int uiAlarmPriority;//1为一级警情；2为二级警情；3为三级警情；4为四级警情。
		unsigned int uiAlarmType;//1为电话报警；2为设备报警；3为短信报警；4为GPS报警；5为视频报警；6为设备故障报警；7为闯关报警（默认值），8为其他报警。
	};
	*/
	sTJGSAlarmParam alarmParam;
	strncpy( alarmParam.szDeviceId, szAlarmInId, 30 );
	time_t tm;
	time(&tm);
	alarmParam.uiAlarmTime = tm;
	alarmParam.uiAlarmPriority = 1;
	alarmParam.uiAlarmType = 7;
	m_pTJGSLib->NoteDevAlarm( m_szServerIp, szAlarmInId, nStatus, alarmParam);
	return 0;
}


void CTJGSProtocol::Login( char *szServerIp, char *szUserName, char *szPasswd )
{
	strncpy( m_szServerIp, szServerIp, 50 );
	strncpy( m_szUserName, szUserName, 50 );
	strncpy( m_szPasswd, szPasswd, 50 );
	//m_pTJGSLib->RequestRegister( szServerIp, szUserName, szPasswd );
}

CTJGSProtocol::~CTJGSProtocol()
{
	PTR_DELETE( m_pTJGSLib);
}

int CTJGSProtocol::TJGSCB_PTZ(char* szPeerIp, 
									char* szDeviceId, 
									int ePtzType, 
									int nSpeed, 
									void* pCBObj)
{
	_DEBUG_("decoder start: peerip:%s deviceid:%s ptztype:%d speed:%d", 
		szPeerIp, 
		szDeviceId, 
		ePtzType, 
		nSpeed );

	CTJGSProtocol *pThis = ( CTJGSProtocol *)pCBObj;
	pThis->m_pChnManager->Ptz( szDeviceId, ePtzType, nSpeed, 0 );
	return 0;

}


int CTJGSProtocol::TJGSCB_QueryDeviceStatus(char* szPeerIp, 
													char* szDeviceId, 
													sTJGSDeviceStatus *pDevStatus, 
													void* pCBObj)
{
	_DEBUG_("CB28181_QueryDeviceStatus PeerIP:%s, szDeviceId:%s \n", szPeerIp, szDeviceId);

	FILE *file = fopen("/proc/uptime", "r" );
	pDevStatus->uiSinceStartup = 0;

	char szContent[256]={0};
	int nRead = fread(szContent, 256, 1, file );
	
	printf("content:%s\n", szContent );
	int argc = 0;
	char *argv[100];
	argv[ argc ] = strtok( szContent, " " );
	while ( argv[argc]!=NULL )
	{
		if ( argc > 90 ) break;
		argv[ ++argc ] = strtok( NULL, " " );
	}

	if ( argc > 0 )
	{
		pDevStatus->uiSinceStartup = atoi( argv[0] );
	}
	fclose(file);

	pDevStatus->uiErrChannelNum = 0;
	pDevStatus->uiOnline = 1;
	pDevStatus->uiRecord = 0;
	pDevStatus->uiStatus = 0;
	return 0;
}


int CTJGSProtocol::TJGSCB_DecoderStart(char *szPeerIp, 
								char *szDeviceId, 
								char *szDestIp, 
								int uiDestPort, 
								char *szCastType, 
								int nLiveHandle,
								void *pCBObj )
{
	_DEBUG_("decoder start: peerip:%s deviceid:%s ip:%s port:%d cast:%s", 
		szPeerIp, 
		szDeviceId, 
		szDestIp, 
		uiDestPort, 
		szCastType );

	if ( pCBObj == NULL )
	{
		_DEBUG_("cbobj == NULL");
		return -1;
	}
	
	CTJGSProtocol *pThis = ( CTJGSProtocol *)pCBObj;

	char szUrl[256]={0};

	if ( strcmp( szCastType, "UDP" ) == 0 )
	{
		
		sprintf( szUrl, "udptjgs/%s:%d", szDestIp, uiDestPort );
		
	}
	else if ( strcmp( szCastType,"TCP") == 0 )
	{
		sprintf( szUrl, "tcp/%s:%d", szDestIp, uiDestPort );
	}
	
	pThis->m_pChnManager->Open( 1, szUrl, 0, 25 );
	
	return 0;
}


//实时点播
int CTJGSProtocol::TJGSCB_LiveStart(char *szPeerIp, 
									 char *szDeviceId, 
									 char *szDestIp, 
									 int uiDestPort, 
									 int nStreamNo, 
									 char *szResolution, 
									 char *nCastType, 
									 int nBitRate,
									 int nFrameRate, 
									 char *szGopM, 
									 int nGopN,
									 char *szAudio,
									 int nLiveHandle,
									 void* pCBObj)

{
	//printf("GBTJGS_QueryRecordInfo PeerIP:%s, sDeviceId:%s, szDestIp:%s, szDestPort:%s, nLiveHandle:%d \n", szPeerIp, szDeviceId, szDestIp, szDestPort, nLiveHandle);
	_DEBUG_( "live start PeerIp:%s id:%s dest:%s port:%d, stream:%d res:%s cast:%s br:%d fps:%d gopm:%s gopn:%d audio:%s", 
	      szPeerIp, 
	      szDeviceId, 
	      szDestIp, 
	      uiDestPort, 
	      nStreamNo, 
	      szResolution, 
	      nCastType, 
	      nBitRate,
	      nFrameRate,
	      szGopM, 
	      nGopN,
	      szAudio );

	if ( pCBObj == NULL )
	{
		_DEBUG_("cbobj == NULL");
		return -1;
	}
	CTJGSProtocol *pThis = ( CTJGSProtocol *)pCBObj;
	EncoderParam encparam;
	memset(&encparam, 0, sizeof(EncoderParam));


	if ( strcmp( szResolution, "D1" ) ==  0 )
	{
		encparam.iResolution = ResolutionType_D1;
	}
	else if ( strcmp( szResolution, "CIF" ) ==  0 )
	{
		encparam.iResolution = ResolutionType_CIF;
	}
	else if ( strcmp( szResolution, "4CIF" ) ==  0 )
	{
		encparam.iResolution = ResolutionType_D1;
	}
	else if ( strcmp( szResolution, "720P" ) ==  0 )
	{
		encparam.iResolution = ResolutionType_720P;
	}
	else if ( strcmp( szResolution, "1080P" ) ==  0 )
	{
		encparam.iResolution = ResolutionType_1080P;
	}
	else 
	{
		encparam.iResolution = ResolutionType_D1;
	}

	if ( nGopN <= 0 || nGopN > 100 )
	{
		nGopN = 10;
	}

	if ( nBitRate < 32 || nBitRate > 20480000 )
	{
		nBitRate = 1500;
	}

	encparam.iProfile = 2;
	encparam.iGop = nGopN>0?nGopN:10;
	encparam.iFPS = nFrameRate;
	encparam.iBitRate = nBitRate;
	encparam.iCbr = 0;
	
	
	EncNetParam net;
	memset(&net, 0, sizeof(net));
	strcpy( net.szPlayAddress, szDestIp );
	net.iPlayPort = uiDestPort;
	net.iEnable = 1;
	net.iIndex = 0;

	if ( strcmp( nCastType, "UDP" ) == 0 )
	{
		net.iNetType = 0; //udp
		net.iMux = 2;
	}
	else if ( strcmp( nCastType,"TCP") == 0 )
	{
		net.iNetType = 1;
		net.iMux = 2;
	}

	int nChannel = pThis->m_pChnManager->SetEncParam( pThis->m_szDevicId, nStreamNo, encparam );
	nChannel = pThis->m_pChnManager->SetEncNet( pThis->m_szDevicId, nStreamNo, net );

	if ( nLiveHandle > 0 )
	{
		if ( pThis->m_mapEncLiveHandle.count(nLiveHandle) > 0 )
		{
		  	pThis->m_mapEncLiveHandle[nLiveHandle] = nChannel;
		}

	}
	return 0;
}

//关闭实时点播
int CTJGSProtocol::TJGSCB_LiveEnd(char* szPeerIp, 
										 char* szDeviceId, 
										 int nLiveHandle,
										 void* pCBObj)

{
	_DEBUG_("TJGSCB_LiveEnd PeerIP:%s, szDeviceId:%s, nLiveHandle:%d \n", 
				szPeerIp, 
				szDeviceId, 
				nLiveHandle);

	if ( nLiveHandle > 0 )
	{
		CTJGSProtocol *pThis = ( CTJGSProtocol *)pCBObj;
		if ( g_nDeviceType == (int)DeviceModel_Encoder ||
			g_nDeviceType == (int)DeviceModel_HDEncoder)
		{
			if ( pThis->m_mapEncLiveHandle.count(nLiveHandle) > 0 )
			{
				int nChannel = pThis->m_mapEncLiveHandle[nLiveHandle];
				EncNetParam net;
				memset(&net, 0, sizeof(net));
				pThis->m_pChnManager->SetEncNet( nChannel, net );
			  	pThis->m_mapEncLiveHandle.erase(nLiveHandle);
			}
		}
        else
        {
			pThis->m_pChnManager->Close( 1 );
        }
	}
	return 0;
}


