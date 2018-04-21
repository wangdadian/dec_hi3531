// ClientPoint.cpp: implementation of the CClientPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CommonDef.h"
#include "ClientPoint.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CClientPoint::CClientPoint(  )
{
	fd = INVALID_SOCKET;
#ifdef _WIN32
	WSADATA ws; 
	WSAStartup(0x0202,&ws); 
#endif 

	m_sServerIP[0] = '\0';
	m_iServerPort = 0;
	m_sLocalIP[0] = '\0';
	m_iLocalPort = 0;
	m_nAutoReconnect = 0;
	m_iExitFlag = 0;
	m_iCloseFlag = 0;
	m_hThreadId = 0;
	InitializeCriticalSection(&m_lock);
}

CClientPoint::~CClientPoint()
{
	m_iExitFlag = 1;
	m_iCloseFlag = 1;
#ifdef _WIN32
	if ( m_hThreadId != NULL )
	{
		WaitForSingleObject(m_hThreadId, 10000 );
	}
	WSACleanup();
#else

	if ( m_hThreadId != 0 )
	{
		pthread_join( m_hThreadId, NULL );
	}
	
#endif 
	CloseClient();
	DeleteCriticalSection(&m_lock);
}


int CClientPoint::CloseClient()
{
	m_iCloseFlag = 1;
	if ( fd != INVALID_SOCKET )
	{
		closesocket( fd );
		return 1;
	}
	else return -1;
	
}

int CClientPoint::Connect()
{
	fd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( fd == INVALID_SOCKET )
	{

#ifdef _WIN32
		int iError = WSAGetLastError();
		OutputDebugStr( "cannot create socket: ");

		switch( iError )
		{
			case WSANOTINITIALISED:
				OutputDebugStr( "A successful WSAStartup call must occur before using this function.\n" ); 
				break;
			case WSAENETDOWN:
				OutputDebugStr( "The network subsystem or the associated service provider has failed. \n" );
				break;
			case WSAEAFNOSUPPORT:
				OutputDebugStr( " The specified address family is not supported. \n" );
				break;
			case WSAEINPROGRESS:
				OutputDebugStr("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
				break;
			case WSAEMFILE:
				OutputDebugStr( "No more socket descriptors are available. \n" );
				break;
			case WSAENOBUFS:
				OutputDebugStr( "No buffer space is available. The socket cannot be created. \n" );
				break;
			case WSAEPROTONOSUPPORT:
				OutputDebugStr("The specified protocol is not supported. \n" );
			case WSAEPROTOTYPE:
				OutputDebugStr( "The specified protocol is the wrong type for this socket. \n" );
			case WSAESOCKTNOSUPPORT:
				OutputDebugStr( "The specified socket type is not supported in this address family. \n" );
		}
#else
		perror( "create socket");
#endif
		return SOCKET_ERROR;
	}

	struct hostent *h;
	
	h = gethostbyname( m_sServerIP );
	if ( h == NULL )
	{
		char szTemp[256] = {0};
		sprintf( szTemp, "hostname %s error!", m_sServerIP );
		OutputDebugStr( szTemp );

#ifdef _WIN32
		int iError = WSAGetLastError();
		switch( iError )
		{
		case WSANOTINITIALISED:
			OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n" );
			break;
		case WSAENETDOWN:
			OutputDebugStr( " The network subsystem has failed. \n" );
			break;
		case WSAHOST_NOT_FOUND:
			OutputDebugStr( "Authoritative answer host not found. \n" );
			break;
		case WSATRY_AGAIN:
			OutputDebugStr( "Nonauthoritative host not found, or server failure. \n" );
			break;
		case WSANO_RECOVERY:
			OutputDebugStr( "A nonrecoverable error occurred. \n" );
			break;		
		case WSANO_DATA:
			OutputDebugStr( "Valid name, no data record of requested type. \n" );
			break;
		case WSAEINPROGRESS:
			OutputDebugStr( "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
			break;
		case WSAEFAULT:
			OutputDebugStr( "The name parameter is not a valid part of the user address space. \n" );
			break;
		case WSAEINTR:
			OutputDebugStr( "A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall. \n" );
			break;
		}
#else
		perror("get host by name error: ");
#endif
		CloseClient();
		return SOCKET_ERROR;
	}

	struct sockaddr_in server_addr, local_addr;
	server_addr.sin_family = h->h_addrtype;
	server_addr.sin_port = htons( m_iServerPort );
	memcpy((char *) &server_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length );

	local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(0);
  
    if ( bind( fd, ( struct sockaddr * ) &local_addr, sizeof( local_addr ) ) == SOCKET_ERROR )
	 {
		 OutputDebugStr( "cannot bind socket: " );
#ifdef _WIN32
		 int iError = WSAGetLastError();

		 switch( iError )
		 {
			 case WSANOTINITIALISED:
				 OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n" );
				 break;
			 case WSAENETDOWN:
				 OutputDebugStr( "The network subsystem has failed. \n" );
				 break;
			 case WSAEACCES:
				 break;
			 case WSAEADDRINUSE:
				 OutputDebugStr( "A process on the computer is already bound to the same fully-qualified address and the socket has not been marked to allow address reuse with SO_REUSEADDR. For example, the IP address and port are bound in the af_inet case). (See the SO_REUSEADDR socket option under setsockopt.) \n" ); 
				 break;
			 case WSAEADDRNOTAVAIL:
				 OutputDebugStr( "The specified address is not a valid address for this computer. \n" );
				 break;
			 case WSAEFAULT:
				 OutputDebugStr( "The name or namelen parameter is not a valid part of the user address space, the namelen parameter is too small, the name parameter contains an incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor s. \n" );
				 break;
			 case WSAEINPROGRESS:
				 OutputDebugStr( "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
				 break;
			 case WSAEINVAL:
				 OutputDebugStr( "The socket is already bound to an address. \n" );
				 break;
			 case WSAENOBUFS:
				 OutputDebugStr("Not enough buffers available, too many connections. \n" );
				 break;
			 case WSAENOTSOCK:
				 OutputDebugStr( "The descriptor is not a socket. \n" );
		 }
#else
		 perror( "bind error");
#endif
	//	CloseClient();
	//	return SOCKET_ERROR;
 	 }


#ifdef WIN32
	ULONG nonblock = 1; 

	if(SOCKET_ERROR == ioctlsocket(fd,   FIONBIO,   (unsigned   long*)&nonblock) ) 
	{
		printf( "ioctl socket error.");
		CloseClient();
		return SOCKET_ERROR;
	}
#else 
	

#endif

	if ( connect(fd, ( struct sockaddr * )&server_addr, sizeof( server_addr ) ) == SOCKET_ERROR  )
	{
		OutputDebugStr( "connect error: ");
		
#ifdef _WIN32
		int iError = WSAGetLastError();
		switch( iError )
		{
		case WSANOTINITIALISED:
			OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n" );
			break;
		case WSAENETDOWN:
			OutputDebugStr( "The network subsystem has failed. \n" );
			break;
		case WSAEADDRINUSE:
			OutputDebugStr("The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs when executing bind, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function. \n" );
			break;
			
		case WSAEINTR:
			OutputDebugStr( "The blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall. \n" );
			break;

		case WSAEINPROGRESS:
			OutputDebugStr("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
			break;

		case WSAEALREADY:
			OutputDebugStr( "A nonblocking connect call is in progress on the specified socket. Note In order to preserve backward compatibility, this error is reported as WSAEINVAL to Windows Sockets 1.1 applications that link to either Winsock.dll or Wsock32.dll. \n" );
			break;

		case WSAEADDRNOTAVAIL:
			OutputDebugStr( "The remote address is not a valid address (such as ADDR_ANY). \n" );
			break;
		case WSAEAFNOSUPPORT:
			OutputDebugStr( "Addresses in the specified family cannot be used with this socket. \n" );
			break;
		case WSAECONNREFUSED:
			OutputDebugStr( "The attempt to connect was forcefully rejected. \n" );
			break;
		case WSAEFAULT:
			OutputDebugStr("The name or the namelen parameter is not a valid part of the user address space, the namelen parameter is too small, or the name parameter contains incorrect address format for the associated address family. \n" );
			break;
		case WSAEINVAL:
			OutputDebugStr("The parameter s is a listening socket. \n" );
		case WSAEISCONN:
			OutputDebugStr( "The socket is already connected (connection-oriented sockets only). \n" );
			break;
		case WSAENETUNREACH:
			OutputDebugStr( "The network cannot be reached from this host at this time. \n" );
			break;
		case WSAENOBUFS:
			OutputDebugStr( "No buffer space is available. The socket cannot be connected. \n" );
			break;
		case WSAENOTSOCK:
			OutputDebugStr( "The descriptor is not a socket. \n " );
			break;
		case WSAETIMEDOUT:
			OutputDebugStr( "Attempt to connect timed out without establishing a connection. \n" );
			break;
		case WSAEWOULDBLOCK:
			OutputDebugStr( "The socket is marked as nonblocking and the connection cannot be completed immediately. \n " );
			break;
		case WSAEACCES:
			OutputDebugStr( "Attempt to connect datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled. \n" );
			break;
		}
#else
		perror( "connect error");
#endif


	//	CloseClient();
		//WSACleanup(); 
	//	return SOCKET_ERROR;
	}

#ifdef WIN32

	struct timeval timeout ;
	fd_set r;
	
	
	FD_ZERO(&r);
	FD_SET( fd, &r);
	timeout.tv_sec = 5; //Á¬½Ó³¬Ê±
	timeout.tv_usec =0;
	int ret = select(0, 0, &r, 0, &timeout);
	
	if ( ret <= 0 )
	{
		printf("select error\n");
		CloseClient();
		//WSACleanup(); 
		return SOCKET_ERROR;
	}
#endif 
	
#ifdef MACOS
	unsigned long ulRcvBuf = 100 * 1024 ;
#else 
	unsigned long ulRcvBuf = 2 * 1024 * 1024;
#endif



	if ( setsockopt( fd, SOL_SOCKET, SO_RCVBUF, (const char*)&ulRcvBuf, sizeof( ulRcvBuf) ) == SOCKET_ERROR )
	{
		ERROR_( "set receive buffer error.");
#ifdef _WIN32
		int iError = WSAGetLastError();
#else
		perror( "set recv buf size error.");
#endif
	}

	struct sockaddr_in cli_addr;

#ifdef _WIN32
	int nclilen = sizeof( cli_addr );
#else 
	socklen_t nclilen = sizeof( cli_addr );
#endif

	getsockname(  fd, ( struct sockaddr * )&cli_addr, &nclilen );

	strcpy( m_sLocalIP, inet_ntoa( cli_addr.sin_addr ) );
	m_iLocalPort = ntohs( cli_addr.sin_port );

	return 0;
}


int CClientPoint::ConnectToController( char *sIP, int iPort )
{
	strcpy( m_sServerIP, sIP );
	m_iServerPort = iPort;
	m_iCloseFlag = 1;
	return Connect();
}

char *CClientPoint::GetAddr()
{
	return m_sServerIP;
}

unsigned int CClientPoint::GetPort()
{

	return m_iServerPort;
}

char *CClientPoint::GetLocalAddr()
{
	return m_sLocalIP;
}

unsigned int CClientPoint::GetLocalPort()
{
	
	return m_iLocalPort;
}

int CClientPoint::SendBySizeT( void *buffer, int iSize, int timeout )
{
	if ( fd == INVALID_SOCKET )  return SOCKET_ERROR;

	if (WriteSelect( timeout ) < 0 ) return SOCKET_ERROR;

	if ( FD_ISSET( fd, &wfds ) )
	{

		if ( send( fd, (const char *)buffer, iSize, 0 ) == SOCKET_ERROR )
		{
			OutputDebugStr( "send error: " );
#ifdef _WIN32
			int iError = WSAGetLastError();
			switch( iError )
			{
			case WSANOTINITIALISED:
				OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n");
				break;
			case WSAENETDOWN:
				OutputDebugStr( "The network subsystem has failed. \n" );
				break;
			case WSAEACCES:
				OutputDebugStr( "The requested address is a broadcast address, but the appropriate flag was not set. Call setsockopt with the SO_BROADCAST parameter to allow the use of the broadcast address. \n" );
				break;
			case WSAEINTR:
				OutputDebugStr( "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall. \n" );
				break;
			case WSAEINPROGRESS:
				OutputDebugStr( "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
				break;
			case WSAEFAULT:
				OutputDebugStr( "The buf parameter is not completely contained in a valid part of the user address space. \n" );
				break;
			case WSAENETRESET:
				OutputDebugStr( "The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress. \n" );
				break;
			case WSAENOBUFS:
				OutputDebugStr("No buffer space is available. \n" );
				break;
			case WSAENOTCONN:
				OutputDebugStr( "The socket is not connected. \n" );
				break;
			case WSAENOTSOCK:
				OutputDebugStr("The descriptor is not a socket. \n" );
			case WSAEOPNOTSUPP:
				OutputDebugStr( "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations. \n" );
				break;
			case WSAESHUTDOWN:
				OutputDebugStr( "The socket has been shut down; it is not possible to send on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH. \n" );
				break;
			case WSAEWOULDBLOCK:
				OutputDebugStr( "The socket is marked as nonblocking and the requested operation would block. \n" );
				break;
			case WSAEMSGSIZE:
				OutputDebugStr( "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport. \n" );
				break;
			case WSAEHOSTUNREACH:
				OutputDebugStr( "The remote host cannot be reached from this host at this time. \n" );
				break;
			case WSAEINVAL:
				OutputDebugStr( "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled. \n" );
				break;	
			case WSAECONNABORTED:
				OutputDebugStr( "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable. \n" );
				break;
			case WSAECONNRESET:
				OutputDebugStr( "The virtual circuit was reset by the remote side executing a hard or abortive close. For UPD sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a \"Port Unreachable\" ICMP packet. The application should close the socket as it is no longer usable. \n" );
				break;
			case WSAETIMEDOUT:
				OutputDebugStr( "The connection has been dropped, because of a network failure or because the system on the other end went down without notice. \n" );
				break;
			}
#else
			perror( "select error");
#endif
			CloseClient();
			return SOCKET_ERROR;
		}
	}
	else return SOCKET_ERROR;
	return 0;

}

int CClientPoint::SendBySize( void *buffer, int iSize )
{
	if ( fd == INVALID_SOCKET )  return SOCKET_ERROR;

	if ( send( fd, (const char *)buffer, iSize, 0 ) == SOCKET_ERROR )
	{
		OutputDebugStr( "send error: " );
#ifdef _WIN32
		int iError = WSAGetLastError();
		switch( iError )
		{
		case WSANOTINITIALISED:
			OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n");
			break;
		case WSAENETDOWN:
			OutputDebugStr( "The network subsystem has failed. \n" );
			break;
		case WSAEACCES:
			OutputDebugStr( "The requested address is a broadcast address, but the appropriate flag was not set. Call setsockopt with the SO_BROADCAST parameter to allow the use of the broadcast address. \n" );
			break;
		case WSAEINTR:
			OutputDebugStr( "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall. \n" );
			break;
		case WSAEINPROGRESS:
			OutputDebugStr( "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
			break;
		case WSAEFAULT:
			OutputDebugStr( "The buf parameter is not completely contained in a valid part of the user address space. \n" );
			break;
		case WSAENETRESET:
			OutputDebugStr( "The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress. \n" );
			break;
		case WSAENOBUFS:
			OutputDebugStr("No buffer space is available. \n" );
			break;
		case WSAENOTCONN:
			OutputDebugStr( "The socket is not connected. \n" );
			break;
		case WSAENOTSOCK:
			OutputDebugStr("The descriptor is not a socket. \n" );
		case WSAEOPNOTSUPP:
			OutputDebugStr( "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations. \n" );
			break;
		case WSAESHUTDOWN:
			OutputDebugStr( "The socket has been shut down; it is not possible to send on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH. \n" );
			break;
		case WSAEWOULDBLOCK:
			OutputDebugStr( "The socket is marked as nonblocking and the requested operation would block. \n" );
			break;
		case WSAEMSGSIZE:
			OutputDebugStr( "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport. \n" );
			break;
		case WSAEHOSTUNREACH:
			OutputDebugStr( "The remote host cannot be reached from this host at this time. \n" );
			break;
		case WSAEINVAL:
			OutputDebugStr( "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled. \n" );
			break;	
		case WSAECONNABORTED:
			OutputDebugStr( "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable. \n" );
			break;
		case WSAECONNRESET:
			OutputDebugStr( "The virtual circuit was reset by the remote side executing a hard or abortive close. For UPD sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a \"Port Unreachable\" ICMP packet. The application should close the socket as it is no longer usable. \n" );
			break;
		case WSAETIMEDOUT:
			OutputDebugStr( "The connection has been dropped, because of a network failure or because the system on the other end went down without notice. \n" );
			break;
		}
#else
		perror( "send error");
#endif
		CloseClient();
		return SOCKET_ERROR;
	}
	return 0;
}


int CClientPoint::RecvBySizeT( void *buffer, int iSize, int timeout )
{
	if ( fd == INVALID_SOCKET ) return SOCKET_ERROR;
	int ret;
	int nBytesUnread = iSize;
	char *pData = (char *)buffer;
	
	while(nBytesUnread > 0) 
	{
		ret = ReadSelect(timeout);
		if ( ret<0 )
		{
			CloseClient();
			return SOCKET_ERROR;
		}
		else if ( ret==0 )
		{
			 return iSize-nBytesUnread;
		}

		ret = recv(fd, (char *)pData, nBytesUnread, 0);
		if(ret==0) return iSize - nBytesUnread;
		else if(ret == SOCKET_ERROR)
		{
			CloseClient();
			return SOCKET_ERROR; 
		}
		nBytesUnread -= ret;
		pData += ret;
	}
	return iSize;
}

int CClientPoint::RecvBySize( void *buffer, int iSize )
{
	if ( fd == INVALID_SOCKET ) return SOCKET_ERROR;

	if ( ( iSize = recv( fd, ( char * )buffer, iSize, 0 ) ) == SOCKET_ERROR )
	{

		OutputDebugStr( "recv error: ");

#ifdef _WIN32
		int iError = WSAGetLastError();
		switch( iError )
		{
		case WSANOTINITIALISED:
			OutputDebugStr( "A successful WSAStartup call must occur before using this function. \n" );
			break;

		case WSAENETDOWN:
			OutputDebugStr( "The network subsystem has failed. \n" );
			break;
		case WSAEFAULT:
			OutputDebugStr( "The buf parameter is not completely contained in a valid part of the user address space. \n" );
			break;
		case WSAENOTCONN:
			OutputDebugStr("The socket is not connected. \n" );
			break;
		case WSAEINTR:
			OutputDebugStr( "The (blocking) call was canceled through WSACancelBlockingCall. \n" );
			break;
		case WSAEINPROGRESS:
			OutputDebugStr( "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n" );
			break;
		case WSAENETRESET:
			OutputDebugStr("The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress. \n" );
			break;
		case WSAENOTSOCK:
			OutputDebugStr( "The descriptor is not a socket. \n" );
			break;
		case WSAEOPNOTSUPP:
			OutputDebugStr("MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations. \n" );
			break;
		case WSAESHUTDOWN:
			OutputDebugStr("The socket has been shut down; it is not possible to receive on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH. \n" );
			break;
		case WSAEWOULDBLOCK:
			OutputDebugStr( "The socket is marked as nonblocking and the receive operation would block. \n" );
			break;
		case WSAEMSGSIZE:
			OutputDebugStr( "The message was too large to fit into the specified buffer and was truncated. \n " );
			break;
		case WSAEINVAL:
			OutputDebugStr( "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative. \n" );
			break;
		case WSAECONNABORTED:
			OutputDebugStr( "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable. \n" );
			break;
		case WSAETIMEDOUT:
			OutputDebugStr( "The connection has been dropped because of a network failure or because the peer system failed to respond. \n" );
			break;
		case WSAECONNRESET:
			OutputDebugStr( "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket as it is no longer usable. On a UPD-datagram socket this error would indicate that a previous send operation resulted in an ICMP \"Port Unreachable\" message. \n" );
			break;
		}
#else
		perror( "recv error");
#endif
		CloseClient();
		return SOCKET_ERROR;
	}
	return iSize;
}

int CClientPoint::WriteSelect( int tm )
{
	struct timeval tv;
	if ( fd==INVALID_SOCKET ) return 0;
	tv.tv_sec = tm/1000;
	tv.tv_usec = tm%1000 * 1000;
	FD_ZERO( &wfds );
	FD_SET( fd, &wfds );
	return select( fd+1, NULL, &wfds, NULL, &tv);
}

int CClientPoint::ReadSelect(int tm)
{

	struct timeval tv;
	if ( fd==INVALID_SOCKET ) return 0;

	tv.tv_sec = tm/1000;
	tv.tv_usec = tm%1000 * 1000;

	FD_ZERO( &rfds );
	FD_SET( fd, &rfds );

	return select( fd+1, &rfds, NULL, NULL, &tv);
}

void CClientPoint::SetAutoReconnect(int nReconnect /*0:close; 1: open*/)
{
	if ( m_nAutoReconnect == 0 && nReconnect == 1 )
	{
#ifdef _WIN32
		m_hThreadId = CreateThread( NULL, NULL, ConnectionCheckProc, (void*)this, NULL, NULL );
#else
		pthread_create( &m_hThreadId, NULL, ConnectionCheckProc, (void*)this );

#endif
	}
	else if ( nReconnect == 0 )
	{
		m_nAutoReconnect = nReconnect;
		m_iExitFlag = 1;

#ifdef _WIN32
		if ( m_hThreadId != NULL )
		{
			WaitForSingleObject(m_hThreadId, 10000 );
		}
#else
		if ( m_hThreadId != 0 )
		{
			pthread_join( m_hThreadId, NULL );
		}

#endif 
	}

	m_nAutoReconnect = nReconnect;
}

#ifdef _WIN32
DWORD CClientPoint::ConnectionCheckProc( void *param )
#else
void *CClientPoint::ConnectionCheckProc( void *param )
#endif
{
	if ( param == NULL ) return NULL;
	CClientPoint *pParent = ( CClientPoint *)param;
	pParent->ConnectionCheckWorker();
#ifndef _WIN32
	pthread_detach( pthread_self());
#endif
	return NULL;
}

void CClientPoint::ConnectionCheckWorker()
{
	while( m_iExitFlag == 0  )
	{
		if ( fd != INVALID_SOCKET )
		{
#ifdef _WIN32
			Sleep( 200 );
#else
			usleep( 200 * 1000 );
#endif
			continue;
		}

		if ( Connect() == 0 ) 
		{
			continue;
		}
#ifdef _WIN32
		Sleep( 1000 );
#else
		usleep( 1000 * 1000 );
#endif	
	}
}

