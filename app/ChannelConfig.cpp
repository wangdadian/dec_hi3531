
#include "ChannelConfig.h"

#include "PublicDefine.h"
#include "sample_comm.h"
#include "Hi3531Global.h"

const int mapIntf[]=
{
	(int)VO_INTF_VGA, 
	(int)VO_INTF_CVBS,
	(int)VO_INTF_HDMI
};


const int mapResolution[]=
{
	/*
	#0-unknown
#1-1920*1080
#2-800*600
#3-1024*768
#4-1280*1024
#5-1366*768
#6-1440*900
#7-1280*800
	*/
	(int)0, 
	(int)VO_OUTPUT_1080P60, 
	(int)VO_OUTPUT_800x600_60, 
	(int)VO_OUTPUT_1024x768_60, 
	(int)VO_OUTPUT_1280x1024_60, 
	(int)VO_OUTPUT_1366x768_60, 
	(int)VO_OUTPUT_1440x900_60, 
	(int)VO_OUTPUT_1280x800_60, 
	
};

CChannelConfig::CChannelConfig( CDecoderSetting *pDecSet, CEncoderSetting* pEncSet)
{
	m_nChanneoCount = 0;
	m_pDecSet = pDecSet;
    m_pEncSet = pEncSet;

    m_iMuxDisplayMode = MD_DISPLAY_MODE_1MUX;
    m_iMuxDisplayModeLast = -1;
	for ( int i = 0; i < MAXDECCHANN; i++)
	{
		memset( &m_ChnParam[i], 0, sizeof( ChnConfigParam ));
	}

	memset(&m_encsettings, 0, sizeof( EncSettings ) );

	InitPtzDefault();

	LoadSip();
	
	
	if ( g_nDeviceType == (int)DeviceModel_Decoder )
	{
		LoadDecUrl();
		LoadDisplay();
	}
    else if(g_nDeviceType == (int)DeviceModel_Encoder || 
		 g_nDeviceType == (int)DeviceModel_HDEncoder)
    {
        m_pEncSet->Init();
		InitEncoderDefault();
    }
	else if(g_nDeviceType == (int)DeviceModel_Dec_Enc)
	{
		LoadDecUrl();
		LoadDisplay();
        m_pEncSet->Init();
		InitEncoderDefault();
	}
}

void CChannelConfig::LoadSip()
{

	struSipServer sipserver;
	memset( &sipserver, 0, sizeof(sipserver));
	if ( m_pDecSet->GetSipServer( &sipserver, (void*)this ) == 0 )
	{
		strncpy( g_szSipServerId, sipserver.id, 255 );
		strncpy( g_szSipServerIp, sipserver.ip, 255 );

		if ( strlen( g_szSipServerIp ) == 0 )
		{
			strcpy( g_szSipServerIp, "0.0.0.0" );
			
		}
		if ( strlen( g_szSipServerId ) == 0 )
		{
			strcpy( g_szSipServerId, "1010101099000001");
		}
	}


}


void CChannelConfig::LoadDecUrl()
{

	for ( int i = 0; i < MAXDECCHANN; i++)
	{
		memset( &m_ChnParam[i], 0, sizeof( ChnConfigParam ));
        /*
        char *szUrl = NULL;
		m_pDecSet->ReadDynamicDecUrl( i+1,  szUrl );
		if ( szUrl != NULL )
		{
			strcpy( m_ChnParam[i].szUrl, szUrl );
			free(szUrl );
		}
		else
		{
			strcpy( m_ChnParam[i].szUrl,"udp/127.0.0.1:30004");
			m_ChnParam[i].nOutResolution = 1;
			m_ChnParam[i].nWbc =  VO_INTF_VGA;
		}*/
		struDynamicDecUrl sDDU;
		m_pDecSet->ReadDynamicDecUrl(i+1, &sDDU);
		if ( strlen(sDDU.url)>0 )
		{
			strcpy( m_ChnParam[i].szUrl, sDDU.url);
            if(sDDU.fps>0)
                m_ChnParam[i].nFPS = sDDU.fps;
            else
                m_ChnParam[i].nFPS = 25;
		}
		else
		{
			strcpy( m_ChnParam[i].szUrl,"udp/127.0.0.1:30004");
            m_ChnParam[i].nFPS = 25;
			m_ChnParam[i].nOutResolution = 1;
			m_ChnParam[i].nWbc =  VO_INTF_VGA;
		}

	}

}
int CChannelConfig::GetDispalyMode(int &mode, int &channel)
{
    mode = MD_DISPLAY_MODE_1MUX;
    channel = 0;
    struDisplayResolution *sDR = new struDisplayResolution[MAXDISPLAYNUM];
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
		memset( &sDR[i], 0, sizeof( struDisplayResolution ) );
		sDR[i].displaymode = i+1;
		sDR[i].displayno = 1;
		if ( CDecoderSetting::GetResolution(&sDR[i], (void*)m_pDecSet) < 0 )
			continue;
    }
    // 查找顺序不能更改，从大到小分屏模式顺序
    // 16分屏
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
        if(sDR[i].channel == 130)
        {
            mode = MD_DISPLAY_MODE_16MUX;
            channel = 130;
            goto GOTO_RETURN;
        }
    }
    // 9分屏
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
        if(sDR[i].channel == 129)
        {
            mode = MD_DISPLAY_MODE_9MUX;
            channel = 129;
            goto GOTO_RETURN;
        }
    }
    // 4分屏
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
        if(sDR[i].channel == 128)
        {
            mode = MD_DISPLAY_MODE_4MUX;
            channel = 128;
            goto GOTO_RETURN;
        }
    }

    // 关联通道不同
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
        if(sDR[i].channel != sDR[0].channel)
        {
            mode = MD_DISPLAY_MODE_MESS;
            channel = 0;
            goto GOTO_RETURN;
        }
    }
    
    // 1分屏显示模式
    mode = MD_DISPLAY_MODE_1MUX;
    channel = sDR[0].channel;
    
GOTO_RETURN:
	PTR_DELETE_A(sDR);
    return 0;
}

void CChannelConfig::LoadDisplay()
{
    int iTmpChnno = 0;
    int iTmpMode = 0;
    GetDispalyMode(iTmpMode, iTmpChnno);
    m_iMuxDisplayMode = iTmpMode;
    if(m_iMuxDisplayMode == MD_DISPLAY_MODE_MESS)
    {
        m_iMuxDisplayMode = MD_DISPLAY_MODE_1MUX;
    }
    struDisplayResolution *sDR = new struDisplayResolution[MAXDISPLAYNUM];
	for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	{
		memset( &sDR[i], 0, sizeof( struDisplayResolution ) );
		sDR[i].displaymode = i+1;
		sDR[i].displayno = 1;
		if ( CDecoderSetting::GetResolution(&sDR[i], (void*)m_pDecSet) < 0 )
			continue;
    }

    // 防止system init出现在bnc输出显示
    if(m_iMuxDisplayModeLast == -1)
    {
        for (int i=0; i<MAXDECCHANN; i++ )
        {
            m_ChnParam[i].nWbc = (int)VO_INTF_VGA | (int)VO_INTF_CVBS | (int)VO_INTF_HDMI;
            m_ChnParam[i].nOutResolution = VO_OUTPUT_1080P60;

            InitMuxDisplay( i+1, m_ChnParam[i].nWbc, m_ChnParam[i].nOutResolution, m_ChnParam[i].nFPS, m_ChnParam[i].nVoDev );
            UnInitMuxDisplay( i+1,m_ChnParam[i].nWbc, m_ChnParam[i].nVoDev);
        }
    }

    for (int i=0; i<MAXDECCHANN; i++ )
    {
		if( m_iMuxDisplayModeLast == MD_DISPLAY_MODE_1MUX)
        {
            UnInitDisplay( i+1,m_ChnParam[i].nWbc, m_ChnParam[i].nVoDev);
        }
        else if( m_iMuxDisplayModeLast == MD_DISPLAY_MODE_4MUX)
        {
            UnInitMuxDisplay( i+1,m_ChnParam[i].nWbc, m_ChnParam[i].nVoDev);
        }
        else if( m_iMuxDisplayModeLast == MD_DISPLAY_MODE_9MUX)
        {
            UnInit9MuxDisplay( i+1,m_ChnParam[i].nWbc, m_ChnParam[i].nVoDev);
        }
        else if( m_iMuxDisplayModeLast == MD_DISPLAY_MODE_16MUX)
        {
            UnInit16MuxDisplay( i+1,m_ChnParam[i].nWbc, m_ChnParam[i].nVoDev);
        }

    }
    m_iMuxDisplayModeLast = m_iMuxDisplayMode;
    
	for ( int i = 0; i < MAXDECCHANN; i++)
	{
		m_ChnParam[i].nWbc = 0;
	}

    if( m_iMuxDisplayMode != MD_DISPLAY_MODE_1MUX )
    {
        for ( int i = 0; i < MAXDECCHANN; i++ )
	    {
	        m_ChnParam[i].nWbc = (int)VO_INTF_VGA | (int)VO_INTF_CVBS | (int)VO_INTF_HDMI;
            m_ChnParam[i].nOutResolution = mapResolution[sDR[2].resolution];//HDMI的分辨率
            //m_ChnParam[i].nOutResolution = VO_OUTPUT_1080P60;
        }
    }
    if( m_iMuxDisplayMode == MD_DISPLAY_MODE_1MUX )
    {	
        for ( int i = 0; i < MAXDISPLAYNUM; i++ )
	    {
    		if ( i <= 2 )
    		{
    			if ( sDR[i].channel == 1  )
    			{
    				m_ChnParam[sDR[i].channel-1].nWbc |= mapIntf[i];
    				if ( i!=1 )
    				{
    					m_ChnParam[0].nOutResolution = mapResolution[sDR[i].resolution];
    				}
    			}
    			else if ( sDR[i].channel == 2	)
    			{
    				m_ChnParam[sDR[i].channel-1].nWbc |= mapIntf[i];
    				if ( i!=1 )
    				{
    					m_ChnParam[1].nOutResolution = mapResolution[sDR[i].resolution];
    				}
    			}
                else if ( sDR[i].channel == 3	)
    			{
    				m_ChnParam[sDR[i].channel-1].nWbc |= mapIntf[i];
    				if ( i!=1 )
    				{
    					m_ChnParam[2].nOutResolution = mapResolution[sDR[i].resolution];
    				}
    			}
                else if ( sDR[i].channel == 4	)
    			{
    				m_ChnParam[sDR[i].channel-1].nWbc |= mapIntf[i];
    				if ( i!=1 )
    				{
    					m_ChnParam[3].nOutResolution = mapResolution[sDR[i].resolution];
    				}
    			}
            }
		}		
	}

	PTR_DELETE_A(sDR);
	m_nChanneoCount = 0;
	InitDefault();
}

void CChannelConfig::LoadConfigFile( char *szConfigFile )
{
	strncpy( m_szConfigFile, szConfigFile, 1000);
}

CChannelConfig::~CChannelConfig()
{
	if(  m_ChnParam[0].nWbc != 0 )
	{
		UnInitDisplay(1, m_ChnParam[0].nWbc, m_ChnParam[0].nVoDev);
	}

	if(  m_ChnParam[1].nWbc != 0 )
	{
		UnInitDisplay(2, m_ChnParam[1].nWbc, m_ChnParam[1].nVoDev);
	}
    if(  m_ChnParam[2].nWbc != 0 )
	{
		UnInitDisplay(3, m_ChnParam[2].nWbc, m_ChnParam[2].nVoDev);
	}

	if(  m_ChnParam[3].nWbc != 0 )
	{
		UnInitDisplay(4, m_ChnParam[3].nWbc, m_ChnParam[3].nVoDev);
	}
}


void CChannelConfig::InitEncoderDefault()
{
	 m_pEncSet->GetEncoderSettings(&m_encsettings);
	/*for ( int i = 0; i < g_nMaxEncChn; i++ )
	{
		m_encsettings.encset[i].iEnable = 1;
		EncoderParam encparam;

		if ( i%2==0 )
		{	
			encparam.iCbr = 0;
			encparam.iProfile = 2;
			encparam.iFPS = 25;
			encparam.iGop = 10;

			if ( g_nDeviceType == (int)DeviceType_HDEncoder )
			{
				encparam.iResolution = (int)ResolutionType_1080P;
				encparam.iBitRate = 6000;
			}
			else
			{
				encparam.iResolution = (int)ResolutionType_D1;
				encparam.iBitRate = 800;
			}

		}
		else
		{
			encparam.iBitRate = 512;
			encparam.iCbr = 0;
			encparam.iProfile = 2;
			encparam.iFPS = 25;
			encparam.iGop = 10;
			encparam.iResolution = (int)ResolutionType_CIF;
		}

		
		sprintf( m_encsettings.encset[i].szName, "通道%d", (i/2 +1) );
		sprintf( m_encsettings.encset[i].szDeviceId, "10101010110000%02d", (i / 2)+1 );
		memcpy( &m_encsettings.encset[i].enc, 
		&encparam, sizeof(encparam));
		
		EncNetParam net;
		memset(&net, 0, sizeof(EncNetParam));
		net.iIndex = 0;
		net.iMux = 0;
		net.iEnable = 1;
		strcpy( net.szPlayAddress, "127.0.0.1" );
		net.iPlayPort = 6688+i;
		net.iNetType = (int)NetType_TCP;
	
		memcpy(&m_encsettings.encset[i].net.net[0], 
			&net, sizeof(net));
	
		memset(&net, 0, sizeof(EncNetParam));
		net.iIndex = 1;
		net.iMux = 0;
		
		if ( i == 4 )
		net.iEnable = 1;
		else net.iEnable = 0;
		
		strcpy( net.szPlayAddress, "192.168.1.128" );
		net.iPlayPort = 8001+i;
		net.iNetType = (int)NetType_UDP;
		
		memcpy(&m_encsettings.encset[i].net.net[1], 
				&net, sizeof(net));
	
	
		memset(&net, 0, sizeof(EncNetParam));
		net.iIndex = 2;
		net.iMux = 0;
		strcpy( net.szPlayAddress, "192.168.1.128" );
		net.iPlayPort = 9000+i;
		net.iEnable = 0;
		net.iNetType = (int)NetType_RTSP;
		memcpy(&m_encsettings.encset[i].net.net[2], 
				&net, sizeof(net));
    
		EncOsdSettings osds;
		memset(&osds, 0, sizeof(osds));
		osds.iFont = 32;
		osds.iChannelNo = i;
			
			
		osds.osd[0].iEnable = 1;
		osds.osd[0].iX = 20;
		osds.osd[0].iY = 12;
		osds.osd[0].iType = (int)OsdType_DateAndTime;


		osds.osd[1].iEnable = 1;
		osds.osd[1].iX = 20;
		osds.osd[1].iY = 52;
		osds.osd[1].iType = (int)OsdType_Content;
		strcpy( osds.osd[1].szOsd, m_encsettings.encset[i].szName);
		memcpy(&m_encsettings.encset[i].osd, 
							&osds, sizeof(osds));

	}
*/
}


int CChannelConfig::SetFPS( int nChannel, int iFPS )
{
	if (nChannel >= MAXDECCHANN || nChannel<0) 
	{
		_DEBUG_("invalid channel:%d\n", nChannel );
        return -1;
	}
	
	if ( iFPS <= 0 || iFPS>60)
	{
		iFPS = 30;
	}

	if ( iFPS != m_ChnParam[nChannel].nFPS )
	{
	    _DEBUG_( "set fps:%d %d", nChannel, iFPS );
		m_ChnParam[nChannel].nFPS = iFPS;

		/*
		HI_U32 s32Ret = HI_MPI_VO_SetChnFrameRate(m_ChnParam[nChannel].nVoDev, 0, iFPS);
		if (s32Ret != HI_SUCCESS)
		{
		printf("Set channel %d frame rate failed with errno %#x!\n", 0,
		s32Ret);
		}
		*/


/*
        UnInitDisplay( nChannel+1, 
            m_ChnParam[nChannel].nWbc, 
            m_ChnParam[nChannel].nVoDev );

		InitDisplay( nChannel+1, 
			m_ChnParam[nChannel].nWbc, 
			m_ChnParam[nChannel].nOutResolution, 
			m_ChnParam[nChannel].nFPS,
			m_ChnParam[nChannel].nVoDev);
*/
        for(int i=0; i<MAXDISPLAYNUM; i++)
        {
            if( m_iMuxDisplayMode == MD_DISPLAY_MODE_1MUX )
            {		
                UnInitDisplay( nChannel+1, 
				    m_ChnParam[nChannel].nWbc, 
				    m_ChnParam[nChannel].nVoDev );

                InitDisplay( nChannel+1, 
        			m_ChnParam[nChannel].nWbc, 
        			m_ChnParam[nChannel].nOutResolution, 
        			m_ChnParam[nChannel].nFPS,
        			m_ChnParam[nChannel].nVoDev);
            }
            else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_4MUX )
            {
                UnInitMuxDisplay( nChannel+1, 
				    m_ChnParam[nChannel].nWbc, 
				    m_ChnParam[nChannel].nVoDev );

                InitMuxDisplay( nChannel+1, 
        			m_ChnParam[nChannel].nWbc, 
        			m_ChnParam[nChannel].nOutResolution, 
        			m_ChnParam[nChannel].nFPS,
        			m_ChnParam[nChannel].nVoDev);
            }  
            else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_9MUX )
            {
                UnInit9MuxDisplay( nChannel+1, 
				    m_ChnParam[nChannel].nWbc, 
				    m_ChnParam[nChannel].nVoDev );

                Init9MuxDisplay( nChannel+1, 
        			m_ChnParam[nChannel].nWbc, 
        			m_ChnParam[nChannel].nOutResolution, 
        			m_ChnParam[nChannel].nFPS,
        			m_ChnParam[nChannel].nVoDev);
            }  
            else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_16MUX )
            {
                UnInit16MuxDisplay( nChannel+1, 
				    m_ChnParam[nChannel].nWbc, 
				    m_ChnParam[nChannel].nVoDev );

                Init16MuxDisplay( nChannel+1, 
        			m_ChnParam[nChannel].nWbc, 
        			m_ChnParam[nChannel].nOutResolution, 
        			m_ChnParam[nChannel].nFPS,
        			m_ChnParam[nChannel].nVoDev);

            }
            _DEBUG_( "set fps:%d %d, OK", nChannel, iFPS );
            return 0; // 设置成功
        }
	}
    return 1;// 未设置
}

void  CChannelConfig::InitDefault()
{
	//m_ChnParam[0].nOutResolution = (int)VO_OUTPUT_1080P60;
	//m_ChnParam[0].nWbc =   VO_INTF_VGA;

    m_nChanneoCount = MAXDECCHANN;
	for(int i=0; i<MAXDECCHANN; i++)
    {
		if( m_iMuxDisplayMode == MD_DISPLAY_MODE_1MUX)
        {
            InitDisplay( i+1, m_ChnParam[i].nWbc, m_ChnParam[i].nOutResolution, m_ChnParam[i].nFPS, m_ChnParam[i].nVoDev );
        }
        else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_4MUX)
        {
            InitMuxDisplay( i+1, m_ChnParam[i].nWbc, m_ChnParam[i].nOutResolution, m_ChnParam[i].nFPS, m_ChnParam[i].nVoDev );
        }
        else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_9MUX)
        {
            Init9MuxDisplay( i+1, m_ChnParam[i].nWbc, m_ChnParam[i].nOutResolution, m_ChnParam[i].nFPS, m_ChnParam[i].nVoDev );
        }
        else if( m_iMuxDisplayMode == MD_DISPLAY_MODE_16MUX)
        {
            Init16MuxDisplay( i+1, m_ChnParam[i].nWbc, m_ChnParam[i].nOutResolution, m_ChnParam[i].nFPS, m_ChnParam[i].nVoDev );
        }
    }   
    /*
	InitDisplay( 1, m_ChnParam[0].nWbc, m_ChnParam[0].nOutResolution, 
	m_ChnParam[0].nFPS,
	m_ChnParam[0].nVoDev);
	m_nChanneoCount++;
	
	//m_ChnParam[1].nOutResolution = (int)VO_OUTPUT_1080P60;
	//m_ChnParam[1].nWbc =  VO_INTF_HDMI | VO_INTF_CVBS ;

	InitDisplay( 2, m_ChnParam[1].nWbc, m_ChnParam[1].nOutResolution, m_ChnParam[1].nFPS, m_ChnParam[1].nVoDev );	
	m_nChanneoCount++;
*/

}

void CChannelConfig::InitPtzDefault()
{
	m_encsettings.ptzport.iBaude = 4800;
	m_encsettings.ptzport.iDataBits= 8;
	m_encsettings.ptzport.iParity = 0;
	m_encsettings.ptzport.iStopBits = 1;


	for ( int i = 0; i < g_nMaxEncChn/2; i++ )
	{
		m_encsettings.ptz[i].iPtzType = 0;
		m_encsettings.ptz[i].iAddressCode = i+1;
	}
}


const char *szParity[]={
	"N",
	"O",
	"E"
};

char *CChannelConfig::GetPtzDevice( char *szDev )
{

	sprintf(szDev, "%s:%d:%d%s%d", DEV_RS485, 
		m_encsettings.ptzport.iBaude,
		m_encsettings.ptzport.iDataBits,
		szParity[m_encsettings.ptzport.iParity],
		m_encsettings.ptzport.iStopBits);
	return szDev;
}





