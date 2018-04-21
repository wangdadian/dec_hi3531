#ifndef FORWARDPORT_H_20150107
#define FORWARDPORT_H_20150107
#include "Protocol.h"
#include "netsync.h"
#include "NetUdp.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"
#include "IniFile.h"

#include <string.h>

class CForwardport : public CProtocol
{
public: 
    CForwardport( int nPort, CChannelManager *pChnManager );
    CForwardport(ForwardportSetting* pFp, CChannelManager *pChnManager );
    virtual ~CForwardport();

private:
    CSyncNetServer *m_pServer; //TCP
    CNetUdp* m_pNetUdp; // UDP
    int m_iNetType; // Õ¯¬Á¿‡–Õ£¨0-UDP£¨1-TCP
    ForwardportSetting m_sFp;
    static void THWorker(CSyncServerNetEnd *net, void *p);
    static void* THWorkerUdp(void* param);
    static void* THWorkerChkCfg(void* param);
    long Init();
    long UnInit();
    void ProcessBuffer(char *buf, int &iSize);


    int m_iExitFlag;
    
    pthread_mutex_t m_lock;    
    pthread_t m_thWorkerUdp;
    pthread_t m_thWorkerChkCfg;
};


#endif


