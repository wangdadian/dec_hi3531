// ClientPoint.h: interface for the CClientPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENTPOINT_H__DB533832_4A86_45BA_A4AB_166BC19CECCB__INCLUDED_)
#define AFX_CLIENTPOINT_H__DB533832_4A86_45BA_A4AB_166BC19CECCB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#include <winsock2.h>
#else 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "winsockdef.h"
#endif 

class CClientPoint  
{
public:
	void SetAutoReconnect( int nReconnect /* 0: close 1: open*/ );
	CClientPoint( );
	virtual ~CClientPoint();

	int CloseClient();
	int ReadSelect(int tm);
	int WriteSelect(int tm);
	
	int RecvBySize( void *buffer, int iSize );
	int RecvBySizeT( void *buffer, int iSize, int timeout );
	
	int SendBySize( void *buffer, int iSize );
	int SendBySizeT( void *buffer, int iSize, int timeout);

	int ConnectToController( char *sIP, int iPort );
	char *GetAddr();
	unsigned int GetPort();
	char *GetLocalAddr();
	unsigned int GetLocalPort();
private:
	char m_szTmp[256];
	unsigned int m_iServerPort;
	char m_sServerIP[MAX_PATH];
	char m_sLocalIP[MAX_PATH];
	unsigned int m_iLocalPort;
	SOCKET fd;
	fd_set rfds, wfds;
	int m_nAutoReconnect;
#ifdef _WIN32
	static DWORD WINAPI ConnectionCheckProc( void *param );
#else
	static HANDLE ConnectionCheckProc( void *param );
#endif
	void ConnectionCheckWorker();
	int Connect();
	int m_iExitFlag;
	int m_iCloseFlag;
#ifdef _WIN32
	HANDLE m_hThreadId;
#else
	pthread_t m_hThreadId;
#endif
	CRITICAL_SECTION m_lock;
};

#endif // !defined(AFX_CLIENTPOINT_H__DB533832_4A86_45BA_A4AB_166BC19CECCB__INCLUDED_)
