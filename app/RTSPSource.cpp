
#include "RTSPSource.h"

CRTSPSource::CRTSPSource( char *szUrl ) : CSource( szUrl )
{
	for ( int i = 0; i < 16; i++ )
	{
		m_strUrl[i] = "";
		m_pRtspClient[i] = NULL;
		m_tmLastData[i] = time(&m_tmLastData[i]);
	}

	m_strUrl[0] =  szUrl;

	m_iExitFlag = 0;
	StartRtspSend(1);

	pthread_create( &m_th, NULL, CheckConnectTh, (void*)this);
	
}


CRTSPSource::~CRTSPSource(  )
{
	m_iExitFlag = 1;
	if (m_th != 0 )
	{
		pthread_join( m_th, NULL);
	}

	for ( int i = 0; i < 16; i++)
	{
		if ( m_pRtspClient[i] != NULL ) 
		{
			delete m_pRtspClient[i];
            m_pRtspClient[i] = NULL;
		}
	}
}

void *CRTSPSource::CheckConnectTh( void * arg )
{

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);		   //允许退出线程	
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,	 NULL);   //设置立即取消  


	CRTSPSource *pThis = ( CRTSPSource*)arg;
	while( pThis->m_iExitFlag == 0 )
	{
		pThis->CheckConnect();
		usleep( 100*1000);
	}
	return NULL;

}

bool CRTSPSource::CheckOnvifUrlInvalid( char * strUrl )
{
	if ( strUrl == NULL ) return false;
	int nIP1, nIP2, nIP3, nIP4, nPort;

	int ret = sscanf( strUrl, "%d.%d.%d.%d/", 
			&nIP1, 
			&nIP2, 
			&nIP3, 
			&nIP4 );

	if ( ret == 4 ) return true;


	ret = sscanf( strUrl, "%d.%d.%d.%d:%d/", 
			&nIP1, 
			&nIP2, 
			&nIP3, 
			&nIP4,
			&nPort );
	
	if ( ret == 5 ) return true;

	return false;
	
}



void  CRTSPSource::CheckConnect()
{
	time_t tmnow;
	time(&tmnow );

	for ( int i = 0; i < 16; i++ )
	{
		if ( m_pRtspClient[i] != NULL )
		{
			if ( tmnow - m_tmLastData[i] >= 10 )
			{
				printf( "no stream data more than 10 sedonds. reconnect...\n" );
				//reconnect 
				//m_strUrl[i] = "";
				StartRtspSend( i+1 );
				m_tmLastData[i] = tmnow;
			}
		}
	}
}

int CRTSPSource::rtspDataCallBack(void *pInputParam, 
								char *streamURL, 
								char* mediumName, 
								unsigned char* pbData, 
								int nDataSize, 
								struct timeval presentationTime )
{
	if ( nDataSize <= 2 ) return 0;
	//printf("recv rtsp:%d\n", nDataSize );
	CRTSPSource *pThis = (CRTSPSource*)pInputParam;
	pThis->SendRtspStream( streamURL, pbData, nDataSize );
	return 0;
}

bool CRTSPSource::ParseRTSPUrl( string streamUri, char *szUrlIP, int &nRtspPort, char *szSurfix )
{

	char szUrl[1024]={0};
	strcpy(szUrl, (char*)streamUri.c_str());
	char *szTobeParse=NULL;
	char szUserName[256]={0};
	char szPasswd[256]={0};
	
	int nWithUserPass = 0;

	ParseRtspUserAndPasswd( szUrl, szUserName, szPasswd );
	if ( ( szTobeParse = strstr( szUrl, "@" )) != NULL )
	{
		szTobeParse++;
		nWithUserPass = 1;
	}
	else 
	{
		if ( (szTobeParse = strstr( szUrl, "rtsp://" ))  == NULL )
		{
			return false;
		}
		else
		{
			szTobeParse += strlen("rtsp://");
		}

	}
	
	nRtspPort = 554;
	int nIP1, nIP2, nIP3, nIP4=0;

	int ret = sscanf( szTobeParse, "%d.%d.%d.%d:%d/", 
		&nIP1, 
		&nIP2, 
		&nIP3, 
		&nIP4, 
		&nRtspPort );

	if ( ret != 5 )
	{
		ret = sscanf( szTobeParse, "%d.%d.%d.%d/", 
			&nIP1, 
			&nIP2, 
			&nIP3, 
			&nIP4 );
		if ( ret != 4 )
		{
			printf("格式不正确, %s", streamUri.c_str() );
			return false;
		}
	}
	else
	{

	}

	sprintf( szUrlIP,  "%d.%d.%d.%d", nIP1, nIP2, nIP3, nIP4 );
	
	char szAddr[100]={0};
	if ( ret == 5 )
	{

		if ( nWithUserPass > 0 )
	   {
			sprintf( szAddr, "rtsp://%s:%s@%s:%d/", szUserName, szPasswd, szUrlIP, nRtspPort );
	   }
	   else 
	   {
		 sprintf( szAddr, "rtsp://%s:%d/", szUrlIP, nRtspPort);
	   }
	}
	else
	{
		if ( nWithUserPass > 0 )
	   {
			sprintf( szAddr, "rtsp://%s:%s@%s/", szUserName, szPasswd, szUrlIP );
	   }
	   else 
	   { 
	   		sprintf( szAddr, "rtsp://%s/", szUrlIP );

	   }
	}
	char szStreamUri[100]={0};
	strcpy( szStreamUri, streamUri.c_str() );

	int nRemaining = strlen( szStreamUri)-strlen(szAddr) ;
	if ( nRemaining > 0 )
	{
		memmove( szSurfix, szStreamUri+strlen(szAddr), nRemaining );
	}

	return true;

}


void CRTSPSource::SendRtspStream( char *streamURL, 
								unsigned char* pbData, 
								int nDataSize )
{
	
	for ( int i = 0; i < 16; i++ )
	{
		//printf( "10\n");
		string streamUriOrig = m_strUrl[i] ;	
		char szUrlIPOrig[100]={0};
		int nRtspPortOrig = 554;
		char szSurfixOrig[100]={0};

		if ( !ParseRTSPUrl(  streamUriOrig, szUrlIPOrig, nRtspPortOrig, szSurfixOrig ) )
		{
			printf("parse rtsp url failed :%s\n", streamUriOrig.c_str() );
			continue;
		}
		string streamUri = streamURL ;	
		char szUrlIP[100]={0};
		int nRtspPort = 554;
		char szSurfix[100]={0};

		if ( !ParseRTSPUrl(  streamUri, szUrlIP, nRtspPort, szSurfix ) )
		{
			printf("parse rtsp url failed :%s\n", streamUri.c_str() );
			continue;
		}

		if ( (strcmp( szUrlIP, szUrlIPOrig) == 0 ) &&
			( nRtspPort == nRtspPortOrig ) &&
			( strcasestr( szSurfixOrig,  szSurfix ) != NULL ||
			strcasestr( szSurfix, szSurfixOrig ) != NULL ) )
		{
			//printf( "update receive time url:%s  request url:%s\n", streamURL, (char*)m_strUrl[i].c_str()  );
			m_tmLastData[i] = time( &m_tmLastData[i] );
			
			

			memmove( pbData+4, pbData, nDataSize );
		    memcpy( pbData, (char*)"\0\0\0\1", 4  );


			if ( m_funcOnRecvCB != NULL )
			{
			   //printf( "handle:0x%x\n", (long)m_pCbObj);
				m_funcOnRecvCB( (unsigned char*)pbData, nDataSize+4 , m_pCbObj );
			}
			break;

		}
		//printf( " cannot find stream.\n ");	
		
	}

	
}


long CRTSPSource::ParseRtspUserAndPasswd( char *szUrl, char *szUserName, char *szPasswd )
{

    char szTmp[256]={0};
	strcpy( szTmp, szUrl );
	
	char *szTmp1 = strstr( szTmp, "@" );

	if ( szTmp1 == NULL)
	{
		return -1;
	}
	*szTmp1 = 0;


	char *szTmp2 = szTmp + strlen("rtsp://" );
	
	if ( szTmp2 == NULL)
	{
		printf("invalid rtsp url %s", szUrl);
		return -1;
	}

	char *szTmp3 = strstr( szTmp2, ":" );

	if ( szTmp3 == NULL)
	{
		return -1;

	}

	memcpy( szUserName, szTmp2, szTmp3 - szTmp2 );
	strcpy( szPasswd, szTmp3+1 ); 
	return 0;

}


long CRTSPSource::StartRtspSend( unsigned int uiStreamNo )
{


	string streamUri = m_strUrl[uiStreamNo-1] ;	
	
	
	char szUrlIP[100]={0};
	int nRtspPort = 554;
	int nIP1, nIP2, nIP3, nIP4=0;

	char szUserName[100]={0};
	char szPasswd[100] = {0};

		
	//streamUri = "rtsp://root:pass@192.168.1.121/stream0";

	printf("parse rtsp url.%s\n", (char*)streamUri.c_str());
	int nWithPasswd=0;
	if ( ParseRtspUserAndPasswd((char*)streamUri.c_str(), szUserName,szPasswd ) == 0 )
	{
		nWithPasswd = 1;
	}
	
	char szTmp[100]={0};

	char *szUrlScan;
	char *szUrlNoUser;
	strcpy( szTmp, streamUri.c_str());

	szUrlNoUser = strstr( szTmp, "@" );
	if ( szUrlNoUser != NULL )
	{
		szUrlScan = szUrlNoUser+1;
	}
	else 
	{
		szUrlScan =  strstr(szTmp, "rtsp://");
		if ( szUrlScan == NULL)
		{
			printf( "invalid rtsp　format:%s",szTmp );
			return -1;
		}
		szUrlScan = szTmp+strlen("rtsp://");
	}
	

	int ret = sscanf( szUrlScan, "%d.%d.%d.%d:%d/", 
		&nIP1, 
		&nIP2, 
		&nIP3, 
		&nIP4, 
		&nRtspPort );

	if ( ret != 5 )
	{
		ret = sscanf( szUrlScan, "%d.%d.%d.%d/", 
			&nIP1, 
			&nIP2, 
			&nIP3, 
			&nIP4 );
		if ( ret != 4 )
		{
			printf("格式不正确, %s", streamUri.c_str() );
			return -1;
		}
	}
	else
	{

	}

	sprintf( szUrlIP,  "%d.%d.%d.%d", nIP1, nIP2, nIP3, nIP4 );
	
	char szAddr[100]={0};
	if ( ret == 5 )
	{
	   if ( nWithPasswd > 0 )
	   {
			sprintf( szAddr, "rtsp://%s:%s@%s:%d/", szUserName, szPasswd, szUrlIP, nRtspPort );
	   }
	   else
	   {
		 	sprintf( szAddr, "rtsp://%s:%d/", szUrlIP, nRtspPort);
	   }
	}
	else
	{
	   if ( nWithPasswd > 0 )
	   {
			sprintf( szAddr, "rtsp://%s:%s@%s/", szUserName, szPasswd, szUrlIP );
	   }
	   else
	   {
		sprintf( szAddr, "rtsp://%s/", szUrlIP );
	   }
	}
	char szStreamUri[100]={0};
	strcpy( szStreamUri, streamUri.c_str() );

	char szSurfix[100]={0};
	int nRemaining = strlen( szStreamUri)-strlen(szAddr) ;
	if ( nRemaining > 0 )
	{
		memmove( szSurfix, szStreamUri+strlen(szAddr), nRemaining );
	}

	m_strUrl[uiStreamNo-1] = streamUri;
	if ( m_pRtspClient[uiStreamNo-1] != NULL )
	{
		delete m_pRtspClient[uiStreamNo-1];
		m_pRtspClient[uiStreamNo-1] = NULL;
	}
	
	m_pRtspClient[uiStreamNo-1] = new CRTSPStreamClient( CRTSPSource::rtspDataCallBack, (void*)this );

	if ( nWithPasswd )
	{
		m_pRtspClient[uiStreamNo-1]->OpenStream( (char*)streamUri.c_str(), true, szUserName, szPasswd );
	}
	else
	{
		m_pRtspClient[uiStreamNo-1]->OpenStream( (char*)streamUri.c_str(), true, (char*)"", (char*)"" );
	}
	return 0;
}


