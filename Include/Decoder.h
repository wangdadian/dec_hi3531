#ifndef _HISI_DECODER_H
#define _HISI_DECODER_H

#include "PublicDefine.h"
#include "sample_comm.h"
#include <pthread.h>
#include "TextOnYUV.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iconv.h>
#include "dwstype.h"

#include "TS2ES.h"
#include "PS2ES.h"
typedef unsigned char       BYTE;

#define SCR_HEIGTH300 300

#define VISS_HEADER_SIZE 40
#define QBOX_HEADER_SIZE 28
#define VISS_HEADER_SIZE 40

#define SAMPLE_MAX_VDEC_CHN_CNT 32

typedef struct sample_vdec_sendparam
{
    pthread_t Pid;
    HI_BOOL bRun;
    VDEC_CHN VdChn;    
    PAYLOAD_TYPE_E enPayload;
	HI_S32 s32MinBufSize;
    VIDEO_MODE_E enVideoMode;
}VDEC_SENDPARAM_S;


typedef struct __hisi_audio_head
{
	BYTE bReserved;
	BYTE bType;
	BYTE bLen;
	BYTE bCounter;
}HISI_AUDIO_HEAD;

class CDecoder
{
public:
	CDecoder( int nVideoOutResolution, 
				 int nDecChannelNo, 
				 int nWbc );
	~CDecoder();
	
	void DecodeData( unsigned char * pData, int nLen );


	void StartDecoder();
	void StartCVBS();
    void ResetVdec();

private: 
	static	void *GetFrameProc(void*	arg);
	void GetFrame();

	int ReplaceQBoxLength(BYTE *pbData, int iDataLength);


	int RemoveQbox(BYTE *pbFrameData, int lFrameSize, int &nRemoveIndex);

	void RemoveVissHead();

	HI_S32 VDEC_CreateVdecChn(HI_S32 s32ChnID, 
				SIZE_S *pstSize, 
				PAYLOAD_TYPE_E enType, 
				VIDEO_MODE_E enVdecMode );
    

	
	int CloneFrame( VIDEO_FRAME_INFO_S oldframe, VIDEO_FRAME_INFO_S &newframe);

	
	void VDEC_WaitDestroyVdecChn(HI_S32 s32ChnID, 
							VIDEO_MODE_E enVdecMode );
	void VDEC_ForceDestroyVdecChn(HI_S32 s32ChnID);


	void ChangeVdec();
	
	CTextOnYuv m_textonyuv;
	
	HI_S32 InitVdec();
	
	HI_S32 UnInitVdec();
	
	HI_S32 InitVInput1();
	HI_S32 UnInitVInput1();

	void SendAudioFrame( unsigned char *pData, int nLen );
	
	//HI_S32 CreateAudioDec();

	int m_nVideoOutResolution;
	int m_nDecChannelNo; // ´Ó0¿ªÊ¼
	int m_nWbc;

	int m_iExitFlag;
	int m_uiFPS;
	HI_U8 *m_pFrameBuf;
	HI_U8 *m_pBuf;
	HI_U8 *m_pAudioBuf;
	HI_U8 *m_pAudioSendData;
	unsigned int m_nCurIndex;
	unsigned int m_nAudioIndex;
	int m_nInited;

	int m_isTS;
	int m_isTSDiscontinue;
	int m_isPS;
	int m_isQbox;
	int m_isES;

	
	pthread_t m_getframeth;
	
	CTS2ES *m_pTs2Es;
	CPS2ES *m_pPs2Es;
	int m_iMpeg2;
	PAYLOAD_TYPE_E m_nPayloadType;
	PAYLOAD_TYPE_E m_enAudPayloadType;

	int m_nLastFormat;
	int m_nFormat;
	unsigned int m_nContinus;
	int m_iDecFlag;
	pthread_mutex_t m_lock;

	int m_nDecBytes;
	time_t m_lastStat;
	BYTE m_nAudioCounter;
	pthread_t m_dateth;


	
    VB_POOL m_hPool;
	VIDEO_FRAME_INFO_S *m_pFrame;
	unsigned int m_iFrameIndex;
  };

#endif



