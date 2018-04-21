#ifndef HIENCODER_H_
#define HIENCODER_H_
#include "StreamTransform.h"

#include "PublicDefine.h"
#include "sample_comm.h"
#include "NetUdp.h"
#include <pthread.h>
#include "RTSPStreamServer.h"
#include "ES2PS.h"
#include "ES2TS.h"


class CHiEncoder : public CStreamTransform
{
public:
	CHiEncoder(int iChannelNo );
	~CHiEncoder();

	HI_S32 StartVenc( EncoderParam param );

	HI_S32 StartNet( EncNetParam param );
	
	HI_S32 StopNet( int iIndex );

	
	HI_S32 SetOsd( EncOsdSettings param );

	HI_S32 SetEncSet( EncSetting param );

	
	HI_S32 ApplyOsd();

private:
	int m_iChannelNo;
	EncoderParam m_param;
	
	HI_S32 GetHiResolution( int iResolution, int &nHiSize );

	
	HI_S32 GetHiCBR( int iCbr, int &nHiCBR );
	
	HI_S32 CheckValid();
	
	HI_S32 InitVenc();
	
	HI_S32 UninitVenc();

	
	HI_S32 ApplyEncParam();

	
	HI_S32 DisplayText(  int RgnHandle, 
						  char *szText, 
								 int iFont, 
								 int iStartX, 
								 int iStartY, 
								 int iLayer );
	
	static HI_VOID* COMM_VENC_GetVencStreamProc(HI_VOID *p);


	static void *GetAudioProc( void*p);

	
	static void * ShowDateProc( void*p);
	
	HI_S32 COMM_VENC_StartGetStream( );


	HI_S32 COMM_VENC_Start(VENC_GRP VencGrp,  
								VENC_CHN VencChn, 
								PAYLOAD_TYPE_E enType, 
								VIDEO_NORM_E enNorm, 
								PIC_SIZE_E enSize, 
								SAMPLE_RC_E enRcMode );

	HI_S32	COMM_VENC_Stop(VENC_GRP VencGrp,VENC_CHN VencChn);

	void OnAVReceived( bool isVideo, unsigned char *pBuf, int nLen );

	int m_iExitFlag;
	int m_iAudioExitFlag;
	int m_iOsdExitFlag;
	static CRTSPStreamServer *m_pRTSPServer;
	
	pthread_t m_th;
	pthread_t m_audioth;
	pthread_t m_dateth;
	char m_szUrl[1024];
	CES2PS *m_pEs2Ps;
	CES2TS *m_pEs2Ts;
	EncNetParam m_netsetting[MAXNET_COUNT];
	EncOsdSettings m_osdsetting;
	EncSetting m_encsetting;
	CRITICAL_SECTION m_lock;
	int m_iTsMux;
	int m_iPsMux;
	int m_iEncInit;
};
#endif


