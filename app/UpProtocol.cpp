#include "UpProtocol.h"

CUpProtocol::CUpProtocol( int nPort, CChannelManager *pChnManager ) : CProtocol( nPort, pChnManager )
{
    m_iExitFlag = 0;
    pthread_mutex_init( &m_lock, NULL );
    m_net = NULL;
    m_pServer = NULL;
    m_pNetUdp = NULL;

    // 启动服务
    m_pServer = new CSyncNetServer( CUpProtocol::THServiceWorker , (void*)this );
    m_pServer->StartServer(UP_SERVER_PORT);
    
    // 启动UDP组播接收线程
    m_thDiscoveryWorker = 0;
    m_pNetUdp = new CNetUdp();
    m_pNetUdp->Open(UP_MULTI_PORT, (char*)UP_MULTI_ADDR, false);
    pthread_create(&m_thDiscoveryWorker, NULL, THDiscoveryWorker, (void*)this);
}
CUpProtocol::~CUpProtocol()
{
    m_iExitFlag = 1;
    m_net = NULL;

    if( m_thDiscoveryWorker != 0 )
    {
        pthread_join(m_thDiscoveryWorker, NULL);
        m_thDiscoveryWorker = 0;
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
}

void CUpProtocol::THServiceWorker(CSyncServerNetEnd *net, void *p)
{
    if(p == NULL)
    {
        _DEBUG_("p==NULL! return");
        return ;
    }
    CUpProtocol *pThis = ( CUpProtocol*)p;

    if(pThis->m_net != NULL)
    {
        _DEBUG_("connected by [%s], ingore [%s]", pThis->m_net->getRemoteAddr(), net->getRemoteAddr());
        net->Close();
        return;
    }

	_DEBUG_("client connected: %s", net->getRemoteAddr());

    pThis->m_net = net;
    struUpHead* phead = new struUpHead;
    long ret = 0;
    while( pThis->m_iExitFlag == 0 )
    {
        if ( !net->IsOK() ) 
        {
            _DEBUG_("net not ok!");
            break;
        }
        ret = pThis->GetMsgHead(net, phead);
        if(ret != 0)
        {
            continue;
            CFacility::SleepMsec(100);
        }
        ret = pThis->ProcessMsg(net, phead);
    }
    if(phead)
    {
        delete phead;
        phead = NULL;
    }
    pThis->m_net = NULL;
    _DEBUG_("client disconnected: %s", net->getRemoteAddr());

}

void* CUpProtocol::THDiscoveryWorker(void* param)
{
    if(param == NULL)
    {
        return NULL;
    }
    _DEBUG_("start!");
    CUpProtocol *pThis = ( CUpProtocol*)param;
    char tmp[1024]={0};
    int ret = 0;
    struUpHead head;
	while( pThis->m_iExitFlag == 0 )
	{
		memset(tmp, 0, sizeof(tmp));
		ret = pThis->m_pNetUdp->GetDataNT( tmp, 1024, 100 );
		if ( ret >= sizeof(struUpHead) )
		{
			memcpy(&head, tmp, sizeof(struUpHead));
            if ( pThis->CheckHead(&head) )
            {
                pThis->ProcessCmd(pThis->m_pNetUdp, &head);
            }
            else
            {
                _DEBUG_("check header error");
            }
		}
		else if ( ret < 0 )
        {
            break;
        }
        else
        {
            continue;
        }
    }
    _DEBUG_("exit!");
    return NULL;
}

long CUpProtocol::BuildHead(struUpHead *head)
{
	//memset(head, 0, sizeof(struUpHead));
	head->version = UP_VERSION;
    sprintf(head->flag, "%s", UP_HEAD_FLAG);
	return 0;
}

long CUpProtocol::SendMsg(CSyncNetEnd *net, void* buf, int len)
{
	_DEBUG_("send msg to [%s] >>>>>>>>>>>>>>>>>>", net->getRemoteAddr());
    try
	{
		net->Send( buf, len);
		net->Flush();
        _DEBUG_("send ok.");
		return 0;
	}
	catch(...)
	{
	    _DEBUG_("send failed!");
		return -1;
	}
    return 0;
}

long CUpProtocol::GetMsgHead( CSyncNetEnd *net, struUpHead *head, int timeout)
{
	int iHeadLen = sizeof(struUpHead);
    int iRecvLen = 0;
	if ( timeout==0 )
    {   
		iRecvLen = net->RecvN( (void*)head, iHeadLen );
    }
	else
    {   
		iRecvLen = net->RecvNT( (void*)head, iHeadLen, timeout );
    }
    if(iRecvLen < 0 )
    {
        return -1;
    }
    if(iRecvLen == 0)
    {
        return -1;
    }
	if ( iRecvLen != iHeadLen )
	{
		_DEBUG_("receive header error");
		return -1;
	}

	if ( ! CheckHead( head ) )
	{
		//_DEBUG_("check header error");
		return -1;
	}
    _DEBUG_("receive header ok");
	return 0;
}

long CUpProtocol::GetMsgBody(CSyncNetEnd *net, struUpHead *head, char* body, int len, int timeout)
{
	int length = head->length;
    if( length != len )
    {
        _DEBUG_("body len error, not [%d], should be [%d]", len, length);
        return -1;
    }
    int iRecvLen = 0;
	if ( timeout==0 )
    {   
		iRecvLen = net->RecvNT_R( (void*)body, len, 100);
    }
	else
    {   
		iRecvLen = net->RecvNT_R( (void*)body, len, timeout );
    }
    if(iRecvLen < 0)
    {
        return -1;
    }
    if(iRecvLen == 0)
    {
        return -1;
    }

	if ( iRecvLen != len )
	{
		_DEBUG_("receive body error, receive %d, should be %d", iRecvLen, len);
		return -1;
	}
    _DEBUG_("receive body ok");
    return 0;
}

long CUpProtocol::ProcessMsg(CSyncNetEnd *net, struUpHead *head)
{
    if(head == NULL)
        return -1;
    int cmd = head->cmd;
    long lRet = 0;
    switch (cmd)
    {
        case UP_MSG_REQ_UPGRADE_DEV:
            lRet = OnReqUpgradeDev(net, head);
            break;
        case UP_MSG_REQ_REBOOT_DEV:
            lRet = OnReqRebootDev(net, head);
            break;
        case UP_MSG_REQ_SETTIME:
            lRet = OnReqSettime(net, head);
            break;
        default:
            _DEBUG_("unknown message type [0x%x]!", cmd);
            lRet = -1;
            break;
    };
    return lRet;
}

bool CUpProtocol::CheckHead(struUpHead *head)
{
    if(head == NULL) 
        return false;
	if( head->version != (unsigned int)UP_VERSION || 
        strncmp(UP_HEAD_FLAG, head->flag, strlen(UP_HEAD_FLAG)) != 0
    )
    {
        _DEBUG_("Check head error, version:%x!=%x, flag:%s!=%s", head->version, UP_VERSION, head->flag, UP_HEAD_FLAG);
        return false;
    }

	return true;
}
long CUpProtocol::ProcessCmd(CNetUdp *net, struUpHead *head)
{
    if(head == NULL)
        return -1;
    int cmd = head->cmd;
    long lRet = 0;
    switch (cmd)
    {
        case UP_CMD_REQ_DETECT_DEVICES:
            lRet = OnCmdReqDetectDev(net, head);
            break;

        default:
            _DEBUG_("unknown command [0x%x]!", cmd);
            lRet = -1;
            break;
    };
    return lRet;
}

long CUpProtocol::OnCmdReqDetectDev(CNetUdp *net, struUpHead *head)
{
    long lRet = 0;
    struUpCmdRetDetectDev retmsg;    
    memset(&retmsg, 0, sizeof(retmsg));

    // 消息头
    memcpy(&retmsg.head, head, sizeof(struUpHead));
    retmsg.head.cmd = UP_CMD_RET_DETECT_DEVICES;
    retmsg.head.length = sizeof(struUpDevInfo);
    
    // 消息体
    m_pChnManager->m_pDecSet->GetHostname(retmsg.body.szDevName, (void*)m_pChnManager->m_pDecSet);
    //_DEBUG_("device name: %s", retmsg.body.szDevName);
    m_pChnManager->m_pDecSet->GetDeviceVer(retmsg.body.szSysVersion, (void*)m_pChnManager->m_pDecSet);
    //_DEBUG_("device buildtime: %s", retmsg.body.szSysVersion);
    m_pChnManager->m_pDecSet->GetDevBuildtime(retmsg.body.szBuildtime, (void*)m_pChnManager->m_pDecSet);
    //_DEBUG_("device buildtime: %s", retmsg.body.szBuildtime);
    CFacility::GetIP(retmsg.body.szDevIp);
    //_DEBUG_("device ip: %s", retmsg.body.szDevIp);
    struNetworkInfo sNI;
    memset(&sNI, 0, sizeof(struNetworkInfo));
    m_pChnManager->m_pDecSet->GetNetworkInfo(&sNI, (void*)m_pChnManager->m_pDecSet);
    strncpy(retmsg.body.szDevNetmask, sNI.netmask, sizeof(retmsg.body.szDevNetmask));
    //_DEBUG_("device netmask: %s", retmsg.body.szDevNetmask);
    strncpy(retmsg.body.szDevGateway, sNI.gateway, sizeof(retmsg.body.szDevGateway));
    //_DEBUG_("device gateway: %s", retmsg.body.szDevGateway);
    unsigned char ucMAC[6] = {0};
    CFacility::GetMAC(ucMAC);
    sprintf(retmsg.body.szDevMac, "%02x:%02x:%02x:%02x:%02x:%02x", ucMAC[0], ucMAC[1], \
                                    ucMAC[2], ucMAC[3], ucMAC[4], ucMAC[5]);
    //_DEBUG_("device MAC: %s", retmsg.body.szDevMac);
    if(m_net == NULL)
    {
        retmsg.body.iCtlFlag = 0;
        //_DEBUG_("device not be connected");
    }
    else
    {
        retmsg.body.iCtlFlag = 1;
        strncpy(retmsg.body.szCtlIp, m_net->getRemoteAddr(), sizeof(retmsg.body.szCtlIp));
        //_DEBUG_("device connected by %s", retmsg.body.szCtlIp);
    }
    // 发送数据
    lRet = net->SendData((void *)&retmsg, sizeof(struUpCmdRetDetectDev));
    if(lRet == sizeof(struUpCmdRetDetectDev))
    {
        _DEBUG_("UP_CMD_RET_DETECT_DEVICES: send ok.");
        return 0;
    }
    _DEBUG_("UP_CMD_RET_DETECT_DEVICES: send failed!");
    return -1;
}

long CUpProtocol::OnReqUpgradeDev(CSyncNetEnd *net, struUpHead *head)
{
    long lRet = 0;
    struUpRetUpgradeDev retmsg;
    char* szData = new char[head->length];
    if(szData == NULL)
    {
        _DEBUG_("allocate memory error!\n");
        exit(1);
    }

    memset(&retmsg, 0, sizeof(struUpRetUpgradeDev));
    memcpy(&retmsg.head, head, sizeof(struUpHead));
    retmsg.head.cmd = UP_MSG_RET_UPGRADE_DEV;
    retmsg.head.length = sizeof(int);
    retmsg.iRet = UP_E_FAILED;

    // 获取升级数据
    lRet = GetMsgBody(net, head, szData, head->length, 3000);
    if(lRet == 0)
    {
        lRet = m_pChnManager->m_pDecSet->UpgradeDevice(szData, (unsigned long)head->length, (void*)m_pChnManager->m_pDecSet);
        if(lRet == 0)
        {
            retmsg.iRet = UP_E_SUCCESS;
            _DEBUG_("upgrade ok.");
        }
        else
        {
            _DEBUG_("upgrade falied!");
        }
    }
    else
    {
        _DEBUG_("upgrade falied!");
    }
        
    lRet = SendMsg(net, (void *)&retmsg, sizeof(struUpRetUpgradeDev));
    PTR_DELETE_A(szData);

    return lRet;
}

long CUpProtocol::OnReqRebootDev(CSyncNetEnd *net, struUpHead *head)
{
    long lRet = 0;
    struUpRetRebootDev retmsg;
    memset(&retmsg, 0, sizeof(struUpRetRebootDev));
    memcpy(&retmsg.head, head, sizeof(struUpHead));
    retmsg.head.cmd = UP_MSG_RET_REBOOT_DEV;
    retmsg.head.length = sizeof(int);
    retmsg.iRet = UP_E_SUCCESS;
    lRet = SendMsg(net, (void *)&retmsg, sizeof(struUpRetRebootDev));

    // 重启
    lRet = system("/sbin/reboot");
    _DEBUG_("reboot ok");
    return lRet;
}

long CUpProtocol::OnReqSettime(CSyncNetEnd *net, struUpHead *head)
{
    long lRet = 0;
    struUpRetSettime retmsg;
    char szTime[80] = {0};

    memset(&retmsg, 0, sizeof(struUpRetSettime));
    memcpy(&retmsg.head, head, sizeof(struUpHead));
    retmsg.head.cmd = UP_MSG_RET_SETTIME;
    retmsg.head.length = sizeof(int);
    retmsg.iRet = UP_E_FAILED;

    // 获取时间字符串
    lRet = GetMsgBody(net, head, szTime, head->length);
    if(lRet == 0)
    {
        //"2015-02-10 18:30:59"
        _DEBUG_("set system time from [%s] to [%s]", CFacility::GetCurTime(), szTime);
        lRet = m_pChnManager->m_pDecSet->SetSysTime(szTime, (void*)m_pChnManager->m_pDecSet);
        if(lRet == 0)
        {
            retmsg.iRet = UP_E_SUCCESS;
            _DEBUG_("set system time ok.");
        }
        else
        {
            _DEBUG_("set system time failed!");
        }
    }
    else
    {
        //_DEBUG_("");
    }

    lRet = SendMsg(net, (void *)&retmsg, sizeof(struUpRetSettime));
    return lRet;
}




