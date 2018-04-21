
#include "encoder.h"

#include "define.h"
#include "Hi3531Global.h"
#include "StreamTransform.h"
#include "Text2Bmp.h"

CRTSPStreamServer* CHiEncoder::m_pRTSPServer=NULL;

CHiEncoder::CHiEncoder( int iChannelNo ) : CStreamTransform()
{
	m_iChannelNo = iChannelNo;
	memset( &m_param, 0, sizeof( EncoderParam ));

	for ( int i = 0; i < MAXNET_COUNT; i++ )
	{
		memset( &m_netsetting[i], 0, sizeof(EncNetParam ));
	}

	InitializeCriticalSection( &m_lock );
	
	memset( &m_osdsetting, 0, sizeof(m_osdsetting));
	memset( &m_encsetting, 0, sizeof(m_encsetting ));
	m_th = 0;
	m_iExitFlag = 0;
	m_iOsdExitFlag = 0;
	
	memset(m_szUrl, 0, sizeof(m_szUrl));
	sprintf(m_szUrl, "%d_%d", iChannelNo /2 , iChannelNo %2 );

	if ( m_pRTSPServer == NULL )
	{
		m_pRTSPServer = new CRTSPStreamServer( 554, (char*)"Mingding RTSP Stream server.");
	}

	m_pRTSPServer->StartAStream(m_szUrl, 512*1024);
	
	m_pEs2Ps = new CES2PS();
	m_pEs2Ts = new CES2TS();
	m_pEs2Ts->SetStreamType( (int)AVC_VIDEO_STREAM_TYPE, 
		(int)G711_AUDIO_STREAM_TYPE );

	m_iAudioExitFlag = 0;
	m_iTsMux = 0;
	m_iPsMux = 0;
	m_iEncInit = 0;

    // 编解码不启用音频接收
    if(g_nDeviceType != DeviceModel_Dec_Enc)
    {
        if ( iChannelNo % 2 == 0 )
    	{
    	 	pthread_create( &m_audioth, NULL,  GetAudioProc, (void*)this);
    	}
    }
	
	pthread_create( &m_dateth, NULL,  ShowDateProc, (void*)this);
}


HI_S32 CHiEncoder::SetEncSet( EncSetting param )
{
	memcpy( &m_encsetting, &param, sizeof(m_encsetting));
	if( m_iEncInit )
	{
		ApplyOsd();
	}
	return 0;
}

CHiEncoder::~CHiEncoder()
{
	for ( int i = 0; i < MAXNET_COUNT; i++ )
	{
		StopStreamDst(i+1);
	}

	m_iAudioExitFlag = 1;
	m_iExitFlag = 1;
	UninitVenc();

	if ( m_audioth != 0 )
	{
		pthread_join( m_audioth, NULL );
	}
	m_iOsdExitFlag = 1;
	if ( m_dateth != 0 )
	{
		pthread_join( m_dateth, NULL );
	}
	
	PTR_DELETE(m_pRTSPServer);
	PTR_DELETE( m_pEs2Ps );
	PTR_DELETE( m_pEs2Ts );

	DeleteCriticalSection(&m_lock);
}

HI_S32 CHiEncoder::StopNet( int iIndex )
{
	if (iIndex > 2 )
	{
		_DEBUG_( "invalid index should be < 3 " );
		return HI_FAILURE;
	}
	
	EnterCriticalSection(&m_lock);
	StopStreamDst( iIndex+1);
	m_netsetting[iIndex].iEnable = 0;
	LeaveCriticalSection(&m_lock);
	return HI_SUCCESS;
}


HI_S32 CHiEncoder::ApplyOsd()
{
	
	HI_S32 s32Ret = HI_FAILURE;
	RGN_HANDLE RgnHandle;
	for (int i=0; i<MAXOSD_COUNT; i++)
    {
	   RgnHandle = i + m_iChannelNo*MAXOSD_COUNT;
	   s32Ret = UninitRgnNo( RgnHandle, m_iChannelNo );
	   //printf("Detach handle:%d from chn success\n", RgnHandle);
   }

	int iFont = 16;
	if ( m_param.iResolution < (int)ResolutionType_D1 )
	{
		iFont = 16;
	}
	else 
	{
		iFont = 32;
	}
	/*********************************************
     step 2: display overlay regions to venc groups
    *********************************************/
    for (int i=0; i<MAXOSD_COUNT; i++)
    {
        RgnHandle = i + m_iChannelNo*MAXOSD_COUNT;
		if ( m_osdsetting.osd[i].iEnable > 0 &&
			m_osdsetting.osd[i].iType == (int)OsdType_Content )
		{

			
			int iStartX = ( m_osdsetting.osd[i].iX / 4 )*4;
			int iStartY = ( m_osdsetting.osd[i].iY / 4 )*4;
		

			DisplayText( RgnHandle, 
						m_osdsetting.osd[i].szOsd, 
						iFont, 
						iStartX,
						iStartY,
						i );
		}

		if ( m_osdsetting.osd[i].iEnable > 0 &&
			m_osdsetting.osd[i].iType == (int)OsdType_Name )
		{
		
			int iStartX = ( m_osdsetting.osd[i].iX / 4 )*4;
			int iStartY = ( m_osdsetting.osd[i].iY / 4 )*4;

            char * ptrOsdStr = NULL;
            if(strlen(m_encsetting.szName)>0)
            {
                ptrOsdStr = m_encsetting.szName;
            }
            else if( strlen(m_osdsetting.osd[i].szOsd)>0)
            {
                ptrOsdStr = m_osdsetting.osd[i].szOsd;
            }
            else
            {
                //
                ptrOsdStr = NULL;
            }
            if(ptrOsdStr != NULL)
            {
			    DisplayText( RgnHandle, 
						ptrOsdStr, 
						iFont, 
						iStartX,
						iStartY,
						i );
             }
		}
		
		if ( m_osdsetting.osd[i].iEnable > 0  &&
				m_osdsetting.osd[i].iType != (int)OsdType_Content )
		{
			char szTime[32]={0};
			switch( m_osdsetting.osd[i].iType )
			{
				case (int)OsdType_DateAndTime:
					GetDateTimeNow(szTime);
					break;
				case (int)OsdType_Date:
					GetDateNowOnly(szTime);
					break;
				case (int)OsdType_Time:
					GetTimeNowOnly(szTime);
					break;
				default:
					continue;
					break;
			};

			int iStartX = ( m_osdsetting.osd[i].iX / 4 )*4;
			int iStartY = ( m_osdsetting.osd[i].iY / 4 )*4;
		
			DisplayText( RgnHandle, 
						szTime, 
						iFont, 
						iStartX,
						iStartY, 
						i );
		}


		
	}
	s32Ret = 0;
	return s32Ret;
}



HI_S32 CHiEncoder::SetOsd( EncOsdSettings param )
{
	HI_S32 s32Ret = HI_FAILURE;
	memcpy( &m_osdsetting, &param, sizeof( EncOsdSettings ));
	ApplyOsd();

	return s32Ret;
}

void *CHiEncoder::ShowDateProc( void* p)
{
	CHiEncoder *pThis = (CHiEncoder*)p;
	
	HI_S32 s32Ret = HI_SUCCESS;
	while( pThis->m_iOsdExitFlag == 0 )
	{
		for (int i=0; i<MAXOSD_COUNT; i++)
	    {
	        int RgnHandle = i + pThis->m_iChannelNo*MAXOSD_COUNT;

            // 日期/时间
			if ( pThis->m_osdsetting.osd[i].iEnable > 0  &&
				pThis->m_osdsetting.osd[i].iType != (int)OsdType_Content )
			{
				char szTime[32]={0};
				switch( pThis->m_osdsetting.osd[i].iType )
				{
					case (int)OsdType_DateAndTime:
						GetDateTimeNow(szTime);
						break;
					case (int)OsdType_Date:
						GetDateNowOnly(szTime);
						break;
					case (int)OsdType_Time:
						GetTimeNowOnly(szTime);
						break;
					default:
						continue;
						break;
				};

				//_DEBUG_( "update time:%s", szTime );

				BITMAP_S stBitmap;
				stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
				
				CText2Bmp text2bmp;
				int nWidth = 0;;
				int nHeight = 0;
				unsigned char *pBmp=NULL;

				int iFont = 0;
				if ( pThis->m_param.iResolution < (int)ResolutionType_D1 )
				{
					iFont = 16;
				}
				else 
				{
					iFont = 32;
				}
				
				text2bmp.GetBitmap( szTime,  iFont, nWidth, nHeight, pBmp);
				stBitmap.u32Width = nWidth;
				stBitmap.u32Height = nHeight;
				stBitmap.pData = (void*)pBmp;
				
				s32Ret = HI_MPI_RGN_SetBitMap( RgnHandle,&stBitmap);
			    if(s32Ret != HI_SUCCESS)
			    {
			        SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
			    }

				PTR_FREE( pBmp );
				
			}
			
		}
		usleep(900*1000);
	}
	
	//pthread_detach( pthread_self());
	return NULL;
}

HI_S32 CHiEncoder::DisplayText( int RgnHandle, 
								 char *szText, 
								 int iFont, 
								 int iStartX, 
								 int iStartY, 
								 int iLayer)
{

	iStartX = ( iStartX / 4 )*4;
	iStartY = ( iStartY / 4 )*4;

	_DEBUG_( "text: %s font:%d x:%d y:%d ", szText, iFont, iStartX, iStartY );
	
	HI_S32 s32Ret = HI_SUCCESS;
	BITMAP_S stBitmap;

	VENC_GRP VencGrp;
	VENC_GRP VencGrpStart= 0;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	RGN_ATTR_S stRgnAttr;
	
	stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;

	CText2Bmp text2bmp;
	int nWidth = 0;;
	int nHeight = 0;
	unsigned char *pBmp=NULL;

	text2bmp.GetBitmap( szText, iFont, nWidth, nHeight, pBmp);
	stBitmap.u32Width = nWidth;
	stBitmap.u32Height = nHeight;
	stBitmap.pData = (void*)pBmp;

	VencGrp = m_iChannelNo+VencGrpStart;
	stChn.enModId = HI_ID_GROUP;
	stChn.s32DevId = VencGrp;
	stChn.s32ChnId = 0;
	
	memset(&stChnAttr,0,sizeof(stChnAttr));

	stChnAttr.bShow = HI_TRUE;
	stChnAttr.enType = OVERLAY_RGN;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = iStartX;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = iStartY;
	stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
	stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = iLayer;

	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
	stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

	s32Ret = InitRgnNo(  RgnHandle, nWidth, nHeight );
	s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
	if(HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d) failed with %#x!\n",\
			   RgnHandle, s32Ret);
		//return HI_FAILURE;
	}
	else
	{
		_DEBUG_( "rgn %d attach to chn %d success.", RgnHandle, VencGrp );
	}

	s32Ret = HI_MPI_RGN_SetBitMap( RgnHandle,&stBitmap);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
		PTR_FREE( pBmp );
        return HI_FAILURE;
    }

	PTR_FREE( pBmp );
	return s32Ret;
}

HI_S32 CHiEncoder::StartNet( EncNetParam param )
{
	_DEBUG_("start net enable:%d index:%d net:%d mux:%d addr:%s port:%d", 
		param.iEnable, 
		param.iIndex, 
		param.iNetType, 
		param.iMux, 
		param.szPlayAddress, 
		param.iPlayPort );

	if (param.iIndex > 2 )
	{
		_DEBUG_( "invalid index should be < 3 " );
		return HI_FAILURE;
	}

	int iIndex = param.iIndex;
	if ( m_netsetting[iIndex].iEnable == param.iEnable &&
		m_netsetting[iIndex].iMux == param.iMux &&
		m_netsetting[iIndex].iNetType == param.iNetType &&
		m_netsetting[iIndex].iPlayPort == param.iPlayPort &&
		strcmp(m_netsetting[iIndex].szPlayAddress, param.szPlayAddress ) == 0 )
	{
		//return HI_SUCCESS;
	}
	
	EnterCriticalSection(&m_lock);
	_DEBUG_( "stop stream dst: %d", param.iIndex+1);
	StopStreamDst(param.iIndex+1);
	LeaveCriticalSection(&m_lock);

	int nDstSendType = 0;
	int nRtpFlag = 0;
	switch ( param.iNetType )
	{
		case 0: //udp
		{
			nDstSendType = (int)StreamCastType_Unicast;
			break;
		}
		case 1: //tcp 
		{
			nDstSendType = (int)StreamCastType_TcpServer;
			break;
		}
		case 2: //rtsp
		{
			memcpy( &m_netsetting[param.iIndex], &param, sizeof(param));
			m_netsetting[param.iIndex].iEnable = 1;
			return HI_SUCCESS;
		}
		case 3: //rtp
		{
			nDstSendType = (int)StreamCastType_Unicast;
			nRtpFlag = 1;
			break;
		}
		default:
		{
			return HI_FALSE;
		}
	};

	memcpy( &m_netsetting[param.iIndex], &param, sizeof(param));
	//m_netsetting[param.iIndex].iEnable = 1;
	m_iTsMux = 0;
	m_iPsMux = 0;

	for ( int i = 0; i < MAXNET_COUNT; i++ )
	{
		if ( m_netsetting[i].iEnable == 0 )
        {
            continue;
		}
		
		if ( m_netsetting[i].iMux == MuxType_TS )
		{
			m_iTsMux = 1;
		}
		if ( m_netsetting[i].iMux == MuxType_PS )
		{
			m_iPsMux = 1;
		}
	}
	
	EnterCriticalSection(&m_lock);
	if ( param.iEnable > 0 )
	{
		_DEBUG_("start stream dst: %d", param.iIndex+1);
		SetStreamDst( param.iIndex+1, 
			nDstSendType, 
			param.szPlayAddress, 
			param.iPlayPort, 
			nRtpFlag );
	}
	LeaveCriticalSection(&m_lock);

	_DEBUG_("start net ok");
	return HI_SUCCESS;
}

HI_S32 CHiEncoder::StartVenc(EncoderParam param, bool bForceStart)
{
	_DEBUG_("start venc profile:%d bitrate:%dkps fps:%d gop:%d resolution:%d.", 
		param.iProfile, 
		param.iBitRate, 
		param.iFPS, 
		param.iGop, 
		param.iResolution );
    if(param.iFPS > 25 || param.iFPS <= 0)
    {
        param.iFPS = 25;
    }

    if(bForceStart != true)
    {
    	if( m_param.iProfile == param.iProfile && 
        	m_param.iBitRate == param.iBitRate && 
        	m_param.iFPS == param.iFPS && 
        	m_param.iResolution == param.iResolution && 
        	m_param.iCbr == param.iCbr && 
        	m_param.iGop == param.iGop && 
        	m_param.iIQP == param.iIQP && 
        	m_param.iPQP == param.iPQP && 
        	m_param.iMinQP == param.iMinQP && 
        	m_param.iMaxQP == param.iMaxQP && 
        	m_param.iAudioEnable == param.iAudioEnable && 
        	m_param.iAudioFormat == param.iAudioFormat && 
        	m_param.iAudioRate == param.iAudioRate && 
        	m_param.iBits == param.iBits
    	)
    	{
            _DEBUG_("Same parameters, no need to start again, chn=%d.", m_iChannelNo);
    		return HI_SUCCESS;
    	}
    }

	memcpy( &m_param, &param, sizeof( EncoderParam ));
    
	if ( m_iEncInit == 0 )
	{
		UninitVenc();
		InitVenc();
		m_iEncInit = 1;
		
	}
	else
	{
		UninitVenc();
		InitVenc();
//		ApplyEncParam();
//		ApplyOsd();
	}
    _DEBUG_("Start VENC OK, chn=%d!", m_iChannelNo);
	return HI_SUCCESS;
}


HI_S32 CHiEncoder::ApplyEncParam()
{
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stVencChnAttr;
	VENC_ATTR_H264_S stH264Attr;
	VENC_ATTR_H264_CBR_S	stH264Cbr;
	VENC_ATTR_H264_VBR_S	stH264Vbr;
	VENC_ATTR_H264_FIXQP_S	stH264FixQp;
	VENC_ATTR_MJPEG_S stMjpegAttr;
	VENC_ATTR_MJPEG_FIXQP_S stMjpegeFixQp;
	VENC_ATTR_JPEG_S stJpegAttr;
	SIZE_S stPicSize;
	VIDEO_NORM_E	enNorm = (VIDEO_NORM_E)VIDEO_ENCODING_MODE_PAL;
	SAMPLE_RC_E enRcMode = (SAMPLE_RC_E)m_param.iCbr;
	int nHiSize = 0;
	GetHiResolution( m_param.iResolution,   nHiSize);


	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enNorm, (PIC_SIZE_E)nHiSize, &stPicSize);
	 if (HI_SUCCESS != s32Ret)
	{
		_DEBUG_("Get picture size failed!\n");
		return HI_FAILURE;
	}
	
	/******************************************
	 step 2:  Create Venc Channel
	******************************************/
	stVencChnAttr.stVeAttr.enType = PT_H264;
	switch(stVencChnAttr.stVeAttr.enType)
	{
		case PT_H264:
		{
			stH264Attr.u32MaxPicWidth = stPicSize.u32Width;
			stH264Attr.u32MaxPicHeight = stPicSize.u32Height;
			stH264Attr.u32PicWidth = stPicSize.u32Width;/*the picture width*/
			stH264Attr.u32PicHeight = stPicSize.u32Height;/*the picture height*/
			stH264Attr.u32BufSize  = stPicSize.u32Width * stPicSize.u32Height * 2;/*stream buffer size*/
			stH264Attr.u32Profile  = m_param.iProfile;/*0: baseline; 1:MP; 2:HP   ? */
			stH264Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
			stH264Attr.bField = HI_FALSE;  /* surpport frame code only for hi3516, bfield = HI_FALSE */
			stH264Attr.bMainStream = HI_TRUE; /* surpport main stream only for hi3516, bMainStream = HI_TRUE */
			stH264Attr.u32Priority = 0; /*channels precedence level. invalidate for hi3516*/
			stH264Attr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame. Invalidate for hi3516*/
			memcpy(&stVencChnAttr.stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

			if(SAMPLE_RC_CBR == enRcMode)
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
				stH264Cbr.u32Gop			=  m_param.iGop;
				stH264Cbr.u32StatTime		= 5; /* stream rate statics time(s) */
				stH264Cbr.u32ViFrmRate		=  25;/* input (vi) frame rate */
				stH264Cbr.fr32TargetFrmRate = m_param.iFPS;/* target frame rate */
				stH264Cbr.u32BitRate = m_param.iBitRate;
				stH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
				memcpy(&stVencChnAttr.stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
			}
			else if (SAMPLE_RC_FIXQP == enRcMode) 
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
				stH264FixQp.u32Gop = m_param.iGop;
				stH264FixQp.u32ViFrmRate =	25;
				stH264FixQp.fr32TargetFrmRate = m_param.iFPS;
				stH264FixQp.u32IQp = m_param.iIQP;
				stH264FixQp.u32PQp = m_param.iPQP;
				memcpy(&stVencChnAttr.stRcAttr.stAttrH264FixQp, &stH264FixQp,sizeof(VENC_ATTR_H264_FIXQP_S));
			}
			else if (SAMPLE_RC_VBR == enRcMode) 
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
				stH264Vbr.u32Gop = m_param.iGop;
				stH264Vbr.u32StatTime = 1;
				stH264Vbr.u32ViFrmRate =  25;
				stH264Vbr.fr32TargetFrmRate =  m_param.iFPS;
				stH264Vbr.u32MinQp = m_param.iMinQP;
				stH264Vbr.u32MaxQp = m_param.iMaxQP;
				stH264Vbr.u32MaxBitRate = m_param.iBitRate;
				memcpy(&stVencChnAttr.stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
			}
			else
			{
				return HI_FAILURE;
			}
		}
		break;
		
		case PT_MJPEG:
		{
			stMjpegAttr.u32MaxPicWidth = stPicSize.u32Width;
			stMjpegAttr.u32MaxPicHeight = stPicSize.u32Height;
			stMjpegAttr.u32PicWidth = stPicSize.u32Width;
			stMjpegAttr.u32PicHeight = stPicSize.u32Height;
			stMjpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
			stMjpegAttr.bByFrame = HI_TRUE;  /*get stream mode is field mode  or frame mode*/
			stMjpegAttr.bMainStream = HI_TRUE;	/*main stream or minor stream types?*/
			stMjpegAttr.bVIField = HI_FALSE;  /*the sign of the VI picture is field or frame?*/
			stMjpegAttr.u32Priority = 0;/*channels precedence level*/
			memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));

			if(SAMPLE_RC_FIXQP == enRcMode)
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
				stMjpegeFixQp.u32Qfactor		= 90;
				stMjpegeFixQp.u32ViFrmRate		= (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stMjpegeFixQp.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				memcpy(&stVencChnAttr.stRcAttr.stAttrMjpegeFixQp, &stMjpegeFixQp,
					   sizeof(VENC_ATTR_MJPEG_FIXQP_S));
			}
			else if (SAMPLE_RC_CBR == enRcMode)
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
				stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32StatTime		 = 1;
				stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32ViFrmRate 	 = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stVencChnAttr.stRcAttr.stAttrMjpegeCbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32FluctuateLevel = 0;
				switch (nHiSize)
				{
				  case PIC_QCIF:
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 384*3; /* average bit rate */
					   break;
				  case PIC_QVGA:	/* 320 * 240 */
				  case PIC_CIF:
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 768*3;
					   break;
				  case PIC_D1:
				  case PIC_VGA:    /* 640 * 480 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*3*3;
					   break;
				  case PIC_HD720:	/* 1280 * 720 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*5*3;
					   break;
				  case PIC_HD1080:	/* 1920 * 1080 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*10*3;
					   break;
				  default :
					   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*7*3;
					   break;
				}
			}
			else if (SAMPLE_RC_VBR == enRcMode) 
			{
				stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGVBR;
				stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32StatTime = 1;
				stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == enNorm)?25:30;
				stVencChnAttr.stRcAttr.stAttrMjpegeVbr.fr32TargetFrmRate = 5;
				stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MinQfactor = 50;
				stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxQfactor = 95;
				switch (nHiSize)
				{
				  case PIC_QCIF:
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate= 256*3; /* average bit rate */
					   break;
				  case PIC_QVGA:	/* 320 * 240 */
				  case PIC_CIF:
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 512*3;
					   break;
				  case PIC_D1:
				  case PIC_VGA:    /* 640 * 480 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*2*3;
					   break;
				  case PIC_HD720:	/* 1280 * 720 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*3*3;
					   break;
				  case PIC_HD1080:	/* 1920 * 1080 */
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*6*3;
					   break;
				  default :
					   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*4*3;
					   break;
				}
			}
			else 
			{
				_DEBUG_("cann't support other mode in this version!\n");
				return HI_FAILURE;
			}
		}
		break;
			
		case PT_JPEG:
			stJpegAttr.u32PicWidth	= stPicSize.u32Width;
			stJpegAttr.u32PicHeight = stPicSize.u32Height;
			stJpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
			stJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
			stJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
			stJpegAttr.u32Priority = 0;/*channels precedence level*/
			memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));
			break;
		default:
			return HI_ERR_VENC_NOT_SUPPORT;
	}

	/******************************************
     step 1:  Stop Recv Pictures
    ******************************************/
    s32Ret = HI_MPI_VENC_StopRecvPic((VENC_CHN)m_iChannelNo);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n",\
               m_iChannelNo, s32Ret);
        return HI_FAILURE;
    }

	 /******************************************
     step 2:  UnRegist Venc Channel
    ******************************************/
    s32Ret = HI_MPI_VENC_UnRegisterChn((VENC_CHN)m_iChannelNo);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_UnRegisterChn vechn[%d] failed with %#x!\n",\
               (VENC_CHN)m_iChannelNo, s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 3:  Distroy Venc Channel
    ******************************************/
    s32Ret = HI_MPI_VENC_DestroyChn((VENC_CHN)m_iChannelNo);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n",\
               (VENC_CHN)m_iChannelNo, s32Ret);
        return HI_FAILURE;
    }

	s32Ret = HI_MPI_VENC_CreateChn((VENC_CHN)m_iChannelNo , &stVencChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		_DEBUG_("HI_MPI_VENC_SetChnAttr [%d] faild with %#x!\n",\
				m_iChannelNo, s32Ret);
		return s32Ret;
	}

	
	/******************************************
	 step 3:  Regist Venc Channel to VencGrp
	******************************************/
	s32Ret = HI_MPI_VENC_RegisterChn((VENC_CHN)m_iChannelNo, (VENC_CHN)m_iChannelNo);
	if (HI_SUCCESS != s32Ret)
	{
		_DEBUG_("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
		return HI_FAILURE;
	}

	/******************************************
	 step 4:  Start Recv Venc Pictures
	******************************************/
	s32Ret = HI_MPI_VENC_StartRecvPic((VENC_CHN)m_iChannelNo);
	if (HI_SUCCESS != s32Ret)
	{
		_DEBUG_("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;


}


HI_S32 CHiEncoder::InitVenc()
{
	HI_S32 s32Ret = HI_SUCCESS;

	int nHiSize = 0;
	GetHiResolution( m_param.iResolution,   nHiSize);
    s32Ret = COMM_VENC_Start( (VENC_GRP)m_iChannelNo, 
							(VENC_CHN)m_iChannelNo, 
							PT_H264,
                            (VIDEO_NORM_E)VIDEO_ENCODING_MODE_PAL,  
                            (PIC_SIZE_E)nHiSize,
                            (SAMPLE_RC_E)m_param.iCbr );
	
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("Start Venc failed!\n");
        return HI_FAILURE;
    } 
	//_DEBUG_("venc bind vpss:%d %d", m_iChannelNo, m_iChannelNo /2 );
    if(g_nDeviceType== DeviceModel_Encoder || g_nDeviceType== DeviceModel_HDEncoder)
    {
    	if ( m_iChannelNo % 2 == 0 )
    	{
    		
        	s32Ret = SAMPLE_COMM_VENC_BindVpss(m_iChannelNo, m_iChannelNo/2, VPSS_BSTR_CHN);
    	}
    	else
    	{
    		s32Ret = SAMPLE_COMM_VENC_BindVpss(m_iChannelNo, m_iChannelNo/2, VPSS_PRE0_CHN);
    	}
    }
    else if(g_nDeviceType== DeviceModel_Dec_Enc )
    {
        if ( m_iChannelNo % 2 == 0 )
    	{
    		
        	s32Ret = SAMPLE_COMM_VENC_BindVpss(m_iChannelNo, m_iChannelNo/2, 0);
    	}
    	else
    	{
            // 不支持子码流
            s32Ret = HI_SUCCESS;
    		//s32Ret = SAMPLE_COMM_VENC_BindVpss(m_iChannelNo, m_iChannelNo/2, 1);
    	}
    }
    else
    {
        _DEBUG_("not encoder!");
    }
	
	if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("Start Venc failed!\n");
        assert(false);
        return HI_FAILURE;
    }

	_DEBUG_("start get stream" );
	COMM_VENC_StartGetStream( );
	m_iEncInit = 1;
	return s32Ret;
}

HI_S32 CHiEncoder::UninitVenc()
{
	m_iExitFlag = 1;
	if ( m_th != 0 )
	{
		//pthread_cancel( m_th);
		pthread_join( m_th, NULL);
        m_th = 0;
	}
	
	HI_S32 s32Ret =  COMM_VENC_Stop( (VENC_GRP) m_iChannelNo, (VENC_CHN)m_iChannelNo );

    if(g_nDeviceType== DeviceModel_Encoder || g_nDeviceType== DeviceModel_HDEncoder)
    {
    	if ( m_iChannelNo % 2 == 0 )
    	{
        	s32Ret = SAMPLE_COMM_VENC_UnBindVpss(m_iChannelNo, m_iChannelNo/2, VPSS_BSTR_CHN);
    	}
    	else
    	{
    		s32Ret = SAMPLE_COMM_VENC_UnBindVpss(m_iChannelNo, m_iChannelNo/2, VPSS_PRE0_CHN);
    	}
    }
    else if(g_nDeviceType== DeviceModel_Dec_Enc)
    {
        if ( m_iChannelNo % 2 == 0 )
    	{
    		
        	s32Ret = SAMPLE_COMM_VENC_UnBindVpss(m_iChannelNo, m_iChannelNo/2, 0);
    	}
    	else
    	{
            s32Ret = HI_SUCCESS;
    		//s32Ret = SAMPLE_COMM_VENC_UnBindVpss(m_iChannelNo, m_iChannelNo/2, 1);
    	}
    }
	
	return s32Ret;
}

HI_S32 CHiEncoder::GetHiResolution( int iResolution, int &nHiSize )
{
	nHiSize = 0;
	switch( iResolution )
	{
		case (int)ResolutionType_QCIF:
			nHiSize = PIC_QCIF;
			break;
		case (int)ResolutionType_CIF:
			nHiSize = PIC_CIF;
			break;
		case (int)ResolutionType_D1:
			nHiSize = PIC_D1;
			break;
		case (int)ResolutionType_720P:
			nHiSize = PIC_HD720;
			break;	
		case (int)ResolutionType_1080P:
			nHiSize = PIC_HD1080;
			break;
		default:
			return HI_FAILURE;
			break;
	}
	
	return HI_SUCCESS;
}

HI_S32 CHiEncoder::GetHiCBR( int iCbr, int &nHiCBR )
{
	nHiCBR = VENC_RC_MODE_H264CBR;

	switch( iCbr )
	{
		case (int)0:
		    nHiCBR  = VENC_RC_MODE_H264CBR;
			break;
		case (int)1:
			nHiCBR = VENC_RC_MODE_H264VBR;
			break;
		default:
			return HI_FAILURE;
			break;
	}
	return HI_SUCCESS;
}

void CHiEncoder::OnAVReceived( bool isVideo, unsigned char *pBuf, int nLen )
{
	/*if ( isVideo )
	{
	    _DEBUG_("received av chn:%d len:%d video:%d\n", m_iChannelNo, nLen, isVideo);
	}*/
	
	//EnterCriticalSection(&m_lock);
	for ( int i = 0; i < (int)MAXNET_COUNT; i++ )
	{
		if ( m_netsetting[i].iEnable > 0 && 
			m_netsetting[i].iMux == (int) MuxType_ES &&
			isVideo )
		{
			if (  m_netsetting[i].iNetType !=  (int)NetType_RTSP)
			{
				SendStream( m_netsetting[i].iIndex+1,(char*) pBuf, nLen );
			}
			else
			{
				m_pRTSPServer->InputData( m_szUrl, pBuf, nLen );
			}
		}
	}

	//return;
	if ( m_iPsMux > 0 )
	{
	    unsigned char *pOutPSData = NULL;
	    int iOutPSDataLen = 0;
		m_pEs2Ps->ES2PS( pBuf, nLen, pOutPSData, iOutPSDataLen, isVideo, 0 );
		if ( iOutPSDataLen > 0 )
		{
			for ( int i = 0; i < MAXNET_COUNT; i++ )
			{
				if ( m_netsetting[i].iEnable > 0 && 
					m_netsetting[i].iMux == (int) MuxType_PS  )
				{
					//SendStream( m_netsetting[i].iIndex, (char*) pOutPSData, iOutPSDataLen );
					if (  m_netsetting[i].iNetType !=  (int)NetType_RTSP /*rtsp*/)
					{
						SendStream( m_netsetting[i].iIndex+1,(char*) pOutPSData, iOutPSDataLen );
					}
					else
					{
						m_pRTSPServer->InputData( m_szUrl,  pOutPSData, iOutPSDataLen );
						break;
					}
				}
			}
		}
	}

	if ( m_iTsMux > 0 )
	{
		unsigned char *pOutTSData = NULL;
	    int iOutTSDataLen = 0;
		if ( isVideo )
			m_pEs2Ts->InputESData(pBuf, nLen, isVideo, true, 0, 0 );
		else
		{
			//printf("0x%x 0x%x 0x%x 0x%x\n", pBuf[0], pBuf[1], pBuf[2], pBuf[3] );
			m_pEs2Ts->InputESData(pBuf+4, nLen-4, isVideo, true, 0, 0 );
		}
		while ( m_pEs2Ts->GetTSData(pOutTSData, iOutTSDataLen) ==  0 )
		{
			if ( iOutTSDataLen > 0 )
			{
				for ( int i = 0; i < MAXNET_COUNT; i++ )
				{
					if ( m_netsetting[i].iEnable > 0 && 
						m_netsetting[i].iMux == (int) MuxType_TS  )
					{
						//SendStream( m_netsetting[i].iIndex, (char*) pOutTSData, iOutTSDataLen );
						if (  m_netsetting[i].iNetType !=  (int)NetType_RTSP /*rtsp*/)
						{
							SendStream( m_netsetting[i].iIndex+1,(char*) pOutTSData, iOutTSDataLen );
						}
						else
						{
							m_pRTSPServer->InputData( m_szUrl,  pOutTSData, iOutTSDataLen );
							break;
						}
					}
				}
			}
		}
	}
	//LeaveCriticalSection(&m_lock);
	return;

}


HI_S32 CHiEncoder::CheckValid()
{
	int nHiSize = 0;
	if( HI_FAILURE == GetHiResolution( m_param.iResolution,  nHiSize ) )
	{
		_DEBUG_( "not supported resolution:%d\n", m_param.iResolution );
		return HI_FAILURE;
	}

	if ( m_param.iBitRate >= (10 * 1024) ) 
	{
		_DEBUG_("not supported bit rate :%d should < 10240 ", m_param.iBitRate );
		return HI_FAILURE;
	}
	
	if ( m_param.iBitRate == 0 ) 
	{
		_DEBUG_("not supported bit rate : %d ", m_param.iBitRate );
		return HI_FAILURE;
	}

	if ( m_param.iFPS > 30 || m_param.iFPS == 0 ) 
	{
		_DEBUG_("invalid fps :%d should <= 30 and != 0 ", m_param.iFPS );
		return HI_FAILURE;
	}


	if ( m_param.iGop > 200 || m_param.iGop== 0 ) 
	{
		_DEBUG_("invalid fps :%d should <= 200 and != 0 ", m_param.iGop );
		return HI_FAILURE;
	}

	int nHiCBR = 0;
	if( HI_FAILURE == GetHiCBR( m_param.iCbr,  nHiCBR ) )
	{
		_DEBUG_( "not supported resolution:%d\n", m_param.iCbr );
		return HI_FAILURE;
	}

	return HI_SUCCESS;

}


/******************************************************************************
* funciton : Start venc stream mode (h264, mjpeg)
* note      : rate control parameter need adjust, according your case.
******************************************************************************/
HI_S32 CHiEncoder::COMM_VENC_Start(VENC_GRP VencGrp,  
							VENC_CHN VencChn, 
							PAYLOAD_TYPE_E enType, 
							VIDEO_NORM_E enNorm, 
							PIC_SIZE_E enSize, 
							SAMPLE_RC_E enRcMode )
{
    HI_S32 s32Ret;
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_H264_S stH264Attr;
    VENC_ATTR_H264_CBR_S    stH264Cbr;
    VENC_ATTR_H264_VBR_S    stH264Vbr;
    VENC_ATTR_H264_FIXQP_S  stH264FixQp;
    VENC_ATTR_MJPEG_S stMjpegAttr;
    VENC_ATTR_MJPEG_FIXQP_S stMjpegeFixQp;
    VENC_ATTR_JPEG_S stJpegAttr;
    SIZE_S stPicSize;


	_DEBUG_("start get pic size, chn=%d", m_iChannelNo);
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enNorm, enSize, &stPicSize);
     if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("Get picture size failed!\n");
        return HI_FAILURE;
    }
    /******************************************
     step 1: Greate Venc Group
    ******************************************/
    _DEBUG_("start create group");
    s32Ret = HI_MPI_VENC_CreateGroup(VencGrp);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n",\
                 VencGrp, s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 2:  Create Venc Channel
    ******************************************/
    stVencChnAttr.stVeAttr.enType = enType;
    switch(enType)
    {
        case PT_H264:
        {
            stH264Attr.u32MaxPicWidth = stPicSize.u32Width;
            stH264Attr.u32MaxPicHeight = stPicSize.u32Height;
            stH264Attr.u32PicWidth = stPicSize.u32Width;/*the picture width*/
            stH264Attr.u32PicHeight = stPicSize.u32Height;/*the picture height*/
            stH264Attr.u32BufSize  = stPicSize.u32Width * stPicSize.u32Height * 2;/*stream buffer size*/
            stH264Attr.u32Profile  = m_param.iProfile;/*0: baseline; 1:MP; 2:HP   ? */
            stH264Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
            stH264Attr.bField = HI_FALSE;  /* surpport frame code only for hi3516, bfield = HI_FALSE */
            stH264Attr.bMainStream = HI_TRUE; /* surpport main stream only for hi3516, bMainStream = HI_TRUE */
            stH264Attr.u32Priority = 0; /*channels precedence level. invalidate for hi3516*/
            stH264Attr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame. Invalidate for hi3516*/
            memcpy(&stVencChnAttr.stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

            if(SAMPLE_RC_CBR == enRcMode)
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                stH264Cbr.u32Gop            =  m_param.iGop;
                stH264Cbr.u32StatTime       = 5; /* stream rate statics time(s) */
                stH264Cbr.u32ViFrmRate      =  25;/* input (vi) frame rate */
                stH264Cbr.fr32TargetFrmRate = m_param.iFPS;/* target frame rate */
				stH264Cbr.u32BitRate = m_param.iBitRate;
                stH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
                memcpy(&stVencChnAttr.stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
            }
            else if (SAMPLE_RC_FIXQP == enRcMode) 
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
                stH264FixQp.u32Gop = m_param.iGop;
                stH264FixQp.u32ViFrmRate =  25;
                stH264FixQp.fr32TargetFrmRate = m_param.iFPS;
                stH264FixQp.u32IQp = m_param.iIQP;
                stH264FixQp.u32PQp = m_param.iPQP;
                memcpy(&stVencChnAttr.stRcAttr.stAttrH264FixQp, &stH264FixQp,sizeof(VENC_ATTR_H264_FIXQP_S));
            }
            else if (SAMPLE_RC_VBR == enRcMode) 
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                stH264Vbr.u32Gop = m_param.iGop;
                stH264Vbr.u32StatTime = 1;
                stH264Vbr.u32ViFrmRate =  25;
                stH264Vbr.fr32TargetFrmRate =  m_param.iFPS;
                stH264Vbr.u32MinQp = m_param.iMinQP;
                stH264Vbr.u32MaxQp = m_param.iMaxQP;
				stH264Vbr.u32MaxBitRate = m_param.iBitRate;
				memcpy(&stVencChnAttr.stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
            }
            else
            {
                return HI_FAILURE;
            }
        }
        break;
        
        case PT_MJPEG:
        {
            stMjpegAttr.u32MaxPicWidth = stPicSize.u32Width;
            stMjpegAttr.u32MaxPicHeight = stPicSize.u32Height;
            stMjpegAttr.u32PicWidth = stPicSize.u32Width;
            stMjpegAttr.u32PicHeight = stPicSize.u32Height;
            stMjpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
            stMjpegAttr.bByFrame = HI_TRUE;  /*get stream mode is field mode  or frame mode*/
            stMjpegAttr.bMainStream = HI_TRUE;  /*main stream or minor stream types?*/
            stMjpegAttr.bVIField = HI_FALSE;  /*the sign of the VI picture is field or frame?*/
            stMjpegAttr.u32Priority = 0;/*channels precedence level*/
            memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));

            if(SAMPLE_RC_FIXQP == enRcMode)
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
                stMjpegeFixQp.u32Qfactor        = 90;
                stMjpegeFixQp.u32ViFrmRate      = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
                stMjpegeFixQp.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
                memcpy(&stVencChnAttr.stRcAttr.stAttrMjpegeFixQp, &stMjpegeFixQp,
                       sizeof(VENC_ATTR_MJPEG_FIXQP_S));
            }
            else if (SAMPLE_RC_CBR == enRcMode)
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
                stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32StatTime       = 1;
                stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32ViFrmRate      = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
                stVencChnAttr.stRcAttr.stAttrMjpegeCbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
                stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32FluctuateLevel = 0;
                switch (enSize)
                {
                  case PIC_QCIF:
                	   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 384*3; /* average bit rate */
                	   break;
                  case PIC_QVGA:    /* 320 * 240 */
                  case PIC_CIF:
                	   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 768*3;
                       break;
                  case PIC_D1:
                  case PIC_VGA:	   /* 640 * 480 */
                	   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*3*3;
                       break;
                  case PIC_HD720:   /* 1280 * 720 */
                	   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*5*3;
                	   break;
                  case PIC_HD1080:  /* 1920 * 1080 */
                  	   stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*10*3;
                	   break;
                  default :
                       stVencChnAttr.stRcAttr.stAttrMjpegeCbr.u32BitRate = 1024*7*3;
                       break;
                }
            }
            else if (SAMPLE_RC_VBR == enRcMode) 
            {
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGVBR;
                stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32StatTime = 1;
                stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == enNorm)?25:30;
                stVencChnAttr.stRcAttr.stAttrMjpegeVbr.fr32TargetFrmRate = 5;
                stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MinQfactor = 50;
                stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxQfactor = 95;
                switch (enSize)
                {
                  case PIC_QCIF:
                	   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate= 256*3; /* average bit rate */
                	   break;
                  case PIC_QVGA:    /* 320 * 240 */
                  case PIC_CIF:
                	   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 512*3;
                       break;
                  case PIC_D1:
                  case PIC_VGA:	   /* 640 * 480 */
                	   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*2*3;
                       break;
                  case PIC_HD720:   /* 1280 * 720 */
                	   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*3*3;
                	   break;
                  case PIC_HD1080:  /* 1920 * 1080 */
                  	   stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*6*3;
                	   break;
                  default :
                       stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = 1024*4*3;
                       break;
                }
            }
            else 
            {
                _DEBUG_("cann't support other mode in this version!\n");
                return HI_FAILURE;
            }
        }
        break;
            
        case PT_JPEG:
            stJpegAttr.u32PicWidth  = stPicSize.u32Width;
            stJpegAttr.u32PicHeight = stPicSize.u32Height;
            stJpegAttr.u32BufSize = stPicSize.u32Width * stPicSize.u32Height * 2;
            stJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
            stJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
            stJpegAttr.u32Priority = 0;/*channels precedence level*/
            memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));
            break;
        default:
            return HI_ERR_VENC_NOT_SUPPORT;
    }

	_DEBUG_("start create chn:%d", VencChn );
    s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n",\
                VencChn, s32Ret);
        return s32Ret;
    }


    /******************************************
     step 3:  Regist Venc Channel to VencGrp
    ******************************************/
	_DEBUG_("register grp:%d chn:%d", VencGrp, VencChn );
    s32Ret = HI_MPI_VENC_RegisterChn(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 4:  Start Recv Venc Pictures
    ******************************************/
    _DEBUG_("start recv pic:%d", VencChn );
    s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;

}

/******************************************************************************
* funciton : Stop venc ( stream mode -- H264, MJPEG )
******************************************************************************/
HI_S32  CHiEncoder::COMM_VENC_Stop(VENC_GRP VencGrp,VENC_CHN VencChn)
{
    HI_S32 s32Ret;

    /******************************************
     step 1:  Stop Recv Pictures
    ******************************************/
    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n",\
               VencChn, s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 2:  UnRegist Venc Channel
    ******************************************/
    s32Ret = HI_MPI_VENC_UnRegisterChn(VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_UnRegisterChn vechn[%d] failed with %#x!\n",\
               VencChn, s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 3:  Distroy Venc Channel
    ******************************************/
    s32Ret = HI_MPI_VENC_DestroyChn(VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n",\
               VencChn, s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 4:  Distroy Venc Group
    ******************************************/
    s32Ret = HI_MPI_VENC_DestroyGroup(VencGrp);
    if (HI_SUCCESS != s32Ret)
    {
        _DEBUG_("HI_MPI_VENC_DestroyGroup group[%d] failed with %#x!\n",\
               VencGrp, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

void * CHiEncoder::GetAudioProc( void* p)
{
	CHiEncoder *pThis = (CHiEncoder*)p;

	HI_S32 s32Ret;
    HI_S32 AencFd;
	
    AUDIO_STREAM_S stFrame; 
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;
	int nAeChn =  pThis->m_iChannelNo / 2;

    s32Ret = HI_MPI_AI_GetChnParam( SAMPLE_AUDIO_AI_DEV, nAeChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    
    stAiChnPara.u32UsrFrmDepth = 30;
    
    s32Ret = HI_MPI_AI_SetChnParam(SAMPLE_AUDIO_AI_DEV, nAeChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }
    
    FD_ZERO(&read_fds);
    AencFd = HI_MPI_AENC_GetFd(nAeChn);
    FD_SET(AencFd,&read_fds);

    while (pThis->m_iAudioExitFlag == 0)
    {     
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AencFd,&read_fds);
        
        s32Ret = select(AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) 
        {
        	printf("select failed\n");
			usleep( 1 * 1000 );
            break;
        }
        else if (0 == s32Ret) 
        {
            //printf("%s: get ai frame select time out\n", __FUNCTION__);
			continue;
			//break;
        }        
        
        if (FD_ISSET(AencFd, &read_fds))
        {
            s32Ret = HI_MPI_AENC_GetStream(  nAeChn,
                &stFrame, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, 0, nAeChn, s32Ret);
                break;
            }
			else
			{
				pThis->OnAVReceived( false, stFrame.pStream, stFrame.u32Len );
			}
            HI_MPI_AENC_ReleaseStream( nAeChn, &stFrame);
        }
    }

	//pthread_detach( pthread_self());
	return NULL;

}


/******************************************************************************
* funciton : get stream from each channels and save them
******************************************************************************/
HI_VOID* CHiEncoder::COMM_VENC_GetVencStreamProc(HI_VOID *p)
{

    VENC_CHN_ATTR_S stVencChnAttr;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd ;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoadType;
    int nTotal = 0;
    CHiEncoder *pThis  = (CHiEncoder*)p;

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/

    /* decide the stream file name, and open file to save stream */
    VencChn = pThis->m_iChannelNo;
    s32Ret = HI_MPI_VENC_GetChnAttr( VencChn, &stVencChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        _DEBUG_("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
               VencChn, s32Ret);
        return NULL;
    }
    enPayLoadType = stVencChnAttr.stVeAttr.enType;

    /* Set Venc Fd. */
    VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (VencFd < 0)
    {
        _DEBUG_("HI_MPI_VENC_GetFd failed with %#x!\n", 
               VencFd);
        return NULL;
    }

	unsigned char szBuf[512*1024] = {0};
	
    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while ( pThis->m_iExitFlag == 0 )
    {
        FD_ZERO(&read_fds);
        FD_SET(VencFd, &read_fds);

        TimeoutVal.tv_sec  = 0;
        TimeoutVal.tv_usec = 20000;
        s32Ret = select(VencFd+1 , &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            //SAMPLE_PRT("get venc stream time out, continue\n");
            continue;
        }
        else
        {
            if (FD_ISSET(VencFd, &read_fds))
            {
                /*******************************************************
                 step 2.1 : query how many packs in one-frame stream.
                *******************************************************/
                memset(&stStream, 0, sizeof(stStream));
                s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", VencChn, s32Ret);
                    break;
                }

                /*******************************************************
                 step 2.2 : malloc corresponding number of pack nodes.
                *******************************************************/
                stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    SAMPLE_PRT("malloc stream pack failed!\n");
                    break;
                }
                
                /*******************************************************
                 step 2.3 : call mpi to get one-frame stream
                *******************************************************/
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret)
                {
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %x!\n", s32Ret);
                    break;
                }

				nTotal = 0;
				for ( unsigned int i = 0; i < stStream.u32PackCount; i++)
    			{
					memcpy( szBuf+nTotal, stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0] );
					nTotal += stStream.pstPack[i].u32Len[0];

					if ( stStream.pstPack[i].u32Len[1] > 0 )
					{
						memcpy( szBuf+nTotal, stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1] );
						nTotal += stStream.pstPack[i].u32Len[1];
					}
					
					//_DEBUG_("get fream size:%d %d\n", stStream.pstPack[i].u32Len[0], stStream.pstPack[i].u32Len[1] );
				}

				pThis->OnAVReceived(true,szBuf,nTotal);
				
 				//printf( "0x%x 0x%x 0x%x 0x%x 0x%x\n", szBuf[0], szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
			
                /*******************************************************
                 step 2.5 : release stream
                *******************************************************/
                s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
                if (HI_SUCCESS != s32Ret)
                {
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    break;
                }
                /*******************************************************
                 step 2.6 : free pack nodes
                *******************************************************/
                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }
	//pthread_detach( pthread_self());
	_DEBUG_("get stream exited");
    return NULL;
}

/******************************************************************************
* funciton : start get venc stream process thread
******************************************************************************/
HI_S32 CHiEncoder::COMM_VENC_StartGetStream( )
{
	m_iExitFlag = 0;
    return pthread_create(&m_th, 0,  COMM_VENC_GetVencStreamProc, (HI_VOID*)this);
}





