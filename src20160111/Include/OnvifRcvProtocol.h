#ifndef ONVIF_RCV_PROTOCOL_H__
#define ONVIF_RCV_PROTOCOL_H__

#include "Protocol.h"
#include "netsync.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "PublicDefine.h"
#include "ChannelManager.h"
#include "StProtocolDefine.h"
#include  <arpa/inet.h>
#include  <net/if.h>
#include  <net/if_arp.h>


class COnvifRcvProtocol : public CProtocol
{

public: 
	COnvifRcvProtocol( int nPort, CChannelManager *pChnManager );
	virtual ~COnvifRcvProtocol();
private:
    static void *discoveryThWorker(void *param);   
    static void *receiverThWorker(void *param);
    int create_server_socket_udp();
    int bind_server_udp1(int server_s, short port);
    static int SetMediaUri(char* szUri, int chnno, void* param);
    static int SetNetIp(char* ip, char* netmask, void* param);
    static int SetNetGW(void* gw, void* param);
    static int GetNetGW(void* gw, void* param);
    static int GetHostname(void* name, void* param);
    static int SetHostname(void* name, void* param);
    static int GetSystime(void* time, void* param);
    static int SetSystime(void* time, void* param);
    static int GetNtp(void* ntp, void* param);
    static int SetNtp(void* ntp, void* param);

    static int GetBuildtime(void* time, void* param);
    static int GetModel(void* model, void* param);
    static int GetSerial(void* serial, void* param);
private:
    int m_iExitFlag;
	pthread_mutex_t m_lock;
    pthread_t m_thdiscoveryWorker;
    pthread_t m_threceiverWorker;
    struct soap *m_pSoap_discovery;
    struct soap *m_pSoap_Receiver;
    int m_iChnno; // 当前关联的解码通道
};


#endif //#ifndef ONVIF_RCV_PROTOCOL_H__






