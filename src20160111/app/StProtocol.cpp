#include "StProtocol.h"

CStProtocol::CStProtocol( int nPort, CChannelManager *pChnManager )  : CProtocol( nPort, pChnManager )
{
    m_bInitedForEnc = false;
    m_iExitFlag = 0;
    m_iChnno = 0;
    pthread_mutex_init( &m_lock, NULL );
    m_pServer = new CSyncNetServer( CStProtocol::THWorker , (void*)this );
    m_pServer->StartServer(nPort);
    InitForEnc();
}

CStProtocol::~CStProtocol() 
{
    UninitForEnc();
    m_iExitFlag = 1;
    if ( m_pServer != NULL)
	{
	    m_pServer->StopServer();
		delete m_pServer;
		m_pServer = NULL;
	}
    pthread_mutex_destroy( &m_lock );
}

int CStProtocol::StopEncoder(int chnno)
{
    _DEBUG_(" CALLED [%d]!", chnno);
    if( *m_szEncIp && m_iChnno==chnno )
    {
        _DEBUG_(" stop encoder [%s]", m_szEncIp);
        OnDecLogoutEnc();
        UninitForEnc();
        m_iChnno = 0;
    }
	else
	{
		
	}
    return 0;
}

long CStProtocol::InitForEnc()
{
    if(m_bInitedForEnc)
    {
        return 0;
    }
	m_iExitFlagForEnc = 0;
    m_iLastConnecedEncTime = 0;
    m_pClient = new CClientSyncNetEnd();
    memset(m_szEncIp, 0, sizeof(m_szEncIp));
    m_byteDecState = 0x00;
    memset(&m_sMulticastGroup, 0 ,sizeof(ReqVcSetMulticastGroup));
    memset(&m_sUnicastEnc, 0, sizeof(ReqVcSetUnicastEnc));
    memset(&m_sRelay, 0, sizeof(ReqVcSetRelay));
    m_bInitedForEnc = true;
    return 0;
}

long CStProtocol::UninitForEnc()
{
    if( ! m_bInitedForEnc )
    {
        return 0;
    }
	m_iExitFlagForEnc = 1;
    if( m_thKeepaliveWorker != 0 )
    {
        pthread_join(m_thKeepaliveWorker, NULL);
        m_thKeepaliveWorker = 0;
    }

	if ( m_pClient != NULL)
	{
	    m_pClient->Close();
		delete m_pClient;
		m_pClient = NULL;
	}
    memset(&m_sMulticastGroup, 0 ,sizeof(ReqVcSetMulticastGroup));
    memset(&m_sUnicastEnc, 0, sizeof(ReqVcSetUnicastEnc));
    memset(&m_sRelay, 0, sizeof(ReqVcSetRelay));
    m_byteDecState = 0x00;
    memset(m_szEncIp, 0, sizeof(m_szEncIp));
    m_bInitedForEnc = false;
    return 0;
}

void CStProtocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s", net->getRemoteAddr());
    if(p == NULL)
    {
        _DEBUG_("p==NULL! return");
        return ;
    }

    CStProtocol *pThis = ( CStProtocol*)p;
    struStHead* phead = new struStHead;
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
        }
        ret = pThis->ProcessMsg(net, phead);
    }
    if(phead)
    {
        delete phead;
        phead = NULL;
    }
    _DEBUG_("client disconnect:%s", net->getRemoteAddr());
}

long CStProtocol::GetMsgHead( CSyncNetEnd *net, struStHead *head, int timeout)
{
	int iHeadLen = sizeof(struStHead);
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

	if ( CheckHead( head ) < 0 )
	{
		_DEBUG_("check header error");
		return -1;
	}
    _DEBUG_("receive header ok");
	return 0;
}

long CStProtocol::GetMsgBody(CSyncNetEnd *net, struStHead *head, char* body, int len, int timeout)
{
	int length = head->length;
	length = changedata(length);
    if( length != (sizeof(struStHead)+len) )
    {
        _DEBUG_("body len error, [%d] should be [%d]", len, length-sizeof(struStHead));
        return -1;
    }
    int iRecvLen = 0;
	if ( timeout==0 )
    {   
		iRecvLen = net->RecvN( (void*)body, len );
    }
	else
    {   
		iRecvLen = net->RecvNT( (void*)body, len, timeout );
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
		_DEBUG_("receive body error");
		return -1;
	}
    _DEBUG_("receive body ok");
    return 0;
}

long CStProtocol::ProcessMsg(CSyncNetEnd *net, struStHead *head)
{
    if(head == NULL)return -1;
    int cmd = head->cmd;
	cmd = changedata(cmd);
    long lRet = 0;
    switch (cmd)
    {
        case REQ_VC_LOGIN_DECODER:
            lRet = OnReqVcLoginDec(net, head);
            break;
        case REQ_VC_LOGOUT_DECODER:
            lRet = OnReqVcLogoutDec(net, head);
            break;
        case REQ_VC_SET_DECODER_PROFILE:
            lRet = OnReqVcSetDecProfile(net, head);
            break;
        case REQ_VC_GET_DECODER_PROFILE:
            lRet = OnReqVcGetDecProfile(net, head);
            break;
        case REQ_VC_GET_DECODER_STATE:
            lRet = OnReqVcGetDecState(net, head);
            break;
		case REQ_VC_SET_UNICAST_ENCODER:
            lRet = OnReqVcSetUnicastEnc(net, head);
            break;
        case REQ_VC_GET_UNICAST_ENCODER:
            lRet = OnReqVcGetUnicastEnc(net, head);
            break;
        case REQ_VC_LOCK_DECODER_VIDEO:
            lRet = OnReqVcLockDecVideo(net, head);
            break;
        case REQ_VC_UNLOCK_DECODER_VIDEO:
            lRet = OnReqVcUnlockDecVideo(net, head);
            break;
		case REQ_VC_SET_MULTICAST_GROUP:
            lRet = OnReqVcSetMulticastGroup(net, head);
            break;
        case REQ_VC_SET_RELAY:
            lRet = OnReqVcSetRelay(net, head);
            break;
        default:
            _DEBUG_("unknown command [0x%x]!", cmd);
            lRet = -1;
            break;
    };
    return lRet;
}
long CStProtocol::OnReqVcLoginDec(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcLoginDec ret;
    };
    struct RETMSG sRetMsg;
    ReqVcLoginDec sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcLoginDec));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_LOGIN_DECODER);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcLoginDec));
    if(lRet == 0 )
    {
        if(check_ip((char*)sBody.szDecIp))
        {
            // 不作处理，默认登陆成功
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
            sprintf(m_szVcIp, "%s", net->getRemoteAddr());
			_DEBUG_("vc[%s] login.", net->getRemoteAddr());
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }
    return lRet;
}

long CStProtocol::OnReqVcLogoutDec(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcLogoutDec ret;
    };
    struct RETMSG sRetMsg;
    ReqVcLogoutDec sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcLogoutDec));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_LOGOUT_DECODER);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcLogoutDec));
    if(lRet == 0 )
    {
        //判断指令中的解码器地址是否和当前设备一致
        if(check_ip((char*)sBody.szDecIp))
        {
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01; 
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
            // 断开连接
            _DEBUG_("client[%s] logout.", net->getRemoteAddr());
            net->Close();
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }
    
    return lRet;
}

long CStProtocol::OnReqVcSetDecProfile(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcSetDecProfile ret;
    };
    struct RETMSG sRetMsg;
    ReqVcSetDecProfile sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcSetDecProfile));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_SET_DECODER_PROFILE);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcSetDecProfile));
    if(lRet == 0 )
    {
        //判断指令中的解码器地址是否和当前设备一致
        if(check_ip((char*)sBody.szDecIp))
        {
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
            // 指令处理(暂不处理2015-01-13)
            
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }    
    
    return lRet;
}

long CStProtocol::OnReqVcGetDecProfile(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcGetDecProfile ret;
    };
    struct RETMSG sRetMsg;
    ReqVcGetDecProfile sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcGetDecProfile));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_GET_DECODER_PROFILE);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcGetDecProfile));
    if(lRet == 0 )
    {
        //判断指令中的解码器地址是否和当前设备一致
        if(check_ip((char*)sBody.szDecIp))
        {
            // 指令处理(暂不处理2015-01-13)
            
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }

    return lRet;
}
    
long CStProtocol::OnReqVcGetDecState(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcGetDecState ret;
    };
    struct RETMSG sRetMsg;
    ReqVcGetDecState sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcGetDecState));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_GET_DECODER_STATE);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcGetDecState));
    if(lRet == 0 )
    {
        //判断指令中的解码器地址是否和当前设备一致
        if(check_ip((char*)sBody.szDecIp))
        {
            sRetMsg.ret.byteState = m_byteDecState;
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }

    return lRet;
}

long CStProtocol::OnReqVcSetUnicastEnc(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcSetUnicastEnc ret;
    };
    struct RETMSG sRetMsg;
    ReqVcSetUnicastEnc sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcSetUnicastEnc));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_SET_UNICAST_ENCODER);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcSetUnicastEnc));
    if(lRet == 0 )
    {
        //判断指令中的解码器地址是否和当前设备一致
        if(check_ip((char*)sBody.szDecIp))
        {
            // 指令处理
            lRet = OnSetUnicastEnc(&sBody);
            if(lRet == 0)
            {
                // 记录最后设置单播编码器时间
                m_iLastConnecedEncTime = (int)time(NULL);
                // 记录解码器单播连接编码器信息
                memcpy(&m_sUnicastEnc, &sBody, sizeof(ReqVcSetUnicastEnc));
                //记录状态
                m_byteDecState = 0x01;
                // 返回结果处理
                sRetMsg.ret.byteErr = 0x00;
                sRetMsg.ret.byteRet = 0x01;
            }
            else
            {
                // 返回结果处理
                sRetMsg.ret.byteErr = 0x00;
                sRetMsg.ret.byteRet = 0x00;
            }
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }
    
    return lRet;
}

long CStProtocol::OnReqVcGetUnicastEnc(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcGetUnicastEnc ret;
    };
    struct RETMSG sRetMsg;
    ReqVcGetUnicastEnc sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcGetUnicastEnc));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = changedata((int)RET_VC_GET_UNICAST_ENCODER);
    sRetMsg.head.length = changedata(htonl(sizeof(RETMSG)));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcGetUnicastEnc));
    if(lRet == 0 )
    {
        if(check_ip((char*)sBody.szDecIp))
        {
            // 指令处理
            memcpy(sRetMsg.ret.szEncId, m_sUnicastEnc.szEncId, 4*sizeof(char));
            memcpy(sRetMsg.ret.szEncIp, m_sUnicastEnc.szEncIp, 4*sizeof(char));
            memcpy(sRetMsg.ret.szTcpPort, &m_sUnicastEnc.usTcpPort, 2*sizeof(char));
            memcpy(sRetMsg.ret.szUdpPort, &m_sUnicastEnc.usUdpPort, 2*sizeof(char));
            int iTime = 0;
            if(m_iLastConnecedEncTime!=0)
            {
                iTime = (int)time(NULL);
                iTime -= m_iLastConnecedEncTime;
            }
			//changedata(iTime);
            memcpy(sRetMsg.ret.szConnectedTime, &iTime, 4*sizeof(char));
            
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }
    
    return lRet;
}

long CStProtocol::OnReqVcLockDecVideo(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcLockDecVideo ret;
    };
    struct RETMSG sRetMsg;
    ReqVcLockDecVideo sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcLockDecVideo));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = RET_VC_LOCK_DECODER_VIDEO;
    sRetMsg.head.length = htonl(sizeof(RETMSG));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcLockDecVideo));
    if(lRet == 0 )
    {
        if(check_ip((char*)sBody.szDecIp))
        {
            // 返回结果处理
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
            
            // 指令处理(暂不处理2015-01-13)
            
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));

        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }
    
    return lRet;
}

long CStProtocol::OnReqVcUnlockDecVideo(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcUnlockDecVideo ret;
    };
    struct RETMSG sRetMsg;
    ReqVcUnlockDecVideo sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcUnlockDecVideo));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = RET_VC_UNLOCK_DECODER_VIDEO;
    sRetMsg.head.length = htonl(sizeof(RETMSG));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcUnlockDecVideo));
    if(lRet == 0 )
    {
        if(check_ip((char*)sBody.szDecIp))
        {
            // 返回结果处理
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
            
            // 指令处理(暂不处理2015-01-13)
            
            //返回处理结果
            lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
        }
        else
        {
            //非本设备指令，直接退出
            _DEBUG_("dec ip not this device, return.");
            return -1;
        }
    }
    else
    {
        _DEBUG_("get msg body error!");
    }

    //返回处理结果
    lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
    
    return lRet;
}
    
long CStProtocol::OnReqVcSetMulticastGroup(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcSetMulticastGroup ret;
    };
    struct RETMSG sRetMsg;
    ReqVcSetMulticastGroup sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcSetMulticastGroup));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = RET_VC_SET_MULTICAST_GROUP;
    sRetMsg.head.length = htonl(sizeof(RETMSG));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcSetMulticastGroup));
    if(lRet == 0 )
    {
        // 指令处理
        lRet = OnSetMulticast(&sBody);
        if(lRet == 0)
        {
            // 返回结果处理
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
            // 记录解码器加入组播信息
            memcpy(&m_sMulticastGroup, &sBody, sizeof(ReqVcSetMulticastGroup));
            //记录状态
            m_byteDecState = 0x02;
        }
        else
        {
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x00;
        }
        lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
    }
    else
    {
        _DEBUG_("get msg body error!");
    }

    return lRet;
}

long CStProtocol::OnReqVcSetRelay(CSyncNetEnd *net, struStHead *head)
{
    long lRet = 0;
    struct RETMSG
    {
        struStHead head;
        RetVcSetRelay ret;
    };
    struct RETMSG sRetMsg;
    ReqVcSetRelay sBody;
    memset(&sRetMsg, 0, sizeof(RETMSG));
    memset(&sBody, 0, sizeof(ReqVcSetRelay));
    
    memcpy(&sRetMsg.head, head, sizeof(struStHead));
    sRetMsg.head.cmd = RET_VC_SET_RELAY;
    sRetMsg.head.length = htonl(sizeof(RETMSG));

    lRet = GetMsgBody(net, head, (char*)&sBody, sizeof(ReqVcSetRelay));
    if(lRet == 0 )
    {
        lRet = OnSetRelay(&sBody);
        if(lRet == 0)
        {
            // 返回结果处理
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x01;
        }
        else
        {
            sRetMsg.ret.byteErr = 0x00;
            sRetMsg.ret.byteRet = 0x00;
        }
        lRet = SendMsg(net, (void *)&sRetMsg, sizeof(RETMSG));
    }
    else
    {
        _DEBUG_("get msg body error!");
    }

    return lRet;
}

long CStProtocol::OnSetUnicastEnc(ReqVcSetUnicastEnc* p)
{
    long lRet = 0;
    unsigned int uiEncIp = 0;
    char szEncIp[64] = {0};
	char szDecIp[64] = {0};
    struct in_addr in_enc;
    // 停止解码
    lRet = StopDecoder(1);
    m_iChnno = 0;
    // 断开编码器输出连接
    lRet = OnDecDisconnectEnc();
    // 登出
    lRet = OnDecLogoutEnc();
    // 清除信息
    lRet = UninitForEnc();
    // 初始化信息
    lRet = InitForEnc();
    
    // 获取编码器IP
    memcpy(&uiEncIp, p->szEncIp, sizeof(unsigned int));
    in_enc.s_addr = uiEncIp;
    sprintf( szEncIp, "%s", inet_ntoa(in_enc));
    
    // 解码器登录编码器
    sprintf(m_szEncIp, "%s", szEncIp);
    lRet = OnDecLoginEnc(szEncIp, (int)ENCODER_TCP_PORT);
    if(lRet != 0)
    {
        // 清除信息
        lRet = UninitForEnc();
        return lRet;
    }
	// 编码器心跳
	pthread_create(&m_thKeepaliveWorker, NULL, THKeepaliveWorker, (void*)this);
    // 连接编码器输出
    lRet = OnDecConnectEnc();
    // 启动解码
    get_ip(szDecIp);
    lRet = StartDecoderUdp(1, (char*)szDecIp, DECODER_UDP_PORT);
    m_iChnno = 1;
    return lRet;
}
long CStProtocol::OnSetMulticast(ReqVcSetMulticastGroup* p)
{
    long lRet = 0;
    unsigned int uiMulticastIp = 0;
    char szMulticastIp[64] = {0};
    struct in_addr in_enc;
    // 停止解码
    lRet = StopDecoder(1);
    m_iChnno = 0;
    // 断开编码器输出连接
    lRet = OnDecDisconnectEnc();
    // 登出
    lRet = OnDecLogoutEnc();
    // 清除信息
    lRet = UninitForEnc();
    // 初始化信息
    lRet = InitForEnc();
    
    // 获取组播IP
    memcpy(&uiMulticastIp, p->szMulticastIp, sizeof(unsigned int));
    in_enc.s_addr = uiMulticastIp;
    sprintf( szMulticastIp, "%s", inet_ntoa(in_enc));

    // 加入组播解码
    lRet = StartDecoderUdp(1, szMulticastIp, DECODER_UDP_PORT);
    m_iChnno = 1;
    return lRet;
}
long CStProtocol::OnSetRelay(ReqVcSetRelay* p)
{
    
    return 0;
}

bool CStProtocol::check_ip(char* ip)
{
	//暂不判断
	return true;
	
    char szMyIp[64] = {0};
    unsigned int uiMyIp = 0;
    unsigned int uiIp = 0;
    memcpy(&uiIp, ip, sizeof(unsigned int));
    get_ip(szMyIp);
    struct in_addr in;
    inet_aton(szMyIp, &in);
    uiMyIp = in.s_addr;
    if(uiIp == uiMyIp)
    {
        return true;
    }
    return false;
}
int  CStProtocol::changedata(int cmd)
{
	int tmpcmd = 0;
	char szcmd[4] = {0};
	szcmd[0]=(char)((cmd&0xff000000)>>24);
  	szcmd[1]=(char)((cmd&0x00ff0000)>>16);
  	szcmd[2]=(char)((cmd&0x0000ff00)>>8);
	szcmd[3]=(char)(cmd&0x000000ff);
	memcpy(&tmpcmd, szcmd, 4);
	return tmpcmd;
}
short CStProtocol::changedatas(short cmd)
{
	short tmpcmd = 0;
	char szcmd[2] = {0};
	szcmd[0]=(char)((cmd&0xff00)>>8);
	szcmd[1]=(char)(cmd&0x00ff);
	memcpy(&tmpcmd, szcmd, 2);
	return tmpcmd;
}

long CStProtocol::CheckHead(struStHead* head)
{
    return 0;
	//if (head->version != CMD_VERSION)
	if (head->version != 0x00 || head->reserved[0] != 0x20)
	{
		_DEBUG_( "check version is 0x%x, not 0x%x, failed!", head->version, (char)CMD_VERSION);
		return -1;
	}
	
	return 0;
}

long CStProtocol::BuildHead(struStHead *head)
{
	memset(head, 0, sizeof(struStHead));
	head->version = 0x00;//CMD_VERSION;
    head->reserved[0]=0x20;
	return 0;
}

long CStProtocol::SendMsg(CSyncNetEnd *net, void* buf, int len)
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

void* CStProtocol::THKeepaliveWorker(void *param)
{
    if(param == NULL)
    {
        return NULL;
    }
    _DEBUG_("start!");
    CStProtocol *pThis = (CStProtocol*)param;
    // 心跳数据
    struct HEARTBEAT
    {
        struStHead head;
        MsgEncKeepaliveDec body;
    };
    int iRet = 0;
    int iTime = 0;
    char szSrcIp[64] = {0};
    unsigned int uiSrcIp = 0;
    unsigned int uiDstIp = 0;
    pThis->get_ip(szSrcIp);
    struct in_addr src;
    struct in_addr dst;
    struct HEARTBEAT hb;
    inet_aton(szSrcIp, &src);
    inet_aton(pThis->m_szEncIp, &dst);
    uiSrcIp = src.s_addr;
    uiDstIp = dst.s_addr;
    //uiSrcIp=htonl(uiSrcIp);
    //uiDstIp=htonl(uiDstIp);
    memset(&hb, 0, sizeof(HEARTBEAT));
    pThis->BuildHead(&hb.head);
    hb.head.length = pThis->changedata(htonl(sizeof(HEARTBEAT)));
    hb.head.workid = pThis->changedata(htonl((int)time(NULL)));
    hb.head.flag = 0x01;
	hb.head.cmd = pThis->changedata((int)(MSG_ENCODER_KEEPALIVE_DECODER));
    memcpy(hb.head.src, &uiSrcIp, 4*sizeof(char));
    memcpy(hb.head.dst, &uiDstIp, 4*sizeof(char));
    memcpy(hb.body.szDecIp, &uiSrcIp, 4*sizeof(char));
    memcpy(hb.body.szEncIp, &uiDstIp, 4*sizeof(char));

	while( pThis->m_iExitFlagForEnc == 0 )
	{
		// 发送心跳
		iRet = pThis->m_pClient->Send((void*)&hb, sizeof(HEARTBEAT));
        if(iRet < 0)
        {
            _DEBUG_("Send heartbeat to [%s] failed!", pThis->m_szEncIp);
            CFacility::SleepSec(1);
            continue;
        }
		//_DEBUG_("Send heartbeat to [%s] ok.", pThis->m_szEncIp);
		
        // 每隔3秒
        while(pThis->m_iExitFlagForEnc == 0 && iTime < KEEPALIVE_INTERVAL)
        {
            iTime++;
            CFacility::SleepSec(1);
        }
        iTime = 0;
    }
    _DEBUG_("exit!");
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
long CStProtocol::StartDecoderUdp(int iChnno, char* szIp, int iPort)
{
    int iFps = 30;
    char szUrl[200]={0};
    sprintf( szUrl, "udp/%s:%d", szIp, iPort );
    _DEBUG_( "start decoder udp chann:%d url:%s", iChnno, szUrl);
    m_pChnManager->SaveUrl(iChnno, szUrl, iFps);
    m_pChnManager->Open( iChnno, szUrl, 0xff, iFps);
    return 0;
}

long CStProtocol::StartDecoderTcp(int iChnno, char* szIp, int iPort)
{
    
    return 0;
}

long CStProtocol::StopDecoder(int iChnno)
{
    m_pChnManager->Close(iChnno);
    return 0;
}

long CStProtocol::OnDecLoginEnc(char* szIp, int iPort)
{
    long lRet = 0;
    unsigned int uiDecIp = 0;
    unsigned int uiEncIp = 0;
    char szDecIp[64] = {0};
    struct in_addr dec;
    struct in_addr enc;
    struct REQMSG
    {
        struStHead head;
        ReqDecLoginEnc req;
    };
    struct RETMSG
    {
        struStHead head;
        RetDecLoginEnc ret;
    };
    REQMSG sReqMsg;
    RETMSG sRetMsg;
    memset(&sReqMsg, 0, sizeof(REQMSG));
    // 建立连接
    if( ! m_pClient->Connect(szIp, iPort, 1500) )
    {
        _DEBUG_("connect to [%s:%d] failed!", szIp, iPort);
        return -1;
    }

    // 组装消息头
    BuildHead(&sReqMsg.head);
    sReqMsg.head.cmd = changedata((int)REQ_DECODER_LOGIN_ENCODER);
    sReqMsg.head.flag = 0x01;
    sReqMsg.head.length = changedata(htonl(sizeof(REQMSG)));
    sReqMsg.head.workid = changedata(htonl((int)time(NULL)));

    // 发送登陆指令
    get_ip(szDecIp);
    inet_aton(szDecIp, &dec);
    inet_aton(szIp, &enc);
    uiDecIp = dec.s_addr;
    uiEncIp = enc.s_addr;
    memcpy(sReqMsg.req.szDecIp, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szEncIp, &uiEncIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.src, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.dst, &uiEncIp, sizeof(unsigned int));
    lRet = SendMsgToEnc((void*)&sReqMsg, sizeof(REQMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    // 接收登陆返回信息
    lRet = GetMsgFromEnc((void*)&sRetMsg, sizeof(RETMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    if( sRetMsg.ret.byteRet != 0x01)
    {
        _DEBUG_("decoder login encoder[%s] failed!", szIp);
        return -1;
    }
    else
    {
        _DEBUG_("decoder login encoder[%s] ok!", szIp);
    }
    return lRet;
}

long CStProtocol::OnDecLogoutEnc()
{
    long lRet = 0;
    unsigned int uiDecIp = 0;
    unsigned int uiEncIp = 0;
    char szDecIp[64] = {0};
    struct in_addr dec;
    struct in_addr enc;
    struct REQMSG
    {
        struStHead head;
        ReqDecLogoutEnc req;
    };
    struct RETMSG
    {
        struStHead head;
        RetDecLogoutEnc ret;
    };
    REQMSG sReqMsg;
    RETMSG sRetMsg;
	memset(&sReqMsg, 0, sizeof(REQMSG));

    // 组装消息头
    BuildHead(&sReqMsg.head);
	sReqMsg.head.cmd = changedata((int)REQ_DECODER_LOGOUT_ENCODER);
    sReqMsg.head.flag = 0x01;
    sReqMsg.head.length = changedata(htonl(sizeof(REQMSG)));
    sReqMsg.head.workid = changedata(htonl((int)time(NULL)));

    // 发送指令
    
    get_ip(szDecIp);
    inet_aton(szDecIp, &dec);
    inet_aton(m_szEncIp, &enc);
    uiDecIp = dec.s_addr;
    uiEncIp = enc.s_addr;
    memcpy(sReqMsg.req.szDecIp, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szEncIp, &uiEncIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.src, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.dst, &uiEncIp, sizeof(unsigned int));
    lRet = SendMsgToEnc((void*)&sReqMsg, sizeof(REQMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    // 接收返回信息
    lRet = GetMsgFromEnc((void*)&sRetMsg, sizeof(RETMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    if( sRetMsg.ret.byteRet != 0x01)
    {
        _DEBUG_("decoder logout encoder[%s] failed!", m_szEncIp);
        return -1;
    }
    else
    {
        _DEBUG_("decoder logout encoder[%s] ok!", m_szEncIp);
    }
    return lRet;
}

long CStProtocol::OnDecConnectEnc()
{
    long lRet = 0;
    unsigned int uiDecIp = 0;
    unsigned int uiEncIp = 0;
    char szDecIp[64] = {0};
    unsigned short usPort = changedatas((short)DECODER_UDP_PORT);
    struct in_addr dec;
    struct in_addr enc;
    struct REQMSG
    {
        struStHead head;
        ReqDecConnectEnc req;
    };
    struct RETMSG
    {
        struStHead head;
        RetDecConnectEnc ret;
    };
    REQMSG sReqMsg;
    RETMSG sRetMsg;
	memset(&sReqMsg, 0, sizeof(REQMSG));

    // 组装消息头
    BuildHead(&sReqMsg.head);
	sReqMsg.head.cmd = changedata((int)REQ_DECODER_CONNECT_ENCODER);
    sReqMsg.head.flag = 0x01;
    sReqMsg.head.length = changedata(htonl(sizeof(REQMSG)));
    sReqMsg.head.workid = changedata(htonl((int)time(NULL)));

    // 发送指令
    
    get_ip(szDecIp);
    inet_aton(szDecIp, &dec);
    inet_aton(m_szEncIp, &enc);
    uiDecIp = dec.s_addr;
    uiEncIp = enc.s_addr;
    memcpy(sReqMsg.req.szDecIp, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szEncIp, &uiEncIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szUdpPort, &usPort, sizeof(unsigned short));
    memcpy(sReqMsg.head.src, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.dst, &uiEncIp, sizeof(unsigned int));
    lRet = SendMsgToEnc((void*)&sReqMsg, sizeof(REQMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    // 接收返回信息
    lRet = GetMsgFromEnc((void*)&sRetMsg, sizeof(RETMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    if( sRetMsg.ret.byteRet != 0x01)
    {
        _DEBUG_("decoder connect encoder[%s] failed!", m_szEncIp);
        return -1;
    }
    else
    {
        _DEBUG_("decoder connect encoder[%s] ok!", m_szEncIp);
    }
    return lRet;
}

long CStProtocol::OnDecDisconnectEnc()
{
    long lRet = 0;
    unsigned int uiDecIp = 0;
    unsigned int uiEncIp = 0;
    unsigned int uiVcIp = 0;
    char szDecIp[64] = {0};
    unsigned short usPort = changedatas((short)DECODER_UDP_PORT);
    struct in_addr dec;
    struct in_addr enc;
    struct in_addr vc;
    struct REQMSG
    {
        struStHead head;
        ReqDecDisconnectEnc req;
    };
    struct RETMSG
    {
        struStHead head;
        RetDecDisconnectEnc ret;
    };
    REQMSG sReqMsg;
    RETMSG sRetMsg;
	memset(&sReqMsg, 0, sizeof(REQMSG));

    // 组装消息头
    BuildHead(&sReqMsg.head);
	sReqMsg.head.cmd = changedata((int)REQ_DECODER_DISCONNECT_ENCODER);
    sReqMsg.head.flag = 0x01;
    sReqMsg.head.length = changedata(htonl(sizeof(REQMSG)));
    sReqMsg.head.workid = changedata(htonl((int)time(NULL)));

    // 发送指令
    
    get_ip(szDecIp);
    inet_aton(szDecIp, &dec);
    inet_aton(m_szEncIp, &enc);
    inet_aton(m_szVcIp, &vc);
    uiDecIp = dec.s_addr;
    uiEncIp = enc.s_addr;
    uiVcIp  = vc.s_addr;
    memcpy(sReqMsg.req.szDecIp, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szEncIp, &uiEncIp, sizeof(unsigned int));
    memcpy(sReqMsg.req.szVcIp,  &uiVcIp,  sizeof(unsigned int));
    memcpy(sReqMsg.head.src, &uiDecIp, sizeof(unsigned int));
    memcpy(sReqMsg.head.dst, &uiEncIp, sizeof(unsigned int));
    lRet = SendMsgToEnc((void*)&sReqMsg, sizeof(REQMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    // 接收返回信息
    lRet = GetMsgFromEnc((void*)&sRetMsg, sizeof(RETMSG));
    if(lRet != 0)
    {
        return lRet;
    }
    if( sRetMsg.ret.byteRet != 0x01)
    {
        _DEBUG_("decoder disconnect encoder[%s] failed!", m_szEncIp);
        return -1;
    }
    else
    {
        _DEBUG_("decoder disconnect encoder[%s] ok!", m_szEncIp);
    }
    return lRet;
}

long CStProtocol::SendMsgToEnc(void* buf, int len)
{
    long lRet = 0;
    if(m_pClient == NULL)
    {
        _DEBUG_("client obj==NULL!");
        return -1;
    }
    lRet = m_pClient->Send( buf, len);
    if(lRet < 0)
    {
        _DEBUG_("send data to [%s] failed!", m_szEncIp);
        return -1;
    }
    _DEBUG_("send data to [%s] ok.", m_szEncIp);
    return 0;
}

long CStProtocol::GetMsgFromEnc(void* buf, int len)
{
    long lRet = 0;
    if(m_pClient == NULL)
    {
        _DEBUG_("client obj==NULL!");
        return -1;
    }
    lRet = m_pClient->RecvT(buf, len, 1000);
    if(lRet <= 0 )
    {
    	_DEBUG_("recv data form enc failed!");
        return -1;
    }
    if(lRet != len)
    {
        _DEBUG_("get data from [%s] error!", m_szEncIp);
        return -1;
    }
    _DEBUG_("get data from [%s] ok.", m_szEncIp);
    return 0;
}








