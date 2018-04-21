#ifndef __VMUX6K_PROTOCOL_H_2015
#define __VMUX6K_PROTOCOL_H_2015

typedef unsigned char BYTE;

#pragma pack(1)

//协议头
typedef struct tagVmux6kHead
{
	BYTE bHead1;
	BYTE bHead2;
	BYTE bVersion;
	BYTE bFlag;
	BYTE bMsgType;
	BYTE bLength_h;
	BYTE bLength_l;
	BYTE bChecksum;
}VMUX6KHEAD;



//各命令ACK返回值
typedef struct tagCmdResponse
{
	short int iSlot;
	short int iChannel;
	int iRet;
}CMDRESPONSE;

//码流发送信息
typedef struct tagStreamControl
{
	short int iSlot;
	BYTE bChannel;	//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;	//码流号：1－4
	int iProtocol;	/* 1=TCP, 2=UDP,3=MUTITYCAST*/
	int iPeerIP;
	int iPeerPort;
}STREAMCONTROL;

//码流发送对端信息
typedef struct tagClientControl
{
	int iProtocol;	/* 1=TCP, 2=UDP,3=MUTITYCAST*/
	int iPeerIP;
	int iPeerPort;
}CLIENTCONTROL;

//码流结构（编码器）
typedef struct tagStreamHead
{
	short int iSlot;
	BYTE bChannel;	//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;	//码流号：1－4
	BYTE bType;		//0: 裸码流(ES,TS,PS)  1: 带媒体传输协议头
	BYTE bReserve[3];
}STREAMHEAD;

//编码器编码参数
typedef struct tagEncoderParam
{
	short int iSlot;
	BYTE bChannel;				//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;				//码流号：1－4
	int iEncodeType;			//编码格式	说明：int类型，MPEG1=1, MPEG2=2, MPEG4=3 ,MJPEG=4 ,H264=5, JPEG2K=6
	unsigned int iFrameRate;	//帧率, unsigned int类型(PAL:0-25,NTSC:0-30)
	unsigned int iCodeRate;		//码率, unsigned int类型,单位为K
	BYTE iEncodePolicy;			//编码策略 说明： 0＝帧率优先，1＝码率优先
	BYTE iEncodeWay;			//编码方式	说明： 0＝按场编，1＝按帧编
	//BYTE bReserve[2];			//保留
	BYTE bStreamFlag;			//码流开关（0=关闭；1=打开 其中码流号为1不可设置，固定为打开）
	BYTE bReserve;				//保留
	unsigned int uiGOP;			//GOP，unsigned int类型
	unsigned int uiGOP_M;		//GOP_M, unsigned int类型,表示GOP的组合方式
	unsigned int uiBasicQP;		//基础QP，unsigned int类型
	unsigned int uiMaxQP;		//最大QP，unsigned int类型
	unsigned int uiCodeRateType;//码率类型	说明：0=CBR, 1=VBR
	unsigned int uiMaxCodeRate;	//最大码率说明：仅在VBR模式下有效，单位K
	unsigned int uiMinCodeRate;	//最小码率	说明：仅在VBR模式下有效，单位K
	short int uiResolutionH;	//分辨率（H）
	short int uiResolutionV;	//分辨率（V）
}ENCODERPARAM;

//编码器OSD参数
typedef struct tagOsdParam
{
	short int iSlot;
	BYTE bChannel;				//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;				//码流号：1－4
	unsigned int uiFontColor;	//OSD字体颜色(0=白色,1=黑色)

	unsigned int uiTimeX;
	unsigned int uiTimeY;
	unsigned int uiWordX;
	unsigned int uiWordY;
	char szWord[64];
	BYTE bShowTime;//是否显示时间 （0-hiden ，1-show）
	BYTE bShowWord;//是否显示文字 （0-hiden ，1-show）
	BYTE bReserve[2];
}OSDPARAM;

//编码器时间配置
typedef struct tagTimeParam
{
	short int iSlot;
	short int iChannel;
	BYTE bYear;
	BYTE bMonth;
	BYTE bDay;
	BYTE bWeek;
	BYTE bHour;
	BYTE bMinute;
	BYTE bSecond;
	BYTE bReserve;
}TIMEPARAM;


//云台控制配置
typedef struct tagPtzParam
{
	short int iSlot;
	short int iChannel;
	BYTE bPtzType;
	BYTE bPresetNo;
	BYTE bSpeed;
}PTZPARAM;


//解码器参数
typedef struct tagDecoderParam
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
	//	BYTE bReserve[2];
}DECODERPARAM;

//工作模式
typedef struct tagWorkModeParam
{
	short int iSlot;		//板卡号：1－16
	short int iChannel;		//通道号：1－8，根据板卡类型而定
	int uiWorkMode;			//工作模式  0=编码器，1=解码器，2=编解码器
}WORKMODEPARAM;


// 视频丢失/遮挡报警信息
typedef struct tagVmux6kVideoAlarm
{
	short int iSlot;		//板卡号：1－16
	short int iChannel;		//通道号：1－4，根据板卡类型而定
	BYTE bAlarmStatus0;		//报警状态，第n 个bit位对于第n路编码通道视频丢失报警，其中 0=不报警，1=报警
	BYTE bAlarmStatus1;		//报警状态，第n 个bit位对于第n路编码通道视频遮挡报警，其中 0=不报警，1=报警
	BYTE bReserve[2];		//Reserved,置0
}VMUX6KVIDEOALARM;	


//设备重启参数
typedef struct tagRebootParam
{
	short int iSlot;
	short int iChannel;
}REBOOTPARAM;


//修改了 获取码流信息,开始发送码流,结束发送码流,三条命令,添加 标准流/裸码流 属性项
//该修改只针对Vbox6671，Vaux6167，Vbox400，Vbox1400型号有效
//码流发送信息
typedef struct tagStreamCtl
{
	short int iSlot;
	BYTE bChannel;	//通道号：1－4，根据板卡类型而定
	BYTE bStreamNo;	//码流号：1－4
	int iHeadFlag;  //码流属性：1 = 标准流,2 = 裸码流
	int iProtocol;	/* 1=TCP, 2=UDP,3=MUTITYCAST*/
	int iPeerIP;
	int iPeerPort;
}STREAMCTL;

//码流发送对端信息
typedef struct tagClientCtl
{
	int iHeadFlag;  //码流属性：1 = 标准流,2 = 裸码流
	int iProtocol;	/* 1=TCP, 2=UDP,3=MUTITYCAST*/
	int iPeerIP;
	int iPeerPort;
}CLIENTCTL;

//////

#pragma  pack()

enum MsgGroup
{
	SYSTEM_PROTOCOL = 0, //系统级协议（catalog = 0）
	CONFIG_PROTOCOL,	//设备配置协议（catalog = 1）
	ENCODER_PROTOCOL,	//编码参数协议（catalog = 2）
	ALARM_PROTOCOL,		//告警协议（catalog = 3）
	STREAM_PROTOCOL,	//媒体流发送控制协议（catalog = 4）
	CAMCONTROL_PROTOCOL	//云台控制命令协议
};

enum NetProtocol
{
	NET_TCP = 1,
	NET_UDP,
	NET_MULTICAST,
	NET_URL,
	NET_CLOSE,
};

#endif



