// NetConnection.h: interface for the CNetConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NETCONNECTION_H__32EE0F75_33C7_47A8_95D2_CBFC4A80D4DD__INCLUDED_)
#define AFX_NETCONNECTION_H__32EE0F75_33C7_47A8_95D2_CBFC4A80D4DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ClientPoint.h"
#include "CommonDef.h"
#include <time.h>
#include "Vmux6kHeader.h"
//万能解码器参数
typedef struct tagComDecoderParam
{
	short int iSlot;
	BYTE bChannel;				//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;				//码流号：1－4
	short int uiResolutionH;	//分辨率（H）
	short int uiResolutionV;	//分辨率（V）
	unsigned int uiPeerIP;
	unsigned int uiPeerPort;
	unsigned char bAudioFlag;	//音频开关 0=关闭 1=打开
	BYTE bAudioImport;			//音频输入源  0=line ,  1=mic
	unsigned char bProtocol;	//1＝TCP，2＝UDP，3＝组播
	unsigned char bDecoderFomat;	//编码格式: MPEG1=1, MPEG2=2, MPEG4=3 ,MJPEG=4 ,H264=5, JPEG2K=6 

	//============================================
	// 扩展参数
	unsigned int uiEncodecType;	//4 字节---编解码器类型，参照visCommHead.H中eCODEC_TYPE定义,unsigned int类型	
	unsigned int uiStreamType;	//4 字节---码流格式，参照visCommHead.H中eCODEC_VIDEO_FORMAT定义,unsigned int类型	
	char szUserName[32];		//DVR用户名
	char szPasswd[32];			//DVR密码
	unsigned int uiDvrChannel;	//4 字节---DVR通道号，说明：unsigned int类型	
	unsigned short usDecodecScreenId;	//2 字节---解码播放屏幕号，说明：unsigned short类型，万能解码器支持多屏幕输出	
	unsigned short usDecodecChannel;	//2 字节---解码播放窗口号，说明：unsigned short类型，万能解码器支持多窗口输出	
	//============================================
}COMDECODERPARAM;

typedef struct tagComDecoderParam_EX
{
	COMDECODERPARAM DecoderParam;
	char szRTSPUrl[255];
}COMDECODERPARAM_EX;

enum CMD_DECODER_TYPE
{
	CMD_GET_DECODER_OSD_PARAM = 0x0206,  //获取编码器OSD参数(时间位置,标题位置,标题内容)
	CMD_SET_DECODER_OSD_PARAM = 0x0207,  //设置解码器OSD参数 
};

//#define GCMD_GET_CODER_OSD_PARAM                       0X0206 //获取编码器OSD参数(时间位置,标题位置,标题内容)
//#define GCMD_SET_DECODER_OSD_PARAM                     0X020B //设置解码器OSD参数 
typedef struct body_0206_0207_md
{
	unsigned int slot_number;

	// 通道号
	unsigned char channel;
	unsigned char stream_id;
	unsigned int osd;

	// 起始位置
	unsigned int x_time;
	unsigned int y_time;

	// 字体大小(参照系统字体大小)
	unsigned int x_font;
	unsigned int y_font;

	// OSD内容
	char buffer_content[64];

	// 显示系统时间
	unsigned char b_showtime;

	// 显示OSD
	unsigned char b_showfont;
	unsigned int reserved;
}DecodecOsdParam;

enum OSD_TYPE
{
	OSD_FONT=1,
	OSD_TIME=2,
};

class CNetConnection  
{
public:
	CNetConnection();
	virtual ~CNetConnection();
	CClientPoint *m_pClient;

	int NoteHeartBeatToServer();

	int ConnectToServer( char *szIpAddr, int uiPlayPort );
	int CloseClient();
	int Send( unsigned char *szData, int nLen );
	int Worker();
	char *GetLocalAddr();
	static DWORD WINAPI WorkProc( void *pParent );
	char szInfo[10240];

	THREADHANDLE m_hdlThread;
	bool m_bIsConnected;
	int m_iExitFlag;
	time_t tmold;

	int StartDecoder( int nChannelNo, char *szUrl );

	long BuildHead(VMUX6KHEAD& Head);

	long CheckHead(const VMUX6KHEAD& Head);



};

#endif // !defined(AFX_NETCONNECTION_H__32EE0F75_33C7_47A8_95D2_CBFC4A80D4DD__INCLUDED_)


