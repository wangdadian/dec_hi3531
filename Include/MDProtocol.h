#ifndef MDPROTOCOL_H_
#define MDPROTOCOL_H_
#include "Protocol.h"
#include "netsync.h"
//#include "dwstype.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "PublicDefine.h"
#include "ChannelManager.h"
#include "Vmux6kProtocolDefine.h"

#pragma pack(1)


//万能解码器参数
typedef struct tagComDecoderParam
{
	unsigned short int iSlot;
	BYTE bChannel;				//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;				//码流号：1－4
	unsigned short int uiResolutionH;	//分辨率（H）
	unsigned short int uiResolutionV;	//分辨率（V）
	unsigned int uiPeerIP;
	unsigned int uiPeerPort;
	unsigned char bAudioFlag;	//音频开关 0=关闭 1=打开
	BYTE bAudioImport;			//音频输入源  0=line ,  1=mic
	unsigned char bProtocol;	//1＝TCP，2＝UDP，3＝组播
	unsigned char bDecoderFomat;	//编码格式: MPEG1=1, MPEG2=2, MPEG4=3 ,MJPEG=4 ,H264=5, JPEG2K=6 

	//============================================
	// 扩展参数
	unsigned int uiEncodecType;// 4 字节---编解码器类型，参照visCommHead.H中eCODEC_TYPE定义,unsigned int类型	
	unsigned int uiStreamType;// 4 字节---码流格式，参照visCommHead.H中eCODEC_VIDEO_FORMAT定义,unsigned int类型	
	char szUserName[32];		//DVR用户名
	char szPasswd[32];			//DVR密码
	unsigned int uiDvrChannel;// 4 字节---DVR通道号，说明：unsigned int类型	
	unsigned short usDecodecScreenId;// 2   字节---解码播放屏幕号，说明：unsigned short类型，万能解码器支持多屏幕输出	
	unsigned short usDecodecChannel;// 2 字节---解码播放窗口号，说明：unsigned short类型，万能解码器支持多窗口输出	
	//============================================
}COMDECODERPARAM;

typedef struct _tagDecDisplayParam
{
    /*
        4分屏屏幕顺序如下(9、16分屏类推):
        -----------
        | 1  | 2  |
        -----------
        | 3  | 4  |
        -----------
    */

    /*
      显示模式
      0-未知，1-VGA, 2-BNC,3-HDMI
     */
    int iDisplaymode;
    // 第N个显示通道，从1开始
    int iDisplayno;
    /*
      分辨率
      0-未知、未设置
      1-1920*1080
      2-800*600
      3-1024*768
      4-1280*1024
      5-1366*768
      6-1440*900
      7-1280*800
     */
    int iResolution;
    //关联的通道号，从1开始，0表示未绑定，128表示2x2组合通道，129表示3x3, 130表示4x4
    int iChannel;
}tDecDisplayParam;

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
	unsigned short slot_number;

	// 通道号
	BYTE channel;
	BYTE stream_id;
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
	BYTE b_showtime;

	// 显示OSD
	BYTE b_showfont;
	unsigned short reserved; // 复用为OSD颜色
}DecodecOsdParam;
enum OSD_TYPE
{
	OSD_FONT=1, // 标题
	OSD_TIME=2, // 时间
	OSD_CONTENT1=3,  // 内容1
	OSD_CONTENT2=4,  // 内容2
	OSD_CONTENT3=5,  // 内容3
	OSD_CONTENT4=6,  // 内容4
	OSD_MONITORID=7,// 监视器ID
	OSD_TYPE_MAX,   // 临界值
};
enum OSD_COLOR
{
    OSD_COLOR_WHITE = 1,// 白色
    OSD_COLOR_BLACK = 2,// 黑色
    OSD_COLOR_RED = 3,// 红色
    OSD_COLOR_GREEN = 4, // 绿色
    OSD_COLOR_BLUE = 5, // 蓝色
    OSD_COLOR_YELLOW = 6, // 黄色
    OSD_COLOR_PURPLE = 7, // 紫色
    OSD_COLOR_CYAN = 8, // 青色
    OSD_COLOR_MAGENTA = 9, // 洋红
};


typedef struct _MDMsg
{
	VMUX6KHEAD head;
	union
	{
        CMDRESPONSE response;
        COMDECODERPARAM decparam;
        COMDECODERPARAM_EX decexparam;
        DecodecOsdParam osd;
        tDecDisplayParam display;
        int iAck;
	}body;
}MDMsg;

#pragma pack()

class CMDProtocol : public CProtocol
{

public: 
	CMDProtocol( int nPort, CChannelManager *pChnManager );
	virtual ~CMDProtocol();
    int StopEncoder(int chnno);
private:
	CSyncNetServer *m_pServer;

	static void THWorker(CSyncServerNetEnd *net, void *p);

	
	int ProcessStartDecoder(  MDMsg &msg, MDMsg &ack);


	
	int ProcessShowOSD(  MDMsg &msg, MDMsg &ack);
    int ProcessDisplay(MDMsg &msg, MDMsg &ack);
	
	int ProcessCtrlRequest(CSyncServerNetEnd *net, MDMsg &msg, MDMsg &ack);
	
	int GetMsg( CSyncNetEnd *net, MDMsg &msg, int timeout);
	
	int SendMsg(CSyncNetEnd *net, MDMsg &msg);
	
	long BuildHead(VMUX6KHEAD& Head);

	
	long CheckHead(  VMUX6KHEAD Head);
	
	long BuildCmdResponse( MDMsg &ack, int msgType, int nRet );
	
	int m_iExitFlag;
	
	long BuildDecoderParamResponse( MDMsg msg, MDMsg &ack );

	COMDECODERPARAM m_decparam[MAXDECCHANN];

	
	COMDECODERPARAM_EX m_decexparam[MAXDECCHANN];
	
	
	pthread_mutex_t m_lock;

	void MSGNTOH( MDMsg &msg );
};


#endif


