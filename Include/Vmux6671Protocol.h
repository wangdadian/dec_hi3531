#ifndef __VMUX_6671_PROTOCOL_H_2015_5
#define __VMUX_6671_PROTOCOL_H_2015_5

#include "Protocol.h"
#include "netsync.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"
#include "Vmux6kProtocolDefine.h"

class CVmux6671Protocol : public CProtocol
{

public: 
    CVmux6671Protocol(int nPort, CChannelManager *pChnManager);
    virtual ~CVmux6671Protocol();
    int StopEncoder(int chnno);
    
private:
    static void THWorker(CSyncServerNetEnd *net, void *p);
    long ProcessMsg(CSyncNetEnd *net, VMUX6KHEAD *head);
    long GetMsgHead( CSyncNetEnd *net, VMUX6KHEAD *head, int timeout=100);
    long GetMsgBody(CSyncNetEnd *net, VMUX6KHEAD *head, char* body, int len, int timeout=100);
    long SendMsg(CSyncNetEnd *net, void* buf, int len);
    long BuildHead(VMUX6KHEAD *head);
    long CheckHead(VMUX6KHEAD *head);
private:
	long OnMsg_System(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
    long OnMsg_Config(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
    long OnMsg_Encoder(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
    long OnMsg_Alarm(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
    long OnMsg_Stream(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
    long OnMsg_CamControl(VMUX6KHEAD *head, void* body, int len, CMDRESPONSE* ack);
private:
    long OnRebootDevice(REBOOTPARAM *param);
    static void* RebootDevWorker(void * param);
    long OnSetDevTime(TIMEPARAM *param);
    long OnGetEncStreamParam(ENCODERPARAM *param);
    long OnSetEncStreamParam(ENCODERPARAM *param);
    long OnStartDecoder(DECODERPARAM *param);
    long OnStopDecoder(DECODERPARAM *param);
    long OnSetOSD(OSDPARAM *param);
    long OnStartEncoder(STREAMCTL *param);
    long OnStopEncoder(STREAMCTL *param);
    long OnPtz(PTZPARAM *param);
private:
    int m_iExitFlag;
    pthread_mutex_t m_lock;
    CSyncNetServer *m_pServer;
};



#endif //__VMUX_6671_PROTOCOL_H_2015_5




