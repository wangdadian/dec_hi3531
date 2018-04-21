#ifndef DEV_UPDATE_PROTOCOL_H_
#define DEV_UPDATE_PROTOCOL_H_

#include "Protocol.h"
#include "netsync.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "PublicDefine.h"
#include "ChannelManager.h"
#include "UpProtocolDefine.h"
#include  <arpa/inet.h>
#include  <net/if.h>
#include  <net/if_arp.h>


class CUpProtocol : public CProtocol
{
public:
    CUpProtocol( int nPort, CChannelManager *pChnManager );
	virtual ~CUpProtocol();


private:
    static void THServiceWorker(CSyncServerNetEnd *net, void *p);
    static void* THDiscoveryWorker(void* param);
    
private:
    bool CheckHead(struUpHead *head);
    long BuildHead(struUpHead *head);    
    long SendMsg(CSyncNetEnd *net, void* buf, int len);
    long GetMsgHead( CSyncNetEnd *net, struUpHead *head, int timeout=100);
    long GetMsgBody(CSyncNetEnd *net, struUpHead *head, char* body, int len, int timeout=100);
    long ProcessMsg(CSyncNetEnd *net, struUpHead *head);
    long ProcessCmd(CNetUdp *net, struUpHead *head);
    
    long OnCmdReqDetectDev(CNetUdp *net, struUpHead *head);
    long OnReqUpgradeDev(CSyncNetEnd *net, struUpHead *head);
    long OnReqRebootDev(CSyncNetEnd *net, struUpHead *head);
    long OnReqSettime(CSyncNetEnd *net, struUpHead *head);
    
private:
    int m_iExitFlag;
	pthread_mutex_t m_lock;
    pthread_t m_thDiscoveryWorker;
    CSyncNetServer *m_pServer;
    CNetUdp* m_pNetUdp;

    CSyncServerNetEnd *m_net; // 连接到此设备的net
};

#endif //DEV_UPDATE_PROTOCOL_H_



