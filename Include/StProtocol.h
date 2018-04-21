#ifndef ST_PROTOCOL_H_2015
#define ST_PROTOCOL_H_2015

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


class CStProtocol : public CProtocol
{

public: 
	CStProtocol( int nPort, CChannelManager *pChnManager );
	virtual ~CStProtocol();
    int StopEncoder(int chnno);
private:
    long InitForEnc();
    long UninitForEnc();
    static void THWorker(CSyncServerNetEnd *net, void *p);
    long ProcessMsg(CSyncNetEnd *net, struStHead *head);
    long GetMsgHead( CSyncNetEnd *net, struStHead *head, int timeout=100);
    long GetMsgBody(CSyncNetEnd *net, struStHead *head, char* body, int len, int timeout=100);
    long SendMsg(CSyncNetEnd *net, void* buf, int len);
    long BuildHead(struStHead *head);
    long CheckHead(struStHead *head);
    bool check_ip(char* ip);
	int  changedata(int cmd);
	short changedatas(short cmd);
private:
    // 和控制器通讯处理接口
    long OnReqVcLoginDec(CSyncNetEnd *net, struStHead *head);
    long OnReqVcLogoutDec(CSyncNetEnd *net, struStHead *head);
    long OnReqVcSetDecProfile(CSyncNetEnd *net, struStHead *head);
    long OnReqVcGetDecProfile(CSyncNetEnd *net, struStHead *head);
    long OnReqVcGetDecState(CSyncNetEnd *net, struStHead *head);
    long OnReqVcSetUnicastEnc(CSyncNetEnd *net, struStHead *head);
    long OnReqVcGetUnicastEnc(CSyncNetEnd *net, struStHead *head);
    long OnReqVcLockDecVideo(CSyncNetEnd *net, struStHead *head);
    long OnReqVcUnlockDecVideo(CSyncNetEnd *net, struStHead *head);
    long OnReqVcSetMulticastGroup(CSyncNetEnd *net, struStHead *head);
    long OnReqVcSetRelay(CSyncNetEnd *net, struStHead *head);
    //
    long OnSetUnicastEnc(ReqVcSetUnicastEnc* p);
    long OnSetMulticast(ReqVcSetMulticastGroup* p);
    long OnSetRelay(ReqVcSetRelay* p);
private:
    // 和编码器通讯处理接口
    static void* THKeepaliveWorker(void *param);
    long StartDecoderUdp(int iChnno, char* szIp, int iPort);
    long StartDecoderTcp(int iChnno, char* szIp, int iPort);
    long StopDecoder(int iChnno);
    long OnDecLoginEnc(char* szIp, int iPort);
    long OnDecLogoutEnc();
    long OnDecConnectEnc();
    long OnDecDisconnectEnc();
    long SendMsgToEnc(void* buf, int len);
    long GetMsgFromEnc(void* buf, int len);
private:
    int m_iExitFlag;
    int m_iExitFlagForEnc;
	pthread_mutex_t m_lock;
    pthread_t m_thKeepaliveWorker;
	CSyncNetServer *m_pServer;
    CClientSyncNetEnd* m_pClient;
    char m_szEncIp[64];
    unsigned char m_byteDecState;//解码器状态 0x00未连接编码器 0x01接收单播流 0x02接收组播流
    ReqVcSetUnicastEnc m_sUnicastEnc;//当前连接的单播编码器
    ReqVcSetMulticastGroup m_sMulticastGroup;//当前解码器的加入的组播地址信息
    ReqVcSetRelay m_sRelay;//当前解码器转发地址信息
    int m_iLastConnecedEncTime;//连接编码器的时间
    bool m_bInitedForEnc;
    char m_szVcIp[64];
    int m_iChnno; // 关联的解码通道
};


#endif //#ifndef ST_PROTOCOL_H_2015


