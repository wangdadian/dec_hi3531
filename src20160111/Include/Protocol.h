

#ifndef PROTOCOL_H_
#define PROTOCOL_H_
#include "ChannelManager.h"
#include  <arpa/inet.h>
#include  <net/if.h>
#include  <net/if_arp.h>

class CProtocol
{

public:
	CProtocol( int nPort, CChannelManager *pChnManager  );
	~CProtocol();

	virtual int OnAlarm( char *szAlarmInId, int nAlarminId, int nStatus ); 
    // chnno:关联的解码器通道
    virtual int StopEncoder(int chnno);
    void get_ip(char *ip);
    void NTOHS( unsigned short &ut );
	void HTONS( unsigned short &ut );	
	void NTOHL( unsigned int &lt );
	void HTONL( unsigned int &lt );
protected:
	CChannelManager *m_pChnManager;
//private:
	int m_nPort;
};

#endif


