#include "Forwardport.h"

CForwardport::CForwardport( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager )
{
    ForwardportSetting sFp;
    sFp.iPort = nPort;
    sFp.iEnable = 1;
    sFp.iType = 1; //TCP
    new (this)CForwardport(&sFp, pChnManager);  
}

CForwardport::CForwardport(ForwardportSetting* pFp, CChannelManager *pChnManager ) : CProtocol( pFp->iPort, pChnManager )
{
    // 启动配置检查线程
    pthread_create(&m_thWorkerChkCfg, NULL, THWorkerChkCfg, (void*)this);
    memset(&m_sFp, 0 , sizeof(ForwardportSetting));
    memcpy(&m_sFp, pFp, sizeof(ForwardportSetting));
    
    Init();
}

CForwardport::~CForwardport()
{
    UnInit();
    
    if( m_thWorkerChkCfg != 0 )
    {
        pthread_join(m_thWorkerChkCfg, NULL);
        m_thWorkerChkCfg = 0;
    }
}

long CForwardport::Init()
{
    m_iExitFlag = 0;
    m_pServer = NULL;
    m_pNetUdp = NULL;
    m_iNetType = m_sFp.iType;
    pthread_mutex_init( &m_lock, NULL );
    m_thWorkerUdp = 0;

    if( m_sFp.iEnable > 0 )
    {
        if(m_iNetType == 1)
        {
            //TCP
    	    m_pServer = new CSyncNetServer( CForwardport::THWorker , (void*)this );
    	    if(m_pServer->StartServer( m_sFp.iPort) > 0)
            {
                _DEBUG_("start forward port: tcp@%d ok", m_sFp.iPort);
            }
            else
            {
                _DEBUG_("start forward port: tcp@%d failed!", m_sFp.iPort);
            }            
        }
        else
        {
            //UDP
            m_pNetUdp = new CNetUdp();
            // 启动UDP接收线程
            pthread_create(&m_thWorkerUdp, NULL, THWorkerUdp, (void*)this);
            m_pNetUdp->Open(m_sFp.iPort, (char*)"127.0.0.1", false);
            _DEBUG_("start forward port: udp@%d", m_sFp.iPort);
        }
    }
    else
    {
        _DEBUG_("forward port disabled!");
    }
    return 0;
}
long CForwardport::UnInit()
{
	m_iExitFlag = 1;
    if( m_thWorkerUdp != 0 )
    {
        pthread_join(m_thWorkerUdp, NULL);
        m_thWorkerUdp = 0;
    }

	if ( m_pServer != NULL)
	{
	    m_pServer->StopServer();
		delete m_pServer;
		m_pServer = NULL;
	}
	if ( m_pNetUdp != NULL)
	{
	    m_pNetUdp->Close();
		delete m_pNetUdp;
		m_pNetUdp = NULL;
	}
    pthread_mutex_destroy( &m_lock );
    return 0;
}

void CForwardport::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s\n", net->getRemoteAddr());
    if(p == NULL)
    {
        _DEBUG_("p==NULL! return\n");
        return ;
    }
	CForwardport *pThis = ( CForwardport*)p;
    char tmp[60]={0};
    int ret = 0;
	while( pThis->m_iExitFlag == 0 )
	{
		if ( !net->IsOK() )
        {
            _DEBUG_("net is not ok!");
            break;
        }
		
		ret = net->RecvNT( tmp, 60, 100 );
		if ( ret > 0 )
		{
			pThis->ProcessBuffer(tmp, ret);
            memset(tmp, 0, sizeof(tmp));
		}
		else if ( ret < 0 )
        {
            _DEBUG_("recv data error!");
            break;
        }
        else
        {
            //_DEBUG_("no data received!");
        }
	};

	_DEBUG_("client disconnect:%s\n", net->getRemoteAddr());

}
void* CForwardport::THWorkerUdp(void* param)
{
    if(param == NULL)
    {
        return NULL;
    }
    _DEBUG_("start!");
    CForwardport *pThis = ( CForwardport*)param;
    char tmp[60]={0};
    int ret = 0;
	while( pThis->m_iExitFlag == 0 )
	{
		memset(tmp, 0, sizeof(tmp));	
		ret = pThis->m_pNetUdp->GetDataNT( tmp, 60, 100 );

		if ( ret > 0 )
		{
			pThis->ProcessBuffer(tmp, ret);
		}
		if ( ret < 0 ) break;

    }
    _DEBUG_("exit!");
    return NULL;
}
void* CForwardport::THWorkerChkCfg(void* param)
{
    if(param == NULL)
    {
        return NULL;
    }
    _DEBUG_("start!");
    CForwardport *pThis = ( CForwardport*)param;
    ForwardportSetting sFP;
    int iTime = 0;
	while( pThis->m_iExitFlag == 0 )
	{
        // 获取透明端口信息
        memset(&sFP, 0, sizeof(ForwardportSetting));
        CProfile pro;
        if (pro.load((char*)CFG_FILE_ENCODER) >= 0)
        {
            sFP.iEnable = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Enable", 0);
            sFP.iType = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Type", 0);
            sFP.iPort = pro.GetProperty((char*)"ENC_FORWARD_PORT", (char*)"ENC_FP_Port", FORWARD_PORT);
            if(sFP.iPort<=0 || sFP.iPort>65535)
            {
                sFP.iPort = FORWARD_PORT;
            }
            if(
                sFP.iEnable == pThis->m_sFp.iEnable && 
                sFP.iType == pThis->m_sFp.iType && 
                sFP.iPort == pThis->m_sFp.iPort
               )
            {
                // 设置未变
            }
            else
            {
                _DEBUG_("Forwardport Config Changed[%d, %d, %d]!", sFP.iEnable, sFP.iType, sFP.iPort);
                pThis->UnInit();
                memcpy(&pThis->m_sFp, &sFP, sizeof(ForwardportSetting));
                pThis->Init();
            }
        }
        else
        {
            _DEBUG_("load file failed [FileName=%s]\n", CFG_FILE_ENCODER);
        }
        // 休息
        while(pThis->m_iExitFlag == 0 && iTime < 5)
        {
            iTime++;
            CFacility::SleepSec(1);
        }
        iTime = 0;
    }
    _DEBUG_("exit!");
    return NULL;
}

void CForwardport::ProcessBuffer(char *buf, int &iSize)
{
	m_pChnManager->OnDataForwardport(buf, iSize);
}





