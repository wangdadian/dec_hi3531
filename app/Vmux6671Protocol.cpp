#include "Vmux6671Protocol.h"

CVmux6671Protocol::CVmux6671Protocol(int nPort, CChannelManager *pChnManager): CProtocol(nPort, pChnManager)
{
    m_iExitFlag = 0;
    m_pServer = NULL;
    pthread_mutex_init( &m_lock, NULL );
    // 启动服务
    m_pServer = new CSyncNetServer( CVmux6671Protocol::THWorker , (void*)this );
    m_pServer->StartServer(nPort);
}

CVmux6671Protocol::~CVmux6671Protocol()
{
    m_iExitFlag = 1;
    if ( m_pServer != NULL)
	{
	    m_pServer->StopServer();
		PTR_DELETE(m_pServer);
	}


    pthread_mutex_destroy( &m_lock );
}

int CVmux6671Protocol::StopEncoder(int chnno)
{
    return 0;
}

void CVmux6671Protocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s", net->getRemoteAddr());
    if(p == NULL)
    {
        _DEBUG_("p==NULL! return");
        return ;
    }

    CVmux6671Protocol *pThis = ( CVmux6671Protocol*)p;
    long ret = 0;
    while( pThis->m_iExitFlag == 0 )
    {
        if ( !net->IsOK() ) 
        {
            _DEBUG_("net not ok!");
            break;
        }
        VMUX6KHEAD head;
        ret = pThis->GetMsgHead(net, &head);
        if(ret != 0)
        {
            continue;
        }
        ret = pThis->ProcessMsg(net, &head);
    }
    _DEBUG_("client disconnect:%s", net->getRemoteAddr());

}

long CVmux6671Protocol::ProcessMsg(CSyncNetEnd *net, VMUX6KHEAD *head)
{
    VMUX6KHEAD retHead;
    CMDRESPONSE retBody;
    long lRet = -1;
    int cmd = 0;
    int iReqBodyLen = (int)(head->bLength_h<<8) + head->bLength_l;
    int iRetBodyLen = sizeof(CMDRESPONSE);
    int iHeadLen = sizeof(VMUX6KHEAD);
    int iSendLen = iHeadLen + iRetBodyLen;
    char* szSendBuff = NULL;
    char* szData = NULL;
    
    if(iReqBodyLen != 0)
    {
        szData = new char[iReqBodyLen];
        if(szData == NULL)
        {
            goto GOTO_ERR1;
        }
        memset(szData, 0, sizeof(char)*iReqBodyLen);
    }
    szSendBuff = new char[iSendLen];
    if(szSendBuff == NULL)
    {
       goto GOTO_ERR1;
    }
    memset(szSendBuff, 0, sizeof(char)*iSendLen);

    // 组建返回消息头
    BuildHead(&retHead);
    retHead.bFlag = 0x10;
    retHead.bMsgType = head->bMsgType;
    retHead.bLength_h = (iRetBodyLen & 0xff00)>>8;
    retHead.bLength_l = (iRetBodyLen & 0xff);
    retHead.bChecksum = (retHead.bHead1 + retHead.bHead2 + retHead.bVersion + retHead.bFlag + \
		                 retHead.bMsgType + retHead.bLength_h + retHead.bLength_l) & 0xff;
    // 返回消息体
    memset(&retBody, 0, sizeof(CMDRESPONSE));
    // 获取消息体
    lRet = GetMsgBody(net, head, szData, iReqBodyLen, 100);
    if(lRet != 0)
    {
        goto GOTO_ERR1;
    }
    
    cmd = head->bFlag;
    switch (cmd)
    {
        case SYSTEM_PROTOCOL://系统级协议（catalog = 0）
            _DEBUG_("SYSTEM_PROTOCOL");
            lRet = OnMsg_System(head, szData, iReqBodyLen, &retBody);
            break;
        case CONFIG_PROTOCOL://设备配置协议（catalog = 1）
            _DEBUG_("CONFIG_PROTOCOL");
            lRet = OnMsg_Config(head, szData, iReqBodyLen, &retBody);
            break;
        case ENCODER_PROTOCOL://编码参数协议（catalog = 2）
            _DEBUG_("ENCODER_PROTOCOL");
            lRet = OnMsg_Encoder(head, szData, iReqBodyLen, &retBody);
            break;
        case ALARM_PROTOCOL://告警协议（catalog = 3）
            _DEBUG_("ALARM_PROTOCOL");
            lRet = OnMsg_Alarm(head, szData, iReqBodyLen, &retBody);
            break;
        case STREAM_PROTOCOL://媒体流发送控制协议（catalog = 4）
            _DEBUG_("STREAM_PROTOCOL");
            lRet = OnMsg_Stream(head, szData, iReqBodyLen, &retBody);
            break;
        case CAMCONTROL_PROTOCOL://云台控制命令协议
            _DEBUG_("CAMCONTROL_PROTOCOL");
            lRet = OnMsg_CamControl(head, szData, iReqBodyLen, &retBody);
            break;
        default:
            _DEBUG_("unknown command type [0x%x]!", cmd);
            goto GOTO_ERR1;
            break;
    };
    memcpy(szSendBuff, &retHead, iHeadLen);
    memcpy(szSendBuff+iHeadLen, &retBody, iRetBodyLen);
    SendMsg(net, (void *)szSendBuff, iSendLen);
    
GOTO_ERR1:
    PTR_DELETE_A(szData);
    PTR_DELETE_A(szSendBuff);
    return lRet;
}

long CVmux6671Protocol::GetMsgHead( CSyncNetEnd *net, VMUX6KHEAD *head, int timeout)
{
	int iHeadLen = sizeof(VMUX6KHEAD);
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
		_DEBUG_("receive [%d != %d] header error", iRecvLen, iHeadLen);
		return -1;
	}

	if ( CheckHead( head ) < 0 )
	{
		return -1;
	}
    _DEBUG_("receive header ok");
	return 0;
}

long CVmux6671Protocol::GetMsgBody(CSyncNetEnd *net, VMUX6KHEAD *head, char* body, int len, int timeout)
{
    if(body==NULL)
    {
        _DEBUG_("body == NULL");
        return -1;
    }
    int length = (int)(head->bLength_h<<8)+head->bLength_l;
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
long CVmux6671Protocol::SendMsg(CSyncNetEnd *net, void* buf, int len)
{
	//_DEBUG_("send msg to [%s] >>>>>>>>>>>>>>>>>>", net->getRemoteAddr());
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

long CVmux6671Protocol::BuildHead(VMUX6KHEAD *head)
{
	memset(head, 0, sizeof(VMUX6KHEAD));
	head->bHead1 = 0xBF;
	head->bHead2 = 0xEC;
	head->bVersion = 0x01;
	return 0;
}

long CVmux6671Protocol::CheckHead(VMUX6KHEAD *head)
{
    if(head == NULL)
    {
        _DEBUG_("head == NULL, check failed!");
        return -1;
    }
	if (head->bHead1 != 0xBF)
	{
		_DEBUG_( "CheckHead head1 is 0x%x", head->bHead1);
		return -1;
	}

	if (head->bHead2 != 0xEC)
	{
		_DEBUG_( "CheckHead head2 is 0x%x", head->bHead2);
		return -1;
	}

	if ((head->bVersion & 0xf) != 0x1)
	{		
		_DEBUG_( "CheckHead version is 0x%x", head->bVersion);
		return -1;
	}


	if (((head->bHead1 + head->bHead2 + head->bVersion + head->bFlag \
		+ head->bMsgType + head->bLength_h + head->bLength_l) & 0xff) != head->bChecksum)
	{
		_DEBUG_( "CheckHead checksum is 0x%x", head->bChecksum);
		return -1;
	}
	return 0;

}

long CVmux6671Protocol::OnMsg_System( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    REBOOTPARAM *pRebootParam = NULL;
    REBOOTPARAM tmpRebootParam;
    int cmd = head->bMsgType;
    switch(cmd)
    {
        case 0x0f: // 重启
            if(len != sizeof(REBOOTPARAM))
            {
                return -1;
            }
            pRebootParam = (REBOOTPARAM*)body;
            memcpy(&tmpRebootParam, pRebootParam, len);
            tmpRebootParam.iChannel = ntohs(pRebootParam->iChannel);
            tmpRebootParam.iSlot = ntohs(pRebootParam->iSlot);
            ack->iChannel = pRebootParam->iChannel;
            ack->iSlot = pRebootParam->iSlot;
            lRet = OnRebootDevice(&tmpRebootParam);
            break;
        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

long CVmux6671Protocol::OnMsg_Config( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    TIMEPARAM *pTimeParam = NULL;
    TIMEPARAM tmpTimeParam;
    int cmd = head->bMsgType;
    switch(cmd)
    {
        case 0x05: //设置时间
            if(len != sizeof(TIMEPARAM))
            {
                return -1;
            }
            pTimeParam = (TIMEPARAM*)body;
            memcpy(&tmpTimeParam, pTimeParam, len);
            tmpTimeParam.iChannel = ntohs(pTimeParam->iChannel);
            tmpTimeParam.iSlot = ntohs(pTimeParam->iSlot);
            ack->iChannel = pTimeParam->iChannel;
            ack->iSlot = pTimeParam->iSlot;
            lRet = OnSetDevTime(&tmpTimeParam);
            break;
        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

long CVmux6671Protocol::OnMsg_Encoder( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    ENCODERPARAM *pEncParam = NULL;
    ENCODERPARAM tmpEncParam;
    DECODERPARAM *pDecParam = NULL;
    DECODERPARAM tmpDecParam;
    OSDPARAM *pOsdParam = NULL;
    OSDPARAM tmpOsdParam;

    int cmd = head->bMsgType;
    switch(cmd)
    {
        case 0x02: // 获取编码器码流参数
            if(len != sizeof(ENCODERPARAM))
            {
                return -1;
            }
            pEncParam = (ENCODERPARAM*)body;
            memcpy(&tmpEncParam, pEncParam, len);
            tmpEncParam.bChannel = pEncParam->bChannel;
            tmpEncParam.iSlot = ntohs(pEncParam->iSlot);
            ack->iChannel = pEncParam->bChannel;
            ack->iSlot = pEncParam->iSlot;
            lRet = OnGetEncStreamParam(&tmpEncParam);
            break;
        case 0x03: // 设置编码器码流参数
            if(len != sizeof(ENCODERPARAM))
            {
                return -1;
            }
            pEncParam = (ENCODERPARAM*)body;
            memcpy(&tmpEncParam, pEncParam, len);
            tmpEncParam.iSlot = ntohs(pEncParam->iSlot);
            tmpEncParam.uiGOP = ntohl(pEncParam->uiGOP);
            tmpEncParam.iCodeRate = ntohl(pEncParam->iCodeRate);
            tmpEncParam.iFrameRate = ntohl(pEncParam->iFrameRate);
            tmpEncParam.uiResolutionH = ntohs(pEncParam->uiResolutionH);
            tmpEncParam.uiResolutionV = ntohs(pEncParam->uiResolutionV);
            tmpEncParam.uiBasicQP = ntohl(pEncParam->uiBasicQP);
            tmpEncParam.uiMaxQP = ntohl(pEncParam->uiMaxCodeRate);
            tmpEncParam.uiCodeRateType = ntohl(pEncParam->uiCodeRateType);
            tmpEncParam.uiMaxCodeRate = ntohl(pEncParam->uiMaxCodeRate);
            tmpEncParam.uiMinCodeRate = ntohl(pEncParam->uiMinCodeRate);
            tmpEncParam.uiGOP_M = ntohl(pEncParam->uiGOP_M);
            tmpEncParam.iEncodeType = ntohl(pEncParam->iEncodeType);
            ack->iChannel = pEncParam->bChannel;
            ack->iSlot = pEncParam->iSlot;
            lRet = OnSetEncStreamParam(&tmpEncParam);
            break;
        case 0x04: // 开始解码
            if(len != sizeof(DECODERPARAM))
            {
                return -1;
            }
            pDecParam = (DECODERPARAM*)body;
            memcpy(&tmpDecParam, pDecParam, len);
            tmpDecParam.iSlot = ntohs(pDecParam->iSlot);
            //tmpDecParam.uiResolutionH = ntohs(pDecParam->uiResolutionH);
            //tmpDecParam.uiResolutionV = ntohs(pDecParam->uiResolutionV);
            //tmpDecParam.uiPeerIP = ntohl(pDecParam->uiPeerIP);
            //tmpDecParam.uiPeerPort = ntohl(pDecParam->uiPeerPort);

            ack->iChannel = pDecParam->bChannel;
            ack->iSlot = pDecParam->iSlot;
            lRet = OnStartDecoder(&tmpDecParam);
            break;
        case 0x05: // 停止解码
            if(len != sizeof(DECODERPARAM))
            {
                return -1;
            }
            pDecParam = (DECODERPARAM*)body;
            memcpy(&tmpDecParam, pDecParam, len);
            tmpDecParam.iSlot = ntohs(pDecParam->iSlot);
            //tmpDecParam.uiResolutionH = ntohs(pDecParam->uiResolutionH);
            //tmpDecParam.uiResolutionV = ntohs(pDecParam->uiResolutionV);
            //tmpDecParam.uiPeerIP = ntohl(pDecParam->uiPeerIP);
            //tmpDecParam.uiPeerPort = ntohl(pDecParam->uiPeerPort);            
            ack->iChannel = pDecParam->bChannel;
            ack->iSlot = pDecParam->iSlot;
            lRet = OnStopDecoder(&tmpDecParam);
            break;
        case 0x07: //OSD设置
            if(len != sizeof(OSDPARAM))
            {
                return -1;
            }

            pOsdParam = (OSDPARAM*)body;
            memcpy(&tmpOsdParam, pOsdParam, len);
            tmpOsdParam.iSlot = ntohs(pOsdParam->iSlot);
            tmpOsdParam.uiFontColor = ntohl(pOsdParam->uiFontColor);
            tmpOsdParam.uiTimeX = ntohl(pOsdParam->uiTimeX);
            tmpOsdParam.uiTimeY = ntohl(pOsdParam->uiTimeY);
            tmpOsdParam.uiWordX = ntohl(pOsdParam->uiWordX);
            tmpOsdParam.uiWordY = ntohl(pOsdParam->uiWordY);
            ack->iChannel = pOsdParam->bChannel;
            ack->iSlot = pOsdParam->iSlot;
            lRet = OnSetOSD(&tmpOsdParam);
            break;
        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

long CVmux6671Protocol::OnMsg_Alarm( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    int cmd = head->bMsgType;
    switch(cmd)
    {
        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

long CVmux6671Protocol::OnMsg_Stream( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    STREAMCTL *pStreamCtl = NULL;
    STREAMCTL tmpStreamCtl;
    int cmd = head->bMsgType;
    switch(cmd)
    {
        case 0x01: // 启动码流控制
            if(len != sizeof(STREAMCTL))
            {
                return -1;
            }
            pStreamCtl = (STREAMCTL*)body;
            memcpy(&tmpStreamCtl, pStreamCtl, len);
            tmpStreamCtl.iSlot = ntohs(pStreamCtl->iSlot);
            tmpStreamCtl.iHeadFlag = ntohl(pStreamCtl->iHeadFlag);
            tmpStreamCtl.iProtocol = ntohl(pStreamCtl->iProtocol);
            //tmpStreamCtl.iPeerIP = ntohl(pStreamCtl->iPeerIP);
            tmpStreamCtl.iPeerPort = ntohl(pStreamCtl->iPeerPort);
            ack->iChannel = pStreamCtl->bChannel;
            ack->iSlot = pStreamCtl->iSlot;
            lRet = OnStartEncoder(&tmpStreamCtl);
            break;
        case 0x03:// 停止码流控制
            if(len != sizeof(STREAMCTL))
            {
                return -1;
            }
            pStreamCtl = (STREAMCTL*)body;
            memcpy(&tmpStreamCtl, pStreamCtl, len);
            tmpStreamCtl.iSlot = ntohs(pStreamCtl->iSlot);
            tmpStreamCtl.iHeadFlag = ntohl(pStreamCtl->iHeadFlag);
            tmpStreamCtl.iProtocol = ntohl(pStreamCtl->iProtocol);
            //tmpStreamCtl.iPeerIP = ntohl(pStreamCtl->iPeerIP);
            tmpStreamCtl.iPeerPort = ntohl(pStreamCtl->iPeerPort);
            ack->iChannel = pStreamCtl->bChannel;
            ack->iSlot = pStreamCtl->iSlot;
            lRet = OnStopEncoder(&tmpStreamCtl);
            break;

        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

long CVmux6671Protocol::OnMsg_CamControl( VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack)
{
    if(!head || !body || !ack)
    {
        return -1;
    }
    long lRet = -1;
    PTZPARAM *pPtzParam = NULL;
    PTZPARAM tmpPtzParam;
    int cmd = head->bMsgType;
    switch(cmd)
    {
        case 0x01:  // PTZ
            if(len != sizeof(PTZPARAM))
            {
                return -1;
            }
            pPtzParam = (PTZPARAM*)body;
            memcpy(&tmpPtzParam, pPtzParam, len);
            tmpPtzParam.iChannel = ntohs(pPtzParam->iChannel);
            tmpPtzParam.iSlot = ntohs(pPtzParam->iSlot);
            ack->iChannel = pPtzParam->iChannel;
            ack->iSlot = pPtzParam->iSlot;
            lRet = OnPtz(&tmpPtzParam);
            break;        
        default:
            _DEBUG_("Unknown message type[0x%x]!", cmd);
            break;
    }
    ack->iRet = htonl(lRet);
    return lRet;
}

////////////////////////////////////////////////////////////////////
long CVmux6671Protocol::OnRebootDevice(REBOOTPARAM *param)
{
    pthread_t  RebootThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&RebootThread, &attr, CVmux6671Protocol::RebootDevWorker, NULL);
    _DEBUG_("##Vmux667## Reboot Device...");
    return 0;
}

void* CVmux6671Protocol::RebootDevWorker(void * param)
{
    CFacility::SleepMsec(2000);
    system("/sbin/reboot");
    return NULL;
}

long CVmux6671Protocol::OnSetDevTime(TIMEPARAM *param)
{
    char time[80] = {0};
    sprintf(time, "%d-%d-%d %d:%d:%d", param->bYear+2000, param->bMonth, param->bDay, \
                                       param->bHour, param->bMinute, param->bSecond);
    _DEBUG_("##Vmux667## 1Set System Time: %s", time);
    return m_pChnManager->m_pDecSet->SetSysTime((char*)time, (void*)m_pChnManager->m_pDecSet);
}

long CVmux6671Protocol::OnGetEncStreamParam(ENCODERPARAM *param)
{
    long lRet = 0;
    int iChnno = param->bChannel;
    int iStreamno = param->bStreamNo;
    if(iStreamno>2)
    {
        iStreamno=2;
    }
    // 转换成内部通道，0-1通道主码流，1-1通道子码流，一次类推
    int iEncChnno = (iChnno-1)*2 + (iStreamno-1);
    // 编码器信息
    EncoderParam sEncParam;
    memset(&sEncParam, 0, sizeof(EncoderParam));
    //PTZ信息
    PtzSetting sPtzSet;
    memset(&sPtzSet, 0, sizeof(PtzSetting));
    // 获取原有信息
    lRet = m_pChnManager->m_pEncSet->GetEncVideo(&sEncParam, &sPtzSet, iEncChnno);
    if(lRet != 0)
    {
        _DEBUG_("Get Encoder[%d] Param failed!", iEncChnno);
        return lRet;
    }
    // 
    param->iEncodeType = htonl(5);// H264
    param->iFrameRate = htonl(sEncParam.iFPS);
    param->iCodeRate = htonl(1000*(sEncParam.iBitRate));
    param->iEncodePolicy = 1;// 码率优先
    param->iEncodeWay = 1;
    param->bStreamFlag = 1; // 码流开关，固定打开
    param->bReserve = 0; // 保留
    param->uiGOP = htonl(sEncParam.iGop);
    param->uiGOP_M = htonl(0);// GOP组合方式
    param->uiBasicQP = htonl(sEncParam.iMinQP); // 基础QP
    param->uiMaxQP = htonl(sEncParam.iMaxQP);
    param->uiCodeRateType = htonl(sEncParam.iCbr);
    param->uiMaxCodeRate = htonl((sEncParam.iBitRate+2000)*1000);
    param->uiMinCodeRate = htonl((sEncParam.iBitRate-500)*1000);
    //0: QCIF, 1: CIF,  2:D1, 3:720P, 4:1080P  default: 2 
    switch(sEncParam.iResolution)
    {
        case 0: // QCIF
            param->uiResolutionH = 176;
            param->uiResolutionV = 144;
            break;
        case 1: // CIF
            param->uiResolutionH = 352;
            param->uiResolutionV = 288;
            break;
        case 2: // D1
            param->uiResolutionH = 704;
            param->uiResolutionV = 576;
            break;
        case 3: // 720P
            param->uiResolutionH = 1280;
            param->uiResolutionV = 720;
            break;
        case 4: // 1080P
            param->uiResolutionH = 1920;
            param->uiResolutionV = 1080;
            break;
        default:
            param->uiResolutionH = 704;
            param->uiResolutionV = 576;
            break;
    };
    param->uiResolutionH = htons(param->uiResolutionH);
    param->uiResolutionV = htons(param->uiResolutionV);
    _DEBUG_("##Vmux667## Get Video Param");
    return lRet;
}

long CVmux6671Protocol::OnSetEncStreamParam(ENCODERPARAM *param)
{
    long lRet = 0;
    int iChnno = param->bChannel;
    int iStreamno = param->bStreamNo;
    if(iStreamno>2)
    {
        iStreamno=2;
    }
    // 转换成内部通道，0-1通道主码流，1-1通道子码流，一次类推
    int iEncChnno = (iChnno-1)*2 + (iStreamno-1);
    // 编码器信息
    EncoderParam sEncParam;
    memset(&sEncParam, 0, sizeof(EncoderParam));
    //PTZ信息
    PtzSetting sPtzSet;
    memset(&sPtzSet, 0, sizeof(PtzSetting));
    // 获取原有信息
    lRet = m_pChnManager->m_pEncSet->GetEncVideo(&sEncParam, &sPtzSet, iEncChnno);
    if(lRet != 0)
    {
        _DEBUG_("Get Encoder[%d] Stream Param failed!", iEncChnno);
        return lRet;
    }
    sEncParam.iBitRate = param->iFrameRate/1000;
    sEncParam.iFPS = param->iFrameRate;
    sEncParam.iCbr = param->uiCodeRateType;
    sEncParam.iGop = param->uiGOP;
    sEncParam.iMinQP = param->uiBasicQP;
    sEncParam.iMaxQP = param->uiMaxQP;
    //0: QCIF, 1: CIF,  2:D1, 3:720P, 4:1080P  default: 2 
    switch(param->uiResolutionV)
    {
        case 144: // QCIF
            sEncParam.iResolution = 0;
            break;
        case 288: // CIF
            sEncParam.iResolution = 1;
            break;
        case 576: // D1
            sEncParam.iResolution = 2;
            break;
        case 720: // 720P
            sEncParam.iResolution = 3;
            break;
        case 1080: // 1080P
            sEncParam.iResolution = 4;
            break;
        default:
            sEncParam.iResolution = 2;
            break;
    };
    _DEBUG_("##Vmux667## Set Video Param");
    lRet = m_pChnManager->m_pEncSet->SetEncVideo(&sEncParam, &sPtzSet, iEncChnno, true);
    return lRet;
}

long CVmux6671Protocol::OnStartDecoder(DECODERPARAM *param)
{
    _DEBUG_("##Vmux667## Start Decoder");
    return 0;
}

long CVmux6671Protocol::OnStopDecoder(DECODERPARAM *param)
{
    _DEBUG_("##Vmux667## Stop Decoder");
    return 0;
}

long CVmux6671Protocol::OnSetOSD(OSDPARAM *param)
{
    long lRet = 0;
    int iChnno = param->bChannel;
    int iStreamno = param->bStreamNo;
    if(iStreamno>2)
    {
        iStreamno=2;
    }
    // 转换成内部通道，0-1通道主码流，1-1通道子码流，一次类推
    int iEncChnno = (iChnno-1)*2 + (iStreamno-1);

    // 获取原有OSD信息
    EncOsdSettings sOsdSetting;
    memset(&sOsdSetting, 0, sizeof(EncOsdSettings));
    lRet = m_pChnManager->m_pEncSet->GetEncDisplay(&sOsdSetting, iEncChnno);
    //0: date 1: time  2: date and time 3: content 4: name
    for(int i=0; i<MAXOSD_COUNT; i++)
    {
        if(sOsdSetting.osd[i].iType == 2) // 日期时间
        {
            sOsdSetting.osd[i].iEnable = param->bShowTime;
            sOsdSetting.osd[i].iX = param->uiTimeX;
            sOsdSetting.osd[i].iY = param->uiTimeY;
        }
        else if(sOsdSetting.osd[i].iType == 4)// 通道名称
        {
            sOsdSetting.osd[i].iEnable = param->bShowWord;
            sOsdSetting.osd[i].iX = param->uiWordX;
            sOsdSetting.osd[i].iY = param->uiWordY;
            sprintf(sOsdSetting.osd[i].szOsd, param->szWord);
        }
    }
    _DEBUG_("##Vmux667## Set OSD");
    lRet = m_pChnManager->m_pEncSet->SetEncDisplay(&sOsdSetting, iEncChnno);
    return lRet;
}

long CVmux6671Protocol::OnStartEncoder(STREAMCTL *param)
{
    long lRet = 0;
    int iChnno = param->bChannel;
    int iStreamno = param->bStreamNo;
    if(iStreamno>2)
    {
        iStreamno=2;
    }
    // 转换成内部通道，0-1通道主码流，1-1通道子码流，一次类推
    int iEncChnno = (iChnno-1)*2 + (iStreamno-1);

    EncNetSettings sEncNetSet;
    memset(&sEncNetSet, 0, sizeof(EncNetSettings));
    // 获取现有数据
    /*lRet = m_pChnManager->m_pEncSet->GetEncTransfer(&sEncNetSet, iEncChnno);
    if(lRet != 0)
    {
        _DEBUG_("Get Encoder[%d] Transfer Info failed!", iEncChnno);
        return lRet;
    }*/
    sEncNetSet.iChannelNo = iEncChnno;
    sEncNetSet.net[0].iEnable = 1;
    sEncNetSet.net[0].iIndex = 0;
    //iNetType -- 0: udp 1: tcp 2: rtsp 3: RTP
    if(param->iProtocol == 1)//tcp
    {
        sEncNetSet.net[0].iNetType = 1; //tcp
    }
    else // UDP / multicast
    {
        sEncNetSet.net[0].iNetType = 0;
    }
    struct in_addr in;
    in.s_addr = param->iPeerIP;
    sprintf(sEncNetSet.net[0].szPlayAddress, "%s", inet_ntoa(in));
    sEncNetSet.net[0].iPlayPort = param->iPeerPort;
    //iMux - 0: es 1: ps 2: ts 
    if(param->iHeadFlag == 1)// 标准流
    {
        sEncNetSet.net[0].iNetType = 1;
    }
    else if(param->iHeadFlag == 2)// 裸码流
    {
        sEncNetSet.net[0].iNetType = 0;
    }
    else
    {
        sEncNetSet.net[0].iMux = 0;
    }
    _DEBUG_("##Vmux667## Start Encoder Video Transfer, [%s] %s:%d", (param->iProtocol==1)?"tcp":"udp", \ 
                    sEncNetSet.net[0].szPlayAddress, sEncNetSet.net[0].iPlayPort);
    lRet = m_pChnManager->m_pEncSet->SetEncTransfer(&sEncNetSet, iEncChnno, true);
    return lRet;
}

long CVmux6671Protocol::OnStopEncoder(STREAMCTL *param)
{
    long lRet = 0;
    int iChnno = param->bChannel;
    int iStreamno = param->bStreamNo;
    if(iStreamno>2)
    {
        iStreamno=2;
    }
    // 转换成内部通道，0-1通道主码流，1-1通道子码流，一次类推
    int iEncChnno = (iChnno-1)*2 + (iStreamno-1);

    EncNetSettings sEncNetSet;
    memset(&sEncNetSet, 0, sizeof(EncNetSettings));
    // 获取现有数据
    lRet = m_pChnManager->m_pEncSet->GetEncTransfer(&sEncNetSet, iEncChnno);
    if(lRet != 0)
    {
        _DEBUG_("Get Encoder[%d] Transfer Info failed!", iEncChnno);
        return lRet;
    }
    //
    // 检索需要停止的传输配置数据，并停止
    //
    // 转换成内部数据
    EncNetParam sEncNetParam;
    memset(&sEncNetParam, 0, sizeof(EncNetParam));
    //iNetType -- 0: udp 1: tcp 2: rtsp 3: RTP
    if(param->iProtocol == 1)//tcp
    {
        sEncNetParam.iNetType = 1; //tcp
    }
    else // UDP / multicast
    {
        sEncNetParam.iNetType = 0;
    }
    struct in_addr in;
    in.s_addr = param->iPeerIP;
    sprintf(sEncNetParam.szPlayAddress, "%s", inet_ntoa(in));
    sEncNetParam.iPlayPort = param->iPeerPort;
    //iMux - 0: es 1: ps 2: ts 
    if(param->iHeadFlag == 1)// 标准流
    {
        sEncNetParam.iNetType = 1;
    }
    else if(param->iHeadFlag == 2)// 裸码流
    {
        sEncNetParam.iNetType = 0;
    }
    else
    {
        sEncNetParam.iMux = 0;
    }
    
    for(int i=0; i<MAXNET_COUNT; i++)
    {
        if(
            sEncNetSet.net[i].iNetType==sEncNetParam.iNetType &&
            sEncNetSet.net[i].iPlayPort==sEncNetParam.iPlayPort &&
            sEncNetSet.net[i].iMux==sEncNetParam.iMux &&
            strcmp(sEncNetSet.net[i].szPlayAddress, sEncNetParam.szPlayAddress)==0
            )
        {
            sEncNetSet.net[i].iEnable = 0;
            sEncNetSet.net[i].iNetType = 0;
            memset(sEncNetSet.net[i].szPlayAddress, 0, sizeof(sEncNetSet.net[i].szPlayAddress));
            sEncNetSet.net[i].iPlayPort = 0;
            sEncNetSet.net[i].iMux = 0;
            break;
        }
    }
    _DEBUG_("##Vmux667## Stop Encoder Video Transfer, [%s] %s:%d", (param->iProtocol==1)?"tcp":"udp", \ 
                    sEncNetParam.szPlayAddress, sEncNetParam.iPlayPort);
    lRet = m_pChnManager->m_pEncSet->SetEncTransfer(&sEncNetSet, iEncChnno, true);
    return lRet;
}

long CVmux6671Protocol::OnPtz(PTZPARAM *param)
{
    long lRet = 0;

    _DEBUG_("##Vmux667## PTZ Controlling");
    return lRet;
}






