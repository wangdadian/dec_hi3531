#pragma once

#include "PublicDefine.h"
#include "sample_comm.h"

#define SAMPLE_AUDIO_PTNUMPERFRM   320
#define SAMPLE_AUDIO_AI_DEV 1
#define SAMPLE_AUDIO_AO_DEV 4
#define AUDIO_CHANNEL_CNT 16


extern char* GetModuleName();

extern char* GetBuildDate();

extern char* GetVersion();


extern HI_S32 InitHi3531();
extern void UnInitHi3531();


extern HI_S32 SetDispalyFPS( int vodev, int iFPS );


extern HI_S32 InitDisplay( int nChannelNo, 
							int nDisplayMode, 
							int nResolution , 
							int nFPS,
							int &VoDev);
extern HI_S32 UnInitMuxDisplay( int nChannelNo, int nDisplayMode, int nVoDev );

extern HI_S32 InitMuxDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev);
extern HI_S32 UnInit9MuxDisplay( int nChannelNo, int nDisplayMode, int nVoDev );

extern HI_S32 Init9MuxDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev);

extern HI_S32 InitRgnNo( int RgnHandle, int nWidth , int nHeight );


extern HI_S32 UninitRgnNo( int RgnHandle, int nChannelNo );


extern HI_S32 InitVI();
	
extern void SetDeviceType( int nType );


extern HI_S32 UninitVI();

extern HI_S32 UnInitDisplay( int nChannelNo, int nDisplayMode, int nVoDev );

extern HI_S32 UninitAdec();

extern HI_S32 InitAdec();

