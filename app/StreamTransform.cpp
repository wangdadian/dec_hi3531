
#include "StreamTransform.h"
//#include "VasCommHead.h"
#include "PublicDefine.h"
#include "VideoHead.h"

#define MAX_BUF_COUNT 100 * 1024


#ifndef INFO 
#define INFO _DEBUG_
#endif

#ifndef ERROR 
#define ERROR _DEBUG_
#endif

CStreamTransform::CStreamTransform()
{
	m_nDstCount = 0;
	m_nSrcCount = 0;
	m_thRecv = 0;
	m_iExitFlag = 0;
	m_pCaller = NULL;

	m_pHeartBeatData = NULL;
	m_nHeartBeatDataLen = 0;
	m_nHeartBeatTimeout = 30;
	tm_lastHB = 0;
	m_szBuf = (char*)malloc( MAX_BUF_COUNT );
	m_szTmpBuf = (char*)malloc( MAX_BUF_COUNT );

	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		m_dstInfo[i].iDstType = 0;
		m_dstInfo[i].iStreamNo = 0;
		m_dstInfo[i].nDataLen = 0;
		m_dstInfo[i].nDataLen2 = 0;
		m_dstInfo[i].nPort = 0;
		m_dstInfo[i].pDst = NULL;
		m_dstInfo[i].pReqData = NULL;
		m_dstInfo[i].pReqData2 = NULL;
		m_dstInfo[i].nRtpFlag = 0;
		m_dstInfo[i].nSeqNo = 0;
		memset( m_dstInfo[i].szIp, 0, sizeof( m_dstInfo[i].szIp ) );


		m_srcInfo[i].iDstType = 0;
		m_srcInfo[i].iStreamNo = 0;
		m_srcInfo[i].nDataLen = 0;
		m_srcInfo[i].nDataLen2 = 0;
		m_srcInfo[i].nPort = 0;
		m_srcInfo[i].pDst = NULL;
		m_srcInfo[i].pReqData = NULL;
		m_srcInfo[i].pReqData2 = NULL;
		memset( m_srcInfo[i].szIp, 0, sizeof( m_srcInfo[i].szIp ) );
	}
	
	InitializeCriticalSection( &m_streamlock );
}

CStreamTransform::~CStreamTransform()
{

	m_iExitFlag = 1;
	if ( m_thRecv != 0 )
	{
		pthread_join( m_thRecv, NULL );
	}
	
	EnterCriticalSection( &m_streamlock );
	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		if ( m_dstInfo[i].pDst != NULL )
		{

			switch( m_dstInfo[i].iDstType )
			{
			case (int)StreamCastType_Multicast:
			case (int)StreamCastType_Unicast:
				{ 
					CNetUdp *pNetUdp = ( CNetUdp*)m_dstInfo[ i].pDst; 
					pNetUdp->CloseUdp();
					delete pNetUdp;
					m_dstInfo[i].pDst = NULL;
					break;
				}
			case (int)StreamCastType_TcpServer:
			case (int)StreamCastType_TcpClient:	
				{ //3:TCP服务端模式：TCP
					CNetTcp *pNetTcp = ( CNetTcp* )m_dstInfo[ i].pDst; 
					pNetTcp->CloseTcp();
					delete pNetTcp;
					m_dstInfo[i].pDst = NULL;
					break;
				}
			default:
				break;
			
		   };
			
			m_dstInfo[i].pDst = NULL;
		}
	}	

	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		if ( m_srcInfo[i].pDst != NULL )
		{
			switch( m_srcInfo[i].iDstType )
			{
			case (int)StreamCastType_Multicast:
			case (int)StreamCastType_Unicast:
				{ 
					CNetUdp *pNetUdp = ( CNetUdp*)m_srcInfo[ i].pDst; 
					pNetUdp->CloseUdp();
					delete pNetUdp;
					m_srcInfo[i].pDst = NULL;
					break;
				}
			case (int)StreamCastType_TcpServer:
			case (int)StreamCastType_TcpClient:	
				{ //3:TCP服务端模式：TCP
					CNetTcp *pNetTcp = ( CNetTcp* )m_srcInfo[ i].pDst; 
					pNetTcp->CloseTcp();
					delete pNetTcp;
					m_srcInfo[i].pDst = NULL;
					break;
				}
			default:
				break;
			
		   };
			
			m_srcInfo[i].pDst = NULL;
		}
	}	
	LeaveCriticalSection( &m_streamlock );

	if ( m_szBuf != NULL )
	{
		delete m_szBuf;
		m_szBuf = NULL;
	}

	if (m_szTmpBuf != NULL )
	{
		delete m_szTmpBuf;
		m_szTmpBuf = NULL;
	}
	DeleteCriticalSection( &m_streamlock );
}


void CStreamTransform::StopStreamSrc( int iStreamNo )
{

	INFO( "stop stream src" );
	for ( int i = 0; i < MAXDSTCNT; i++ )
	{

		if ( m_srcInfo[i].iStreamNo == iStreamNo ) 
		{
			if ( iStreamNo != 0 )
			{
				m_nSrcCount--;
			}
		}
		
		if ( m_srcInfo[i].iStreamNo == iStreamNo && 
			m_srcInfo[i].pDst != NULL )
		{
			m_nSrcCount--;
			switch( m_srcInfo[i].iDstType )
			{
			case (int)StreamCastType_Multicast:
			case (int)StreamCastType_Unicast:
				{ 
					CNetUdp *pNetUdp = ( CNetUdp*)m_srcInfo[ i].pDst; 
					pNetUdp->CloseUdp();
					
					EnterCriticalSection( &m_streamlock );
					delete pNetUdp;
					m_srcInfo[i].pDst = NULL;
					
					LeaveCriticalSection( &m_streamlock );
					break;
				}
			case (int)StreamCastType_TcpServer:
			case (int)StreamCastType_TcpClient:	
				{ //3:TCP服务端模式：TCP
					CNetTcp *pNetTcp = ( CNetTcp* )m_srcInfo[ i].pDst; 
					pNetTcp->CloseTcp();
					
					EnterCriticalSection( &m_streamlock );
					delete pNetTcp;
					m_srcInfo[i].pDst = NULL;
					LeaveCriticalSection( &m_streamlock );
					break;
				}
			default:
				break;
			
		   };
			
		}
		
		EnterCriticalSection( &m_streamlock );
		m_srcInfo[i].iStreamNo = 0;
		m_srcInfo[i].pDst = NULL;
		m_srcInfo[i].szIp[0] = 0;
		m_srcInfo[i].nPort = 0;

		if ( m_srcInfo[i].pReqData != NULL )
		{
			free( m_srcInfo[i].pReqData );
			m_srcInfo[i].pReqData = NULL;
			m_srcInfo[i].nDataLen = 0;
		}

		if ( m_srcInfo[i].pReqData2 != NULL )
		{
			free( m_srcInfo[i].pReqData2 );
			m_srcInfo[i].pReqData2 = NULL;
			m_srcInfo[i].nDataLen2 = 0;
		}
		
		LeaveCriticalSection( &m_streamlock );
		break;
	}
}


void CStreamTransform::StopStreamDst( int iStreamNo )
{
	INFO( "stop stream dst\n" );
	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		if ( m_dstInfo[i].iStreamNo == iStreamNo && 
			m_dstInfo[i].pDst != NULL )
		{
			if ( m_dstInfo[i].iStreamNo == iStreamNo ) 
			{
				if ( iStreamNo != 0 )
				{
					m_nDstCount--;
				}
			}
		
			switch( m_dstInfo[i].iDstType )
			{
			case (int)StreamCastType_Multicast:
			case (int)StreamCastType_Unicast:
				{ 
					INFO("stop udp");
					CNetUdp *pNetUdp = ( CNetUdp*)m_dstInfo[ i].pDst; 
					pNetUdp->CloseUdp();
					
					EnterCriticalSection( &m_streamlock );
					delete pNetUdp;
					m_dstInfo[i].pDst = NULL;
					INFO("stop udp ok");
					
					LeaveCriticalSection( &m_streamlock );
					break;
				}
			case (int)StreamCastType_TcpServer:
			case (int)StreamCastType_TcpClient:	
				{ //3:TCP服务端模式：TCP
					INFO("stop tcp");
					CNetTcp *pNetTcp = ( CNetTcp* )m_dstInfo[ i].pDst; 
					pNetTcp->CloseTcp();
					
					EnterCriticalSection( &m_streamlock );
					delete pNetTcp;
					m_dstInfo[i].pDst = NULL;
					INFO("stop tcp ok");
					LeaveCriticalSection(&m_streamlock);
					break;
				}
			default:
				break;
			
		   };
			
			EnterCriticalSection( &m_streamlock );
			m_dstInfo[i].iStreamNo = 0;
			m_dstInfo[i].pDst = NULL;
			m_dstInfo[i].szIp[0] = 0;
			m_dstInfo[i].nPort = 0;
			m_dstInfo[i].nRtpFlag = 0;
			m_dstInfo[i].nSeqNo = 0;
			
			LeaveCriticalSection( &m_streamlock );
			break;
		}
	}
	INFO("stop stream dst ok");
}

void CStreamTransform::SetStreamDst( int iStreamNo, 
				   int nDstSendType,
				   char *szIP,
				   int nPort, 
				   int nRtpFlag)
{
	INFO( "set stream dst stream no:%d ip:%s port:%d", 
		iStreamNo, szIP, nPort );
	
	int iExistFlag = -1;
	try
	{
		for ( int i = 0; i < MAXDSTCNT; i++ )
		{

			INFO( "dst stream:%d", m_dstInfo[i].iStreamNo );
			if ( m_dstInfo[i].iStreamNo == iStreamNo && 
				strcmp( m_dstInfo[i].szIp, szIP ) == 0 && 
				m_dstInfo[i].nPort == nPort )
			{
				m_dstInfo[i].nRtpFlag = nRtpFlag;
				return;
			}
	        // 如果码流好相同，则先关闭之前的连接重新设置添加
			else if ( m_dstInfo[i].iStreamNo == iStreamNo && 
				m_dstInfo[i].pDst != NULL )
			{
				switch( m_dstInfo[i].iDstType )
				{
				// 组播、单播
				case (int)StreamCastType_Multicast:
				case (int)StreamCastType_Unicast:
					{ 
						CNetUdp *pNetUdp = ( CNetUdp*)m_dstInfo[ i].pDst; 
						pNetUdp->CloseUdp();
						
						EnterCriticalSection( &m_streamlock );
						delete pNetUdp;
						m_dstInfo[i].pDst = NULL;
						m_dstInfo[i].nRtpFlag = nRtpFlag;
						
						LeaveCriticalSection( &m_streamlock );
						break;
					}
				case (int)StreamCastType_TcpServer:
				case (int)StreamCastType_TcpClient:	
					{ //3:TCP服务端模式：TCP
						CNetTcp *pNetTcp = ( CNetTcp* )m_dstInfo[ i].pDst; 
						pNetTcp->CloseTcp();
						
						EnterCriticalSection( &m_streamlock );
						delete pNetTcp;
						m_dstInfo[i].pDst = NULL;
						m_dstInfo[i].nRtpFlag = 0;
						
						LeaveCriticalSection( &m_streamlock );
						break;
					}
				default:
					break;
				
			   };
				
				iExistFlag = i;
				break;
			}
		}	
	}
	catch( ... )
	{


	}

	if ( iExistFlag < 0 )
	{

		for ( int i = 0; i < MAXDSTCNT; i++ )
		{
			if ( m_dstInfo[i].iStreamNo == 0 )
			{
				iExistFlag = i;
			}
		}
	
	}
	
	if ( iExistFlag < 0 )
	{
		ERROR( "no stream dst info. dstcoutn:%d", m_nDstCount );
		return;
	}
	m_nDstCount++;
	
	EnterCriticalSection( &m_streamlock );
	m_dstInfo[ iExistFlag ].iStreamNo = iStreamNo;
	m_dstInfo[ iExistFlag ].iDstType = nDstSendType;
	strncpy( m_dstInfo[ iExistFlag ].szIp, szIP, 50 );
	m_dstInfo[ iExistFlag ].nPort = nPort;
	
	LeaveCriticalSection( &m_streamlock );
	try
	{
		switch ( nDstSendType )
		{
		case (int)StreamCastType_Multicast:
		case (int)StreamCastType_Unicast:
			{ 

				INFO("start udp:%s %d", szIP, nPort );
				//2:UDP单播模式：UDP
				CNetUdp *pNetUdp = new CNetUdp();
				pNetUdp->Open( nPort, szIP, true );
				
				EnterCriticalSection( &m_streamlock );
				m_dstInfo[ iExistFlag].pDst = (void*)pNetUdp;
				m_dstInfo[iExistFlag].nRtpFlag = nRtpFlag;
				
				LeaveCriticalSection( &m_streamlock );
				break;
			}
		case (int)StreamCastType_TcpServer:
			{
				//3:TCP服务端模式：TCP
				INFO("start tcp server:%s %d", szIP, nPort );
				CNetTcp *pNetTcp = new CNetTcp();
				pNetTcp->Open( nPort, szIP, false );
				
				EnterCriticalSection( &m_streamlock );
				m_dstInfo[ iExistFlag].pDst = (void*)pNetTcp;
				m_dstInfo[iExistFlag].nRtpFlag = 0;
				
				LeaveCriticalSection( &m_streamlock );
				break;
			}
		case (int)StreamCastType_TcpClient:
			{
				//4:TCP客户端模式：TCP
				INFO("start tcp client:%s %d", szIP, nPort );
				CNetTcp *pNetTcp = new CNetTcp();
				pNetTcp->Open( nPort, szIP, true );
				pNetTcp->Connect();
				
				EnterCriticalSection( &m_streamlock );
				m_dstInfo[ iExistFlag].pDst = (void*)pNetTcp;
				m_dstInfo[iExistFlag].nRtpFlag = 0;
				
				LeaveCriticalSection( &m_streamlock );
				break;
			}
		default:
			{
				//错误
				ERROR( " unknown Transfer Type " );
				return ;
			}
		}
	}
	catch( ... )
	{


	}
	if ( m_thRecv == 0 )
	{
		INFO( "create recv thread\n" );
		pthread_create( &m_thRecv, NULL, ThreaProc, (void*)this );
	}

	

	
}


void CStreamTransform::SetStreamRecvCB( StreamRecvCB callback, void *pCaller )
{
	m_recvCB = callback;
	m_pCaller = pCaller;
}

void CStreamTransform::SetStreamSrc( int iStreamNo, 
				   int nSrcType,
				   char *szIP,
				   int nPort )
{
	printf( "set stream src\n" );
	int iExistFlag = -1;
	EnterCriticalSection( &m_streamlock );
	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		if ( m_srcInfo[i].iStreamNo == iStreamNo && 
			strcmp( m_srcInfo[i].szIp, szIP ) == 0 && 
			m_srcInfo[i].nPort == nPort )
		{
		    //iExistFlag = i;
			LeaveCriticalSection( &m_streamlock );
			return;
		}
		else if ( m_srcInfo[i].iStreamNo == iStreamNo && 
			m_srcInfo[i].pDst != NULL )
		{
			switch( m_srcInfo[i].iDstType )
			{
			case (int)StreamCastType_Multicast:
			case (int)StreamCastType_Unicast:
				{ 
					CNetUdp *pNetUdp = ( CNetUdp*)m_srcInfo[ i].pDst; 
					pNetUdp->CloseUdp();
					delete pNetUdp;
					m_srcInfo[i].pDst = NULL;
					break;
				}
			case (int)StreamCastType_TcpServer:
			case (int)StreamCastType_TcpClient:	
				{ //3:TCP服务端模式：TCP
					CNetTcp *pNetTcp = ( CNetTcp* )m_srcInfo[ i].pDst; 
					pNetTcp->CloseTcp();
					delete pNetTcp;
					m_srcInfo[i].pDst = NULL;
					break;
				}
			default:
				break;
			
		   };
			
			m_srcInfo[i].pDst = NULL;
			iExistFlag = i;
			break;
		}
	}	

	if ( iExistFlag < 0 )
	{

		for ( int i = 0; i < MAXDSTCNT; i++ )
		{
			if ( m_dstInfo[i].iStreamNo == 0 )
			{
				iExistFlag = i;
			}
		}
	
	}


	if ( iExistFlag < 0 )
	{
		LeaveCriticalSection( &m_streamlock );
		ERROR( "no stream src info." );
		return;
	}

	m_nSrcCount++;
	
	m_srcInfo[ iExistFlag ].iStreamNo = iStreamNo;
	m_srcInfo[ iExistFlag ].iDstType = nSrcType;
	strncpy( m_srcInfo[ iExistFlag ].szIp, szIP, 50 );
	m_srcInfo[ iExistFlag ].nPort = nPort;

	INFO( "start open net work " );

	switch ( nSrcType )
	{
	case (int)StreamCastType_Multicast:
	case (int)StreamCastType_Unicast:
		{
			if ( m_srcInfo[iExistFlag].pDst == NULL )
			{
				//2:UDP单播模式：UDP
				CNetUdp *pNetUdp = new CNetUdp();
				pNetUdp->Open( nPort, szIP, false );
				m_srcInfo[ iExistFlag].pDst = (void*)pNetUdp;
			}
			break;
		}
	case (int)StreamCastType_TcpServer:
		{//3:TCP服务端模式：TCP
			if ( m_srcInfo[iExistFlag].pDst == NULL )
			{
				CNetTcp *pNetTcp = new CNetTcp();
				pNetTcp->Open( nPort, szIP, true );
				pNetTcp->Connect();
				m_srcInfo[ iExistFlag].pDst = (void*)pNetTcp;
			}
			break;
		}
	case (int)StreamCastType_TcpClient:
		{//4:TCP客户端模式：TCP
			if ( m_srcInfo[iExistFlag].pDst == NULL )
			{
				CNetTcp *pNetTcp = new CNetTcp();
				pNetTcp->Open( nPort, szIP, false );
				pNetTcp->Connect();
				m_srcInfo[ iExistFlag].pDst = (void*)pNetTcp;
			}
			break;
		}
	default:
		{//错误
			ERROR( " unknown TransferType " );
			LeaveCriticalSection( &m_streamlock );
			return ;
		}
	}

	INFO( "end .open net work " );
	
	LeaveCriticalSection( &m_streamlock );
	
}

void *CStreamTransform::ThreaProc( void *arg )
{
	CStreamTransform *pThis = ( CStreamTransform *)arg;
	while( pThis->m_iExitFlag == 0 )
	{
		pThis->worker();
	}
	return NULL;
}

void CStreamTransform::worker()
{
	//printf( "worker \n" );

    int iNeedWait = 1;
	EnterCriticalSection(&m_streamlock );
	
	try
	{
		for ( int i = 0; i < MAXDSTCNT; i++ )
		{
			if ( m_srcInfo[i].pDst == NULL ) 
			{
				continue;
			}
			//printf( "process stream no:%d\n", m_srcInfo[i].iStreamNo );
			switch( m_srcInfo[i].iDstType )
			{
				case (int)StreamCastType_Multicast:
				case (int)StreamCastType_Unicast:
					{ 
						CNetUdp *pNetUdp = ( CNetUdp*)m_srcInfo[ i].pDst; 
						int nRet = pNetUdp->GetDataNT( m_szBuf, MAX_BUF_COUNT, 1 );

						if ( nRet > 0 && m_recvCB != NULL )
						{
							m_recvCB( m_srcInfo[i].iStreamNo, m_szBuf, nRet, m_pCaller );
							iNeedWait |= 2;
							time_t tmnow;
							tmnow = time(&tmnow );
							m_srcInfo[i].tmLastReceived = tmnow;
						}
						else if ( nRet == 0 )
						{
							iNeedWait |= 4;
						}
						else if ( nRet < 0 )
						{
							iNeedWait |= 8;
						}
						
						break;
					}
				case (int)StreamCastType_TcpServer:
				case (int)StreamCastType_TcpClient:	
					{
						//3:TCP服务端模式：TCP
						CNetTcp *pNetTcp = ( CNetTcp* )m_srcInfo[ i].pDst; 
						
						time_t tmold;
						tmold = time(&tmold );

						if ( tmold - tm_lastHB > m_nHeartBeatTimeout && 
							m_pHeartBeatData != NULL &&
							m_nHeartBeatDataLen > 0 )
						{
							pNetTcp->SendData( m_pHeartBeatData, m_nHeartBeatDataLen );
							tm_lastHB = tmold;
							m_srcInfo[i].tmLastReceived = tmold;
						}

						int nGetLen = MAX_BUF_COUNT;
						int nRet = pNetTcp->GetData( (void*)m_szBuf, nGetLen, 5 );

						time_t tmnew;
						tmnew = time(&tmnew );
						if ( tmnew - tmold >= 3 )
						{
							ERROR( "get data spend:%d received:%d" , (int)(tmnew - tmold), nRet );
						}
						//printf( "received :%d\n", nRet );
						if ( nRet > 0 && m_recvCB != NULL )
						{
							m_recvCB( m_srcInfo[i].iStreamNo, m_szBuf, nRet, m_pCaller );
							iNeedWait |= 2;
						}
						else if ( nRet == 0 )
						{
							//usleep( 1000 );
							iNeedWait |= 4;
						}
						else if ( nRet < 0 || (tmnew - m_srcInfo[i].tmLastReceived ) > 10 )
						{
							INFO( "%s reconnect ...", m_srcInfo[i].szIp );
							
							if ( m_srcInfo[i].iDstType == (int)StreamCastType_TcpServer )
							{
								pNetTcp->Open( m_srcInfo[ i].nPort, m_srcInfo[ i].szIp, true );
								nRet = pNetTcp->Connect();
								//iNeedWait = 100000;
							}
							
							if ( nRet >= 0 && m_srcInfo[i].pReqData != NULL &&
								m_srcInfo[i].nDataLen > 0 )
							{							
								pNetTcp->SendData( m_srcInfo[i].pReqData, m_srcInfo[i].nDataLen );
							}
							else if ( nRet >= 0 && m_srcInfo[i].pReqData2 != NULL &&
								m_srcInfo[i].nDataLen2 > 0 )
							{							
								pNetTcp->SendData( m_srcInfo[i].pReqData2, m_srcInfo[i].nDataLen2 );
							}

							iNeedWait |= 8;
						}
						break;
					}
				default:
					break;
			};
		}
	}
	catch( ... )
	{


	}
	LeaveCriticalSection(&m_streamlock );

	if (  iNeedWait == 1 )//没有源的时候， 停止较长时间
	{
		//printf( "sleep 1000 \n");
		usleep( 1000 );
	}
	else if ( (iNeedWait & 2) != 0 ) //当正确获取码流时， 不延时
	{
		//printf( "sleep 0 \n");
		usleep(1);
	}
	else if ( (iNeedWait & 4) != 0 ) //当获取超时时， 延时10ms
	{
		//printf( "sleep 10 \n");
		usleep( 10 );
	}
	else if ( (iNeedWait & 8) != 0 ) 
	{
		//printf( "sleep 1000 \n");
		usleep( 1000 );
	}
    else
    {
        usleep(100);
    }
	//INFO( "end worker" );
	
}


void CStreamTransform::SetHeartBeatData( void *data, int nLen, int timeout )
{
	if ( nLen <= 0 || data == NULL ) return;
	
	EnterCriticalSection( &m_streamlock );
	if ( m_pHeartBeatData != NULL )
	{
		free( m_pHeartBeatData );
		m_pHeartBeatData = NULL;
	}

	m_pHeartBeatData = (unsigned char*)malloc( nLen );
	memcpy( m_pHeartBeatData, data, nLen );
	m_nHeartBeatDataLen = nLen;
	m_nHeartBeatTimeout = timeout;
	LeaveCriticalSection( &m_streamlock );

}

void CStreamTransform::SetSrcReqData(
						int iStreamNo, 
						char *szData, 
						int nLen, 
						int iDataIndex )
{
	INFO( "set src req data index:%d", iDataIndex );
	EnterCriticalSection( &m_streamlock );

	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		if ( iStreamNo == 0 ) //发送所有
		{

			if ( iDataIndex == 1 )
			{
				if ( m_srcInfo[i].pReqData != NULL )
				{
					free( m_srcInfo[i].pReqData );
					m_srcInfo[i].pReqData = NULL;
					m_srcInfo[i].nDataLen = 0;
				}

				m_srcInfo[i].pReqData = malloc( nLen);
				memcpy( m_srcInfo[i].pReqData, szData, nLen );
				m_srcInfo[i].nDataLen = nLen;
			}
			else if ( iDataIndex == 2 )
			{
				if ( m_srcInfo[i].pReqData2 != NULL )
				{
					free( m_srcInfo[i].pReqData2 );
					m_srcInfo[i].pReqData2 = NULL;
					m_srcInfo[i].nDataLen2 = 0;
				}

				m_srcInfo[i].pReqData2 = malloc( nLen);
				memcpy( m_srcInfo[i].pReqData2, szData, nLen );
				m_srcInfo[i].nDataLen2 = nLen;
			}
		}
		else if ( m_srcInfo[i].iStreamNo == iStreamNo )
		{

			if ( iDataIndex == 1 )
			{
				if ( m_srcInfo[i].pReqData != NULL )
				{
					free( m_srcInfo[i].pReqData );
					m_srcInfo[i].pReqData = NULL;
					m_srcInfo[i].nDataLen = 0;
				}

				m_srcInfo[i].pReqData = malloc( nLen);
				memcpy( m_srcInfo[i].pReqData, szData, nLen );
				m_srcInfo[i].nDataLen = nLen;
			}
			else if ( iDataIndex == 2 )
			{
				if ( m_srcInfo[i].pReqData2 != NULL )
				{
					free( m_srcInfo[i].pReqData2 );
					m_srcInfo[i].pReqData2 = NULL;
					m_srcInfo[i].nDataLen2 = 0;
				}

				m_srcInfo[i].pReqData2 = malloc( nLen);
				memcpy( m_srcInfo[i].pReqData2, szData, nLen );
				m_srcInfo[i].nDataLen2 = nLen;
			}

		}
	
	}
	LeaveCriticalSection( &m_streamlock );
	INFO( "end set src req data" );
}

void CStreamTransform::SendStream( int iStreamNo, 
				  char *szData, 
				  int nLen )
{
	EnterCriticalSection( &m_streamlock );
	try
	{
		for ( int i = 0; i < MAXDSTCNT; i++ )
		{
			if ( m_dstInfo[i].iStreamNo == iStreamNo && 
				m_dstInfo[i].pDst != NULL )
			{
				int nMaxSend = 1400;
				int nSended = 0;
				for ( int j = 0; j <= nLen / nMaxSend; j++ )
				{

					int nToBeSend = 0;
					
					if ( ((j+1) * nMaxSend) > nLen )
					{
						nToBeSend = ( nLen - (j)*nMaxSend );
					}
					else 
					{
						nToBeSend = nMaxSend;
					}
					
					switch( m_dstInfo[i].iDstType )
					{
						case (int)StreamCastType_Multicast:
						case (int)StreamCastType_Unicast:
							{ 
								CNetUdp *pNetUdp = ( CNetUdp*)m_dstInfo[ i].pDst; 

								if ( m_dstInfo[i].nRtpFlag > 0 )
								{
									RTPHeader header;
									memset( &header, 0, sizeof( header ));
									header.version = 2;
									time_t tm;
									header.timestamp = time( &tm );
									header.payloadtype = 96;
									header.ssrc = 0x1000000;
									if ( j== 0 )
									{
										header.marker = 1;
									}
									m_dstInfo[i].nSeqNo++;
									header.sequencenumber = m_dstInfo[i].nSeqNo;
									
									memcpy( m_szTmpBuf,&header, sizeof(header));
									memcpy( m_szTmpBuf + sizeof(header), szData + nSended, nToBeSend );
									pNetUdp->SendData( (void*)m_szTmpBuf, nToBeSend + sizeof(header));
									
								}
								else 
								{
									pNetUdp->SendData( (void*)(szData+nSended), nToBeSend );
								}
								break;
							}
						case (int)StreamCastType_TcpServer:
							{
								//3:TCP服务端模式：TCP
								CNetTcp *pNetTcp = ( CNetTcp*)m_dstInfo[ i].pDst; 
								int nRet = pNetTcp->SendLargeData( (void*)(szData+nSended), nToBeSend);
								if ( nRet < 0 )
								{
									pNetTcp->Open( m_dstInfo[i].nPort, m_dstInfo[i].szIp, false );
									//pNetTcp->Connect();
								}
								break;
							}
						case (int)StreamCastType_TcpClient:	
							{//3:TCP服务端模式：TCP
								CNetTcp *pNetTcp = ( CNetTcp*)m_dstInfo[ i].pDst; 
								int nRet = pNetTcp->SendLargeData( (void*)(szData+nSended), nToBeSend);
								if ( nRet < 0 )
								{
									pNetTcp->Open( m_dstInfo[i].nPort, m_dstInfo[i].szIp, true );
									pNetTcp->Connect();
								}
								break;
							}
						default:
							break;
					}

					nSended += nToBeSend;
				};
			}
		}
	}
	catch( ... )
	{

	}

	if ( m_nDstCount >= MAXDSTCNT )
	{
		ERROR( "no stream dst info." );
		LeaveCriticalSection( &m_streamlock );
		return;
	}
	LeaveCriticalSection( &m_streamlock );
}

void CStreamTransform::SendSrcStreamReq( int iStreamNo, 
										char *szData, 
										int nLen )
{
	SetSrcReqData( iStreamNo, szData, nLen );
	EnterCriticalSection(&m_streamlock );
	for ( int i = 0; i < MAXDSTCNT; i++ )
	{
		
		if ( m_srcInfo[i].iStreamNo == iStreamNo && 
			m_srcInfo[i].pDst != NULL )
		{
			switch( m_srcInfo[i].iDstType )
			{
				case (int)StreamCastType_Multicast:
				case (int)StreamCastType_Unicast:
					{ 
						CNetUdp *pNetUdp = ( CNetUdp*)m_srcInfo[ i].pDst; 
						pNetUdp->SendData( szData, nLen );
						break;
					}
				case (int)StreamCastType_TcpServer:
						{
							INFO( "cp10");
							//3:TCP服务端模式：TCP
							CNetTcp *pNetTcp = ( CNetTcp*)m_srcInfo[i].pDst; 
							INFO("cp 20: 0x%lx  len:%d", (long)pNetTcp, nLen );
							int nRet = pNetTcp->SendLargeData( szData, nLen );
							INFO( "send stream req data stream:%d len:%d ret:%d\n", 
								iStreamNo, nLen, nRet );
							if ( nRet <= 0 )
							{
								INFO( "recconect .. " );
								pNetTcp->Open( m_srcInfo[i].nPort, m_srcInfo[i].szIp, true );
								pNetTcp->Connect();
								pNetTcp->SendLargeData( szData, nLen );
							}
							break;
						}
				case (int)StreamCastType_TcpClient:	

						{//3:TCP服务端模式：TCP
							CNetTcp *pNetTcp = ( CNetTcp*)m_srcInfo[ i].pDst; 
							int nRet = pNetTcp->SendLargeData( szData, nLen );
							INFO( "send stream req data stream:%d len:%d ret:%d\n", 
								iStreamNo, nLen, nRet );
							if ( nRet <= 0 )
							{
								pNetTcp->Open( m_srcInfo[i].nPort, m_srcInfo[i].szIp, false );
								pNetTcp->Connect();
								pNetTcp->SendLargeData( szData, nLen );
							}
							break;
						}
				default:
					break;
			};
		}
	}
	LeaveCriticalSection(&m_streamlock );



	
}

