#ifndef AT200PROTOCOL_H_
#define AT200PROTOCOL_H_
#include "Protocol.h"
#include "netsync.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"

#pragma pack(1)


class CAT200Protocol : public CProtocol
{

public: 
	CAT200Protocol( int nPort, CChannelManager *pChnManager );
	virtual ~CAT200Protocol();
    int StopEncoder(int chnno);
private:
	CSyncNetServer *m_pServer;

	static void THWorker(CSyncServerNetEnd *net, void *p);
	
	int CheckSwitch(char *cmd);
	void ProcessBuffer(char *buf, int &iSize);
	int CheckCtrl(char *cmd);

	int m_iExitFlag;
	
	pthread_mutex_t m_lock;
	

	int m_iCmdIndex;
	char m_szCmd[1024];


};


#endif







