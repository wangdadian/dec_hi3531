
#include "ChannelManager.h"
#include <unistd.h>
#include "UdpSource.h"
#include "RTSPSource.h"
#include "Hi3531Global.h"
#include "Protocol.h"
#include "MDProtocol.h"

CChannel::CChannel( int nChannelNo, ChnConfigParam chnparam )
{
	memset( m_szUrl, 0, sizeof(m_szUrl ));
	m_nChannelNo = nChannelNo;
	memcpy( &m_chnConfigParam, &chnparam, sizeof( ChnConfigParam ));
	m_pDecoder = new CDecoder(  chnparam.nOutResolution, 
								m_nChannelNo, 
								chnparam.nWbc );
	EncOsdSettings osdset;
	memset(&osdset,0,sizeof(osdset));
	osdset.iChannelNo = nChannelNo;
	osdset.iFont = 32;
	osdset.osd[0].iEnable = 1;
	osdset.osd[0].iType = (int)OsdType_DateAndTime;
	osdset.osd[0].iX = 200;
	osdset.osd[0].iY = 100;
	
	//m_pDecoder->SetOsd(osdset);
	m_pSrc = NULL;
	//sleep(1);
}

CChannel::~CChannel()
{
	if ( m_pSrc != NULL )
	{
		delete m_pSrc;
		m_pSrc = NULL;
	}
    //_DEBUG_("delete m_pDecoder");
	if ( m_pDecoder != NULL )
	{
		delete m_pDecoder;
		m_pDecoder = NULL;
	}
    //_DEBUG_("~CChannel end.");
}

void* CChannel::thDeleteSrc(void * param)
{
    if(param != NULL)
    {
        CChannel *pThis = (CChannel*)param;
        delete pThis->m_pSrc;
		pThis->m_pSrc = NULL;
        _DEBUG_( "Delete m_pSrc.[channel=%d]", pThis->m_nChannelNo+1);
    }
        
    return NULL;
}

void CChannel::Open( char *szUrl, int nVideoFormat )
{

	//strcpy( szUrl, "onvif/192.168.1.121:8080/onvif/device_service" );
	//strcpy( szUrl, "rtsp://192.168.1.121:554/h264_0/admin/admin/");
	//strcpy( szUrl, "udp/192.168.1.127:10002" );
	//strcpy( szUrl, "tcp/192.168.1.201:10002" );
   
	_DEBUG_( "Open URL:%s,channel=%d", szUrl, m_nChannelNo+1);

	if ( m_pSrc != NULL)
	{
	    /*
    	pthread_t  deleteSrcTH;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        pthread_create(&deleteSrcTH, &attr, thDeleteSrc, this);
        */
		delete m_pSrc;
		m_pSrc = NULL;
        _DEBUG_( "Delete m_pSrc, channel=%d", m_nChannelNo+1);
	}

    //此行代码会导致多画面切换时，图像静止20160628
    //m_pDecoder->ResetVdec();
    
	if( strstr( szUrl, "cvbs/" ) == NULL ) 
	{
		m_pDecoder->StartDecoder( );
	}

	//strcpy( szUrl, "tcp/192.168.1.201:30001");
	strcpy(  m_szUrl, szUrl );
	
	if ( strstr( szUrl, "tcp/" ) != NULL )
	{
		m_pSrc = new CNetSource( szUrl );
		m_pSrc->SetRecvCB( &CChannel::OnRecv, (void*)this );
	}
	else if  ( strstr( szUrl, "udp/" ) != NULL )
	{
		//szUrl = "udp/224.31.9.134:7893";
		m_pSrc = new CUdpSource(szUrl);
		m_pSrc->SetRecvCB( &CChannel::OnRecv, (void*)this );

	}
    else if  ( strstr( szUrl, "udptjgs/" ) != NULL )
	{
		char szRealUrl[MAX_URL_LEN] = {0};
        sprintf(szRealUrl, "udp/%s", szUrl+8);
		m_pSrc = new CUdpSource(szRealUrl);
		m_pSrc->SetRecvCB( &CChannel::OnRecv, (void*)this, 1);
	}
	/*
	//format: onvif/ip:port/onvif/device_service 
	else if( strstr( szUrl, "onvif/" ) != NULL ) 
	{
		m_pSrc = new COnvifSource( (char*)(szUrl+strlen("onvif/")));
		m_pSrc->SetRecvCB( &CChannel::OnRecv, (void*)this );
	}
	*/
	else if( strstr( szUrl, "rtsp://" ) != NULL ) 
	{
	    // 20180416注释同时注释Makefiile中的RTSPSource.o的编译，原因为无法引用libRtsp.so中的接口，原因尚未查明。
	    // 变故源于/opt/include下的头文件更新后
		//m_pSrc = new CRTSPSource( szUrl);
		//m_pSrc->SetRecvCB( &CChannel::OnRecv, (void*)this );
	}
	else if( strstr( szUrl, "cvbs/" ) != NULL ) 
	{
		OpenCVBS();
	}
	
}

void CChannel::OpenCVBS( )
{
	m_pDecoder->StartCVBS();
}

void CChannel::OnRecv( unsigned char * buf, int nSize, void *pCbObj )
{
	//printf( "received :%d\n", nSize );
	 CChannel *pThis = (CChannel*)pCbObj;
	 pThis->m_pDecoder->DecodeData(buf, nSize);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CChannelManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CChannelManager::CChannelManager( CDecoderSetting *pDecSet, CEncoderSetting* pEncSet)
{
	pthread_mutex_init(&m_savlock, NULL);
	pthread_mutex_init(&m_openlock, NULL);
	pthread_mutex_init(&m_protocollock, NULL);
	
	for ( int i = 0; i< MAXDECCHANN; i++ )
	{
		m_pChannel[i] = NULL;
	}

	for ( int i = 0; i < MAXENCCHAN; i++ )
	{
		m_pEncoder[i] = NULL;
	}
	
	m_pDecSet = pDecSet;
    m_pEncSet = pEncSet;
	m_pDecSet->SetCB_DDU( OnDDUChanged, (void*)this);
    m_pDecSet->SetCB_OsdShow(OnDecOsdShowChanged,(void*)this);
	m_pDecSet->SetCB_Resolution( OnDisplayChanged, (void*)this);
    
	m_pEncSet->SetCB_OnOsdChange(OnOsdChange, (void*)this);
    m_pEncSet->SetCB_OnNetChange(OnNetChange, (void*)this);
    m_pEncSet->SetCB_OnParamChange(OnParamChange, (void*)this);
    m_pEncSet->SetCB_OnChannelChange(OnChannelChange, (void*)this);
    m_pEncSet->SetCB_OnPtzChange(OnPtzChange, (void*)this);
    m_pEncSet->SetCB_OnPtzPortChange(OnPtzPortChange, (void*)this);
    m_pEncSet->SetCB_OnAlarminChange(OnAlarminChange, (void*)this);
    m_pEncSet->SetCB_OnAlarmoutChange(OnAlarmoutChange, (void*)this);
	
	m_pChannelConfig = new CChannelConfig(pDecSet, pEncSet);

    // 编码
	if ( g_nDeviceType == (int)DeviceModel_Encoder ||
	     g_nDeviceType == (int)DeviceModel_HDEncoder
	   )
	{
		for ( int i = 0; i < g_nMaxEncChn; i++ )
		{
			OpenEnc( i );
		}
	}
    
    // 编解码
	if (g_nDeviceType == (int)DeviceModel_Dec_Enc)
	{
		for ( int i = 0; i < g_nMaxEncChn; i++ )
		{
            // 仅初始化编码通道主码流
            if( (i%2) == 0)
            {
			    OpenEnc( i );
            }
		}
	}

    // 解码、编解码
	if(	g_nDeviceType == (int)DeviceModel_Dec_Enc
		||
		g_nDeviceType == (int)DeviceModel_Decoder)
	{
       
        struDisplayResolution sHdmiDR;
        sHdmiDR.displaymode=3;// hdmi
        sHdmiDR.displayno = 1;
        CDecoderSetting::GetResolution(&sHdmiDR, (void*)m_pDecSet);
        m_pDecoderOsd = new CDecoderOsd(&sHdmiDR);
        SetDecOsdMode();
        
		for (int i=0; i < m_pChannelConfig->m_nChanneoCount; i++ )
		{
			Open( i+1, (char*)m_pChannelConfig->m_ChnParam[i].szUrl, 1, m_pChannelConfig->m_ChnParam[i].nFPS);
		}
        
	}	

	m_pAlarmCtrl = new CAlarmCtrl();
	m_pAlarmCtrl->SetAlarmNotifyCB( OnAlarm, (void *)this );

	m_pPtzCtrl = new CPtzCtrl(
		m_pChannelConfig->m_encsettings.ptzport.iBaude,
		m_pChannelConfig->m_encsettings.ptzport.iStopBits ,
		m_pChannelConfig->m_encsettings.ptzport.iDataBits,
		m_pChannelConfig->m_encsettings.ptzport.iParity	);

	for ( int i = 0; i < g_nMaxEncChn / 2; i++ )
	{
		m_pPtzCtrl->SetPtzType(i,m_pChannelConfig->m_encsettings.ptz[i]);
	}

}

CChannelManager::~CChannelManager()
{
	for ( int i = 0; i< MAXDECCHANN; i++ )
	{
		if ( m_pChannel[i] != NULL )
		{
			delete m_pChannel[i];
			m_pChannel[i] = NULL;
    	}
	}

	if ( m_pChannelConfig != NULL )
	{
		delete m_pChannelConfig;
		m_pChannelConfig = NULL;
	}

	if ( m_pPtzCtrl != NULL )
	{
		delete m_pPtzCtrl;
		m_pPtzCtrl = NULL;
	}

	if ( m_pAlarmCtrl != NULL )
	{
		delete m_pAlarmCtrl;
		m_pAlarmCtrl = NULL;
	}
	if ( m_pDecoderOsd != NULL )
	{
		delete m_pDecoderOsd;
		m_pDecoderOsd = NULL;
    }
    
    for ( int i = 0; i < MAXENCCHAN; i++ )
	{
		if(m_pEncoder[i] != NULL)
		{
            delete m_pEncoder[i];
            m_pEncoder[i] = NULL;
		}
	}

	pthread_mutex_destroy(&m_savlock);
	pthread_mutex_destroy(&m_openlock);
	pthread_mutex_destroy(&m_protocollock);
}

void CChannelManager::GetChannelUrl( int nChannelNo, char *szUrl, int &nFPS )
{
	nFPS = 0;
	szUrl[0]='\0';
	if ( nChannelNo > MAXDECCHANN )
	{
		printf("error invalid channel no:%d\n", nChannelNo );
		return;
	}

	if ( m_pChannel[nChannelNo-1] == NULL )
	{
		printf("error no such channel.%d\n", nChannelNo );
		return;
	}
	strcpy( szUrl, m_pChannel[nChannelNo-1]->m_szUrl );
	nFPS = m_pChannelConfig->m_ChnParam[nChannelNo-1].nFPS;
}

long CChannelManager::OnDecOsdShowChanged( struDecOsdInfo* pOsd, void *pCBObj )
{
    CChannelManager *pThis = ( CChannelManager*)pCBObj;
    if( pOsd == NULL) 
        return -1;
    if ( pThis->m_pDecoderOsd != NULL )
    {
        pThis->m_pDecoderOsd->OnOsdShowChanged(pOsd->show);
    }
    return 0;
}

long CChannelManager::OnDDUChanged( const struDynamicDecUrl* pDDU, void *pCBObj)
{
	if ( pDDU == NULL && pCBObj == NULL ) return -1;
	_DEBUG_( "ddu changed chan:%d url:%s\n", pDDU->channel, pDDU->url );

	if ( pDDU->channel > MAXDECCHANN ) 
	{
		_DEBUG_("error: channel [%d] > max dec channel\n", pDDU->channel );
		return -1;
	}
	CChannelManager *pThis = ( CChannelManager*)pCBObj;
	if (pThis == NULL ) return -1;

    // 停止解码
    if(strlen(pDDU->url)<=0)
    {
        int channel = pDDU->channel;
        pThis->Close(channel);
        _DEBUG_("stop decoder.");
        return 0;
    }
	if ( strstr( pDDU->url , "tcp/" ) == NULL &&
		strstr(pDDU->url , "udp/") == NULL && 
		strstr(pDDU->url , "rtsp://" ) == NULL && 
		strstr(pDDU->url , "cvbs/" ) == NULL)
	{
		_DEBUG_("error: invalid url format:%s\n", pDDU->url );
		return -1;
	}

	pThis->Open( pDDU->channel, (char*)pDDU->url,1, pDDU->fps);
	return 0;
}


long CChannelManager::OnDisplayChanged( const struDisplayResolution* pDR, void *pCBObj)
{
	if ( pDR == NULL || pCBObj == NULL) 
        return -1;
	CChannelManager *pThis = ( CChannelManager*)pCBObj;
    
    //
	pThis->m_pChannelConfig->LoadDisplay();
    
    // 设置OSD显示模式
    pThis->SetDecOsdMode();
    
    // 设置OSD分辨率
    struDisplayResolution tmpDR;
    memset(&tmpDR, 0, sizeof(struDisplayResolution));
    tmpDR.displaymode = 3;
    tmpDR.displayno = 1;
    if(pThis->m_pDecSet->GetResolution(&tmpDR, (void *)pThis->m_pDecSet)==0)
    {
        if ( pThis->m_pDecoderOsd != NULL )
        {
            pThis->m_pDecoderOsd->OnDisplayResolutionChanged(&tmpDR);
        }
    }
    // 重新切换解码避免显示模式更改时，解码接收缓冲多造成解码延时，需重新切换
    for (int i=0; i<MAXDECCHANN; i++ )
    {
        pThis->Open(i+1, pThis->m_pChannelConfig->m_ChnParam[i].szUrl, 1, pThis->m_pChannelConfig->m_ChnParam[i].nFPS);
    }

	return 0;
}

long CChannelManager::SetDecOsdMode()
{
    int mode = 0;
    int channel = 0;
    m_pChannelConfig->GetDispalyMode(mode, channel);
    if ( m_pDecoderOsd != NULL )
    {
        //m_pDecoderOsd->ClearOsdDisplay();
        m_pDecoderOsd->SetOsdShowMode(mode, channel);
    }
    return 0;
}
long CChannelManager::SetDecOsd(int chnno,void * pData)
{
    if(pData == NULL || chnno < 0)
        return -1;

    //DecodecOsdParam* pOsd = (DecodecOsdParam*)pData;
    if ( m_pDecoderOsd != NULL )
    {
        m_pDecoderOsd->SetOsdInfo(pData);
    }
    return 0;
}
long CChannelManager::SetMonitorId(int chnno, int mid)
{
    if ( m_pDecoderOsd != NULL )
    {
        m_pDecoderOsd->SetMonitorId(chnno, mid);
    }
    return 0;
}

long CChannelManager::ClearDecOsd(int chnno)
{
    if ( m_pDecoderOsd != NULL )
    {
        m_pDecoderOsd->ClearOsdShow(chnno);
    }
    return 0;
}
long CChannelManager::StopEncoderOfDec(int chnno)
{
	pthread_mutex_lock(&m_protocollock);
    CProtocol *pTmp = NULL;
    vector<void*>::iterator it;
    for(it=m_pProtocols.begin(); it!=m_pProtocols.end(); it++)
    {
        pTmp = (CProtocol*)*it;
        if(pTmp != NULL)
        {
            pTmp->StopEncoder(chnno);
        }
    }
	pthread_mutex_unlock(&m_protocollock);
    return 0;
}

long CChannelManager::SetDecDisplay(void* pData)
{
    long lRet = 0;
    if(pData == NULL)
        return -1;
    tDecDisplayParam *pdp = (tDecDisplayParam *)pData;
    // 判断参数是否有效
    /*if( (pdp->iChannel<0 || pdp->iChannel>MAXDECCHANN) && pdp->iChannel!=128)
    {
        _DEBUG_("invalid channel %d !", pdp->iChannel);
        return -1;
    }*/
    if( (pdp->iChannel>0 && pdp->iChannel<=MAXDECCHANN)
        ||
        (pdp->iChannel>=128 && pdp->iChannel<=130))
    {
        _DEBUG_("channel %d", pdp->iChannel);
    }
    else
    {
        _DEBUG_("invalid channel %d !", pdp->iChannel);
        return -1;
    }
    
    if( pdp->iDisplaymode<0 || pdp->iDisplaymode>MAXDISPLAYNUM)
    {
        _DEBUG_("invalid display mode %d !", pdp->iDisplaymode);
        return -1;
    }
    if( pdp->iResolution<0 || pdp->iResolution>7)
    {
        _DEBUG_("invalid resolution %d !", pdp->iResolution);
        return -1;
    }
    // 判断参数是否和原参数相同，相同则不处理
    if(pdp->iDisplaymode != 0)
    {
        struDisplayResolution tmpDR;
        memset(&tmpDR, 0, sizeof(struDisplayResolution));
        tmpDR.displaymode = pdp->iDisplaymode;
        tmpDR.displayno = pdp->iDisplayno;
        if(m_pDecSet->GetResolution(&tmpDR, (void *)m_pDecSet)==0)
        {
            if( tmpDR.channel == pdp->iChannel 
                && 
                (tmpDR.resolution == pdp->iResolution || pdp->iResolution==0)
            )
            {
                _DEBUG_("display param not changed, return!");
                return 0;
            }
        }
    }
    if(pdp->iDisplaymode == 0)
    {
        struDisplayResolution tmpDR;
        memset(&tmpDR, 0, sizeof(struDisplayResolution));
        tmpDR.displayno = pdp->iDisplayno;
        int i = 1;
        for(i=1; i<=MAXDISPLAYNUM;i++)
        {
            tmpDR.displaymode = i;
            if(m_pDecSet->GetResolution(&tmpDR, (void *)m_pDecSet)==0)
            {
                if( tmpDR.channel == pdp->iChannel 
                    && 
                    (tmpDR.resolution == pdp->iResolution || pdp->iResolution==0)
                )
                {
                    _DEBUG_("display[%d] param not changed!", i);
                    continue;
                }
                else
                {
                    break;
                }
            }

        }
        if(i>MAXDISPLAYNUM)
        {
            _DEBUG_("all display param not changed, return!");
            return 0;
        }
    }

    struDisplayResolution sDR;
    sDR.channel = pdp->iChannel;
    sDR.displaymode = pdp->iDisplaymode;
    sDR.displayno = pdp->iDisplayno;
    sDR.resolution = pdp->iResolution;
    sDR.enabled = 0;// 生效

    // 写入配置分区
    m_pDecSet->SetResolution(&sDR, (void*)m_pDecSet);
    return lRet;
}

long CChannelManager::OnOsdChange(int chnno, void* pData, void* param)
{
    if(g_nDeviceType== DeviceModel_Dec_Enc)
    {
        // 仅支持主码流
        if((chnno % 2) != 0)
        {
            return 0;
        }
    }

	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;
    
    if(pData == NULL || chnno < 0)
    {
        return -1;
    }
    EncOsdSettings *pOsd = (EncOsdSettings*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.encset[chnno].osd), pOsd, sizeof(EncOsdSettings));
    
	if ( pThis->m_pEncoder[chnno] == NULL )
	{
		pThis->m_pEncoder[chnno] = new CHiEncoder(chnno);
	}

	pThis->m_pEncoder[chnno]->SetOsd( pThis->m_pChannelConfig->m_encsettings.encset[chnno].osd );
    return 0;
}

long CChannelManager::OnNetChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;
    if(pData == NULL || chnno < 0)
        return -1;
    EncNetSettings *pNet = (EncNetSettings*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.encset[chnno].net), \
            pNet, sizeof(EncNetSettings));

    //
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        pThis->SetEncNet(chnno, \
            pThis->m_pChannelConfig->m_encsettings.encset[chnno].net.net[i]);
    }

    return 0;
}
long CChannelManager::OnParamChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) 
        return -1;
    if(pData == NULL || chnno < 0)
        return -1;
    EncoderParam* pParam = (EncoderParam*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.encset[chnno].enc), \
            pParam, sizeof(EncoderParam));
    pThis->SetEncParam(chnno, pThis->m_pChannelConfig->m_encsettings.encset[chnno].enc);
	//pThis->OpenEnc( chnno );
	
    return 0;
}
long CChannelManager::OnChannelChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL) return -1;
    
    if(pData == NULL || chnno<0)
        return -1;
    EncSetting *pEnc = (EncSetting*)pData;

	if ( chnno >= g_nMaxViChn  || chnno < g_nMaxViChn ) return -1;

    // ??DT????3?oí±ào?
    sprintf(pThis->m_pChannelConfig->m_encsettings.encset[chnno*2].szName, "%s", pEnc->szName);
    sprintf(pThis->m_pChannelConfig->m_encsettings.encset[chnno*2].szDeviceId, "%s", pEnc->szDeviceId);

    return 0;
}

long CChannelManager::OnPtzChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;
    if(pData == NULL || chnno < 0)
        return -1;
    PtzSetting* pParam = (PtzSetting*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.ptz[chnno]), \
            pParam, sizeof(PtzSetting));
	
	for ( int i = 0; i < g_nMaxEncChn / 2; i++ )
	{
		pThis->m_pPtzCtrl->SetPtzType(i, pThis->m_pChannelConfig->m_encsettings.ptz[i]);
	}
	
    return 0;
}
long CChannelManager::OnPtzPortChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;

    if(pData == NULL) 
        return -1;
    PtzPortSetting* pPtzPort = (PtzPortSetting*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.ptzport), pPtzPort, sizeof(PtzPortSetting));

	if ( pThis->m_pPtzCtrl != NULL )
	{
		delete pThis->m_pPtzCtrl;
		pThis->m_pPtzCtrl = NULL;
	}
	
	pThis->m_pPtzCtrl = new CPtzCtrl(
		pThis->m_pChannelConfig->m_encsettings.ptzport.iBaude,
		pThis->m_pChannelConfig->m_encsettings.ptzport.iStopBits ,
		pThis->m_pChannelConfig->m_encsettings.ptzport.iDataBits,
		pThis->m_pChannelConfig->m_encsettings.ptzport.iParity	);


	for ( int i = 0; i < g_nMaxEncChn / 2; i++ )
	{
		pThis->m_pPtzCtrl->SetPtzType(i, pThis->m_pChannelConfig->m_encsettings.ptz[i]);
	}
	
    return 0;
}
long CChannelManager::OnAlarminChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;
    
    if(pData == NULL || chnno<0) 
        return -1;
    AlarmInSetting* pAlarmin = (AlarmInSetting*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.alarm.alarmin[chnno]), \
            pAlarmin, sizeof(AlarmInSetting));
    return 0;
}
long CChannelManager::OnAlarmoutChange(int chnno, void* pData, void* param)
{
	CChannelManager *pThis = ( CChannelManager*)param;
	if (pThis == NULL ) return -1;
    
    if(pData == NULL || chnno<0) 
        return -1;
    AlarmOutSetting* pAlarmout = (AlarmOutSetting*)pData;
    memcpy(&(pThis->m_pChannelConfig->m_encsettings.alarm.alarmout[chnno]), \
            pAlarmout, sizeof(AlarmOutSetting));
    return 0;

}

void CChannelManager::OnDataForwardport(char* szData, int iLen)
{
    if(szData == NULL || iLen <= 0)
    {
        _DEBUG_("Forwardport Data==NULL!");
        return;
    }
    _DEBUG_("recv %s [%d]", szData, iLen);
    m_pPtzCtrl->WriteData(szData, iLen);
}

void CChannelManager::SaveUrl( int nChannelNo, char *szUrl, int nFPS )
{
	//pthread_mutex_lock( &m_savlock);
	_DEBUG_( "save url %d, %s, %d", nChannelNo, szUrl, nFPS );
	char szOrigUrl[MAX_URL_LEN]={0};
	int nOrigFps = 0;

	GetChannelUrl( nChannelNo, szOrigUrl, nOrigFps );
    if(nFPS<=0 || nFPS>60)
        nFPS=nOrigFps;

	//printf("orig fps:%d\n", nOrigFps );
	if ( strcmp(szOrigUrl, szUrl)==0 && nOrigFps == nFPS )
	{
		_DEBUG_("the same url, no need save, return.");
		//pthread_mutex_unlock( &m_savlock);
		return;
	}
	
	struDynamicDecUrl ddu;
	memset(&ddu, 0, sizeof(ddu));
	ddu.channel = nChannelNo;
	ddu.fps = nFPS;
	strcpy( ddu.url, szUrl );
	CDecoderSetting::SetDynamicDecUrl(&ddu,(void*)this->m_pDecSet);
	//pthread_mutex_unlock( &m_savlock);
}

void CChannelManager::Open( int nChannelNo, 
							char *szUrl, 
							int nVideoFormat, 
							int iFPS
							)
{
	if ( g_nDeviceType == (int)DeviceModel_Encoder ||
		g_nDeviceType == (int)DeviceModel_HDEncoder)
	{
		return;
	}
    
    // OPEN之前停止之前的连接编码器
    StopEncoderOfDec(nChannelNo);
    // 并清空OSD显示
    //ClearDecOsd(nChannelNo); // 20180201注释掉，测试未发现影响，不注释的话执行时会占用50-60ms的时间延时
	pthread_mutex_lock(&m_openlock);
	if ( m_pChannel[nChannelNo-1] == NULL )
	{
		m_pChannel[nChannelNo-1] = new CChannel(nChannelNo-1, m_pChannelConfig->m_ChnParam[nChannelNo-1]);
	}
	int iRet = m_pChannelConfig->SetFPS( nChannelNo-1, iFPS);
    // 如果设置帧率成功会重新加载显示模式，显示模式加载时，解码接收缓冲多造成解码延时，需重新切换
    if(iRet == 0)
    {
        for (int i=0; i<MAXDECCHANN; i++ )
        {
            if(i==(nChannelNo-1))
            {
                m_pChannel[i]->Open(szUrl, nVideoFormat);
            }
            else
            {
                m_pChannel[i]->Open(m_pChannelConfig->m_ChnParam[i].szUrl, 1);
            }
        }
    }
    else
    {
    	m_pChannel[nChannelNo-1]->Open(szUrl, nVideoFormat );
    }
    pthread_mutex_unlock(&m_openlock);

    ///
    if(g_nDeviceType == (int)DeviceModel_Dec_Enc	)
    {
        int nEncChnNo = (nChannelNo - 1)*2; // 仅支持主码流
        if ( nEncChnNo < 0 || nEncChnNo >= g_nMaxEncChn )
        {
            return;
        }

        if ( m_pEncoder[nEncChnNo] != NULL )
		{
		    m_pEncoder[nEncChnNo]->StartVenc(m_pChannelConfig->m_encsettings.encset[nEncChnNo].enc, true);
        }
        else
        {
            assert(false);
        }
    }

}


 void CChannelManager::Close( int nChannelNo )
{
    if ( g_nDeviceType == (int)DeviceModel_Encoder ||
         g_nDeviceType == (int)DeviceModel_HDEncoder)
    {
        return;
    }
    // 停止之前的连接编码器
    StopEncoderOfDec(nChannelNo);
    // 并清空OSD显示
    ClearDecOsd(nChannelNo);
    // 关闭解码通道
	pthread_mutex_lock(&m_openlock);
	if ( m_pChannel[nChannelNo-1] == NULL )
	{
		pthread_mutex_unlock(&m_openlock);
		return;
	}

	//delete m_pChannel[nChannelNo-1];
	//m_pChannel[nChannelNo-1] = NULL;
	
    //m_pChannelConfig->LoadDisplay();
	pthread_mutex_unlock(&m_openlock);
}

void CChannelManager::StopEncNet( int nChannelNo, int iIndex )
{
	
	if ( nChannelNo < 0 || nChannelNo >= g_nMaxEncChn ) return;
	m_pChannelConfig->m_encsettings.encset[nChannelNo].net.net[iIndex].iEnable = 0;

	if ( m_pEncoder[nChannelNo] != NULL )
	{
		m_pEncoder[nChannelNo]->StopNet( iIndex);
	}
}

void CChannelManager::SetSysTime( int nYear, 
									int nMonth, 
									int nDay, 
									int nHour, 
									int nMin, 
									int nSec )
{
	char szTime[80]={0};
	sprintf(szTime, "%d-%d-%d %d:%d:%d", nYear, 
		nMonth, 
		nDay, 
		nHour, 
		nMin, 
		nSec );

	_DEBUG_( "set sys time:%s", szTime );
	
	CDecoderSetting::SetSysTime(szTime, (void*)m_pDecSet);
}

void CChannelManager::Ptz( int nChannelNo, 
							int nPtzAction, 
							int nParam1, 
							int nParam2 )
{
	
	if ( nChannelNo < 0 || nChannelNo >= g_nMaxEncChn ) return;
	m_pPtzCtrl->Ptz(nChannelNo, nPtzAction, nParam1, nParam2);
}

int CChannelManager::Ptz(  char *szDeviceId,
							int nPtzAction, 
							int nParam1, 
							int nParam2 )
{
	if ( szDeviceId == NULL )
	{
		printf("error: device id == null \n");
		return -1;
	}

	if ( strlen(szDeviceId ) == 0 )
	{
		printf("error: device id == null \n");
		return -1;
	}	
	
	for ( int i = 0; i < g_nMaxEncChn; i++ )
	{
		if ( i % 2 != 0 )
        {
            continue;
		}

		if ( strcmp( m_pChannelConfig->m_encsettings.encset[i].szDeviceId, szDeviceId ) != 0 ) continue;
		m_pPtzCtrl->Ptz(i, nPtzAction, nParam1, nParam2);
		return i;
	}
	return -1;
}

void CChannelManager::SetEncNet(int nChannelNo, 
								EncNetParam encnet )
{
	if ( nChannelNo < 0 || nChannelNo >= g_nMaxEncChn )
    {
        return;
	}
	memcpy( &m_pChannelConfig->m_encsettings.encset[nChannelNo].net.net[encnet.iIndex], &encnet, sizeof(encnet));
	if ( m_pEncoder[nChannelNo] != NULL )
	{
	    if(encnet.iEnable == 1)
        {   
		    m_pEncoder[nChannelNo]->StartNet(encnet);
        }
	}
    m_pEncSet->SetEncTransfer( &(m_pChannelConfig->m_encsettings.encset[nChannelNo].net), nChannelNo, false);
}

int CChannelManager::SetEncNet( char *szDeviceId,
        								int nStreamNo, 
        								EncNetParam encnet )
{
	if ( szDeviceId == NULL )
	{
		printf("error: device id == null \n");
		return -1;
	}

	if ( strlen(szDeviceId ) == 0 )
	{
		printf("error: device id == null \n");
		return -1;
	}	
	
	for ( int i = 0; i < g_nMaxEncChn; i++ )
	{
		if ( i % 2 != 0 )
        {
            continue;
		}

		if ( strcmp( m_pChannelConfig->m_encsettings.encset[i].szDeviceId, szDeviceId ) != 0 )
        {
            continue;
		}
		if ( nStreamNo > 0 )
		{
            i++;
		}
		memcpy( &m_pChannelConfig->m_encsettings.encset[i].net.net[0], &encnet, sizeof(encnet));

		if ( m_pEncoder[i] != NULL )
		{
    		if(encnet.iEnable == 1)
            {   
			    m_pEncoder[i]->StartNet(encnet);
            }
		}
        m_pEncSet->SetEncTransfer( &(m_pChannelConfig->m_encsettings.encset[i].net), i, false);
		return i;
	}

	_DEBUG_("cannot find device:%s", szDeviceId );
	return -1;
}

void CChannelManager::GetEncNet(int nChannelNo, 
								  int nIndex, 
								  EncNetParam &encnet )
{
	
	if ( nChannelNo < 0 || nChannelNo >= g_nMaxEncChn )
    {
        return;
	}
 	memcpy( &encnet, &m_pChannelConfig->m_encsettings.encset[nChannelNo].net.net[nIndex], sizeof(encnet));
}

void CChannelManager::SetEncParam( int nChannelNo, 
									EncoderParam encset )
{
	if ( nChannelNo < 0 || nChannelNo >= g_nMaxEncChn )
    {
        return;
	}
	memcpy( &m_pChannelConfig->m_encsettings.encset[nChannelNo].enc, &encset, sizeof(encset ));
	if ( m_pEncoder[nChannelNo] != NULL )
	{
		m_pEncoder[nChannelNo]->StartVenc(m_pChannelConfig->m_encsettings.encset[nChannelNo].enc );
	}
    m_pEncSet->SetEncVideo(&m_pChannelConfig->m_encsettings.encset[nChannelNo].enc, 
			&m_pChannelConfig->m_encsettings.ptz[nChannelNo/2], nChannelNo, false );
}

int CChannelManager::SetEncParam( char *szDeviceId,
									int nStreamNo, 
									EncoderParam encset )
{
	if ( szDeviceId == NULL )
	{
		printf("error: device id == null \n");
		return -1;
	}

	if ( strlen(szDeviceId ) == 0 )
	{
		printf("error: device id == null \n");
		return -1;
	}	
	
	for ( int i = 0; i < g_nMaxEncChn; i++ )
	{
		if ( i % 2 != 0 )
        {
            continue;
		}
		_DEBUG_("channel:%d id:%s", i,  m_pChannelConfig->m_encsettings.encset[i].szDeviceId );
		if ( strcmp( m_pChannelConfig->m_encsettings.encset[i].szDeviceId, szDeviceId ) != 0 )
        {
            continue;
		}
		if ( nStreamNo > 0 )
        {
            i++;
		}
		memcpy( &m_pChannelConfig->m_encsettings.encset[i].enc, &encset, sizeof(encset ));
		if ( m_pEncoder[i] != NULL )
		{
			m_pEncoder[i]->StartVenc(m_pChannelConfig->m_encsettings.encset[i].enc);
		}
        m_pEncSet->SetEncVideo(&m_pChannelConfig->m_encsettings.encset[i].enc, 
			                   &m_pChannelConfig->m_encsettings.ptz[i/2], i, false );
		return i;
	}

	_DEBUG_("cannot find device:%s", szDeviceId );
	return -1;
	
}

void CChannelManager::OpenEnc( int nChannelNo, bool bStartVEncForce)
{
	pthread_mutex_lock(&m_openlock);
	if ( m_pEncoder[nChannelNo] == NULL )
	{
		m_pEncoder[nChannelNo] = new CHiEncoder(nChannelNo);
	}
	if ( m_pChannelConfig->m_encsettings.encset[nChannelNo].iEnable > 0 )
	{
		m_pEncoder[nChannelNo]->SetEncSet( m_pChannelConfig->m_encsettings.encset[nChannelNo] );
		m_pEncoder[nChannelNo]->StartVenc(m_pChannelConfig->m_encsettings.encset[nChannelNo].enc, bStartVEncForce);
		m_pEncoder[nChannelNo]->SetOsd( m_pChannelConfig->m_encsettings.encset[nChannelNo].osd );
		for ( int i = 0 ; i < MAXNET_COUNT; i++ )
		{
			if ( m_pChannelConfig->m_encsettings.encset[nChannelNo].net.net[i].iEnable > 0 )
			{
				m_pEncoder[nChannelNo]->StartNet( m_pChannelConfig->m_encsettings.encset[nChannelNo].net.net[i] );
			}
		}
	}
	pthread_mutex_unlock(&m_openlock);

}

void CChannelManager::AddProtocols( void *pProtocol )
{
	pthread_mutex_lock(&m_protocollock);
	m_pProtocols.push_back( (void*)pProtocol );
	pthread_mutex_unlock(&m_protocollock);
}

int CChannelManager::OnAlarm( char *szAlarmInId, 
								int nAlarminId, 
								int nStatus, 
								void *pObj )
{
	CChannelManager *pThis = ( CChannelManager*)pObj;
	pthread_mutex_lock(&pThis->m_protocollock);
	for ( unsigned int i = 0 ; i < pThis->m_pProtocols.size(); i++ )
	{
		CProtocol *pProtocol = ( CProtocol*)pThis->m_pProtocols[i];
		pProtocol->OnAlarm( szAlarmInId, nAlarminId, nStatus );
	}
	pthread_mutex_unlock(&pThis->m_protocollock);
	return 0;
}

void CChannelManager::SetGuard( char *szDeviceId )
{
	m_pAlarmCtrl->SetAlarmGuard((char*) szDeviceId,1);
}

void CChannelManager::ResetGuard( char *szDeviceId )
{
	m_pAlarmCtrl->SetAlarmGuard( (char*)szDeviceId, 0);
}

void CChannelManager::ClearAlarm( char *szDeviceId )
{
	m_pAlarmCtrl->ClearAlarmIn((char*)szDeviceId);
}


