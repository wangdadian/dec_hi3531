#ifndef  __TCP__SERVER__WIN__
#define  __TCP__SERVER__WIN__

#include <vector>
#include "winsockdef.h"
#include <netinet/in.h>
#include <arpa/inet.h>

#include <list>
#include <map>
using namespace std;

// 数据处理函数指针
typedef void process_handler_fun(SOCKET s, char* in_buffer, int buf_len, char*& out_buffer, int& out_len, int& packetSize, void* pObject);

namespace server_space
{

#define DATA_BUFSIZE 1024			// 接收缓冲区大小
#define MAXSESSION 10000			// 最大连接数
#define HEART_BEAT_INTERVAL 20		//心跳间隔
#define closesocket(x) close(x)
	typedef void *LPVOID;
	typedef struct _WSABUF {
	    ULONG len;     /* the length of the buffer */
	    char  *buf; /* the pointer to the buffer */
	} WSABUF,  * LPWSABUF;


	typedef struct _OVERLAPPED
		{
		unsigned long Internal;
		unsigned long InternalHigh;
		union 
			{
			struct 
				{
				DWORD Offset;
				DWORD OffsetHigh;
				}	;
			void * Pointer;
			}	;
		HANDLE hEvent;
		}	OVERLAPPED;
	
	typedef struct _OVERLAPPED *LPOVERLAPPED;
	typedef struct _OVERLAPPED *    LPWSAOVERLAPPED;


	
	typedef struct _WSAOVERLAPPED {
		DWORD	 Internal;
		DWORD	 InternalHigh;
		DWORD	 Offset;
		DWORD	 OffsetHigh;
		HANDLE hEvent;
	} WSAOVERLAPPED,  * LPWSAOVERLAPPED;

	typedef struct _SOCKET_INFORMATION
	{
		OVERLAPPED Overlapped;
		SOCKET Socket;
		WSABUF DataBuf;
		DWORD BytesSEND;
		DWORD BytesRECV;
	} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

	typedef struct socket_map
	{
		LPSOCKET_INFORMATION recv_link;
		LPSOCKET_INFORMATION send_link;
	}socket_map_list;


	// 为每个客户端存储所有数据，直到有处理被处理
	typedef struct sock_data_pool
	{
		string data;

	}SOCK_DATA_POOL;


	// 按照套接字添加映射资源
	void AddSockMapInfo(SOCKET ,socket_map_list* pObj);

	// 按照套接字匹配接收数据池子
	void AddSockDataPool(SOCKET, SOCK_DATA_POOL*);

	// 按照套接字销毁对应的资源
	void DeleteSockMap(SOCKET sock);

	// 销毁套接字资源
	void CleanSocketInfo(LPSOCKET_INFORMATION obj);

	// 释放套接字的映射资源
	void ReleaseSockMapSource(LPSOCKET_INFORMATION obj);

	// 获取套接字的发送资源
	LPSOCKET_INFORMATION GetSendSockInfo(SOCKET sock, int bufferSize);

	// 获取一个新的套接字资源
	LPSOCKET_INFORMATION GetNewSockInfo(SOCKET sock ,int bufferSize);

	// 获取当前的客户端连接
	unsigned int GetCurrentSessionCount();

	// 设置回调函数，处理函数
	void SetProcessHandler(process_handler_fun* fun, void *pObject);

	// 数据处理中心函数
	void DataProcessCenter(SOCKET sock,			// 套接字
							char* in_buf,		// 进去的数据指针
							int in_len,			// 数据长度
							char*& out_buf,		// 返回的数据指针
							int& out_len);		// 返回的数据长度

	// 程序退出时候释放资源
	void DeleteResource();

	class TcpServerWin
	{
	public:
		// 构造函数
		TcpServerWin();

		// 析构函数
		~TcpServerWin();

		// 开启服务
		void Start(int port);

	public:
		// 接收线程函数
		static  DWORD WINAPI AcceptThread(LPVOID lpParameter);

		// 检测心跳函数
		static  DWORD WINAPI CheckHeartBeat(LPVOID lpParameter);

		// 接收数据回调函数
		void   RecvWorkerRoutine(DWORD Error, DWORD BytesTransferred,LPWSAOVERLAPPED Overlapped, DWORD InFlags);

		// 发送数据回调函数
		void   SendWorkerRoutine(DWORD Error, DWORD BytesTransferred,LPWSAOVERLAPPED Overlapped, DWORD InFlags);

	private:
		// 初始化环境
		SOCKET  m_ListenSocket;
		unsigned int m_port;
	};
}

#endif

