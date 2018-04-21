#pragma once
#include "PublicDefine.h"

#include "DecoderSetting.h"
#include "EncoderSetting.h"
#include "encoder.h"

typedef struct _ChnConfigParam
{
	char szUrl[1024];
	int nOutResolution;

	
	//VO_OUTPUT_PAL = 0,
	//VO_OUTPUT_NTSC,
	
	// VO_OUTPUT_1080P24,
	// VO_OUTPUT_1080P25,
	// VO_OUTPUT_1080P30,
	
	// VO_OUTPUT_720P50, 
	// VO_OUTPUT_720P60,   
	// VO_OUTPUT_1080I50,
	// VO_OUTPUT_1080I60,	 
	// VO_OUTPUT_1080P50,
	// VO_OUTPUT_1080P60,			 
	
	//VO_OUTPUT_576P50,
	//VO_OUTPUT_480P60,
	
	//VO_OUTPUT_800x600_60, 		   /* VESA 800 x 600 at 60 Hz (non-interlaced) */
	//VO_OUTPUT_1024x768_60,		   /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
	//VO_OUTPUT_1280x1024_60,		   /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
	//VO_OUTPUT_1366x768_60,		   /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
	//VO_OUTPUT_1440x900_60,		   /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
	//VO_OUTPUT_1280x800_60,		   /* 1280*800@60Hz VGA@60Hz*/


	int nWbc;
	int nVoDev;
	//#define VO_INTF_CVBS    (0x01L<<0)
	//#define VO_INTF_YPBPR   (0x01L<<1)
	//#define VO_INTF_VGA     (0x01L<<2)
	//#define VO_INTF_BT656   (0x01L<<3)
	//#define VO_INTF_BT1120  (0x01L<<4)
	//#define VO_INTF_HDMI    (0x01L<<5)
	int nFPS;
}ChnConfigParam;

class CChannelConfig
{
public:
	CChannelConfig(CDecoderSetting *pDecSet, CEncoderSetting* pEncSet);
	~CChannelConfig();

	void LoadConfigFile( char *szConfigFile );
    // 获取显示模式，返回MD_DISPLAY_MODE，
    // MD_DISPLAY_MODE_1MUX: chanenl保存解码通道号
    // MD_DISPLAY_MODE_MESS: channel为0
    // MD_DISPLAY_MODE_4MUX: channel=128
    // MD_DISPLAY_MODE_9MUX: channel=129
    // MD_DISPLAY_MODE_16MUX: channel=130
    int GetDispalyMode(int &mode, int &channel);
	void LoadDecUrl();
	void LoadDisplay();
	
	void LoadSip();

    // 返回0表示设置成功，>0表示无需设置，<0表示设置失败
	int SetFPS( int nChannel, int iFPS );
	
	char *GetPtzDevice( char *szDev );
	ChnConfigParam m_ChnParam[MAXDECCHANN];
	EncSettings m_encsettings;
	AlarmSettings m_alarms;
	char m_szConfigFile[1024];
	int m_nChanneoCount;
	int m_nEncChannelCount;
    int m_iMuxDisplayMode;
    int m_iMuxDisplayModeLast;
private:
	void InitDefault();
	void InitEncoderDefault();
	CDecoderSetting *m_pDecSet;
    CEncoderSetting *m_pEncSet;

	void InitPtzDefault();
    
};



