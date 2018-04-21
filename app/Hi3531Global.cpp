#include "Hi3531Global.h"


#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "hifb.h"
#include <pthread.h>


#ifndef MODULENAME
	#define MODULENAME "undefined module name!"
#endif

#ifndef BUILDDATE
	#define BUILDDATE "undefined build date!"
#endif

#ifndef VERSION
	#define VERSION "undefined module version!"
#endif


int g_nDeviceType = 0;
int g_nMaxEncChn = 0; // 从0开始，0-通道1主码流，1-通道1子码流，以此类推
int g_nMaxViChn = 0;

char g_szSipServerIp[256]={"0.0.0.0"};
char g_szSipServerId[256]={"1010101099000001"};
char g_szLocalIp[50]={"127.0.0.1"};
char g_szSipDeviceId[16][256];


void SetDeviceType( int nType )
{
	g_nDeviceType = 0;
	switch( nType )
	{
		case DeviceModel_Decoder:
			g_nDeviceType = nType;
			break;
		case DeviceModel_Encoder:
			g_nDeviceType = nType;
			g_nMaxEncChn = 16;
			g_nMaxViChn = 8;
			break;
		case DeviceModel_HDEncoder:
			g_nDeviceType = nType;
			g_nMaxEncChn = 8;
			g_nMaxViChn = 4;
			break;
        case DeviceModel_Dec_Enc:
			g_nDeviceType = nType;
			g_nMaxEncChn = 8;
			g_nMaxViChn = 4;
			break;
		default:
			break;
	
	};

}


char* GetModuleName()
{
	return (char*)MODULENAME;
}

char* GetBuildDate()
{
	return (char*)BUILDDATE;
}

char* GetVersion()
{
	return (char*)VERSION;
}

#define DBG_MEM_RW_SIZE 	5
#define    BASE_TL_PRIVATE_286x	        200	/* 192-255 are private */
#define    	TL_2865_RD 					_IOWR('H', BASE_TL_PRIVATE_286x + 7, unsigned int)
#define    	TL_2865_WR 					_IOWR('H', BASE_TL_PRIVATE_286x + 8, unsigned int)

int tw286x_rd(int chip_id, unsigned char adr, unsigned char *out_val)
{
	unsigned int buf[DBG_MEM_RW_SIZE];
	
	buf[1] = chip_id;
	buf[2] = adr;

	int fd_tw286x = open("/dev/tw_286x", O_RDWR);
	if(ioctl(fd_tw286x, TL_2865_RD, buf) < 0)
    {
        close( fd_tw286x);
		printf("common.c@tw286x_rd: TL_2865_RD error\n");
		return -1;
	}
	*out_val = buf[0];
	close( fd_tw286x);
	return 0;
}

int tw286x_wr(int chip_id, unsigned char adr, unsigned char data)
{
	unsigned int buf[DBG_MEM_RW_SIZE];
	buf[1] = chip_id;
	buf[2] = adr;
	buf[3] = data;
    
	int fd_tw286x = open("/dev/tw_286x", O_RDWR);
	if(ioctl(fd_tw286x, TL_2865_WR, buf) < 0)
    {
        close( fd_tw286x);
		printf("common.c@tw286x_wr: TL_2865_RD error\n");
		return -1;
	}
	//printf("chip id:%d, adr:%x, data:%x\n\n", chip_id, adr, data);
	close( fd_tw286x);
	return 0;
}


#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)


HI_S32 InitAudio()
{
    HI_S32 s32Ret= HI_SUCCESS;
    AIO_ATTR_S stAioAttr;
    AUDIO_RESAMPLE_ATTR_S stAiReSampleAttr;
    AUDIO_RESAMPLE_ATTR_S stAoReSampleAttr;
    AIO_ATTR_S stHdmiAoAttr;
	PAYLOAD_TYPE_E enPayloadType = PT_G711A;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
	
	tw286x_wr(0, 0x40, 0);//csp modify
	tw286x_wr(0, 0xE0, 0x10);
    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ClkSel = 1;
	stAioAttr.u32ChnCnt = AUDIO_CHANNEL_CNT;

  //resample 
	//stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_32000;
    //stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM * 4;

    /* ai 32k -> 8k */
    stAiReSampleAttr.u32InPointNum = SAMPLE_AUDIO_PTNUMPERFRM * 4;
    stAiReSampleAttr.enInSampleRate = AUDIO_SAMPLE_RATE_32000;
    stAiReSampleAttr.enReSampleType = AUDIO_RESAMPLE_4X1;

    /* ao 8k -> 32k */
    stAoReSampleAttr.u32InPointNum = SAMPLE_AUDIO_PTNUMPERFRM;
    stAoReSampleAttr.enInSampleRate = AUDIO_SAMPLE_RATE_8000;
    stAoReSampleAttr.enReSampleType = AUDIO_RESAMPLE_1X4;


    /********************************************
	  step 1: config audio codec
	********************************************/

	/*s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr, HI_FALSE);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
	*/

	/********************************************
	  step 2: start Ai
	********************************************/
	s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, stAioAttr.u32ChnCnt, &stAioAttr, HI_FALSE, NULL);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	/********************************************
	  step 3: start Aenc
	********************************************/
	s32Ret = SAMPLE_COMM_AUDIO_StartAenc(stAioAttr.u32ChnCnt, enPayloadType);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	/********************************************
	  step 4: Aenc bind Ai Chn
	********************************************/
	
	for (unsigned int i=0; i<stAioAttr.u32ChnCnt; i++)
	{
		int AeChn = i;
		int AiChn = i;
		s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return s32Ret;
		}
		_DEBUG_("Ai(%d,%d) bind to AencChn:%d ok!",AiDev , AiChn, AeChn);
	}

	AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
	
	s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	
    stAioAttr.u32ChnCnt = 2;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, AoChn, &stAioAttr, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	
	return s32Ret;
}


HI_S32 InitAdec()
{
	
   HI_S32 s32Ret= HI_SUCCESS;
   AIO_ATTR_S stAioAttr;
   PAYLOAD_TYPE_E enPayloadType = PT_G711A;
   
   tw286x_wr(0, 0x40, 0);//csp modify
   tw286x_wr(0, 0xE0, 0x10);
   stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
   stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
   stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
   stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
   stAioAttr.u32EXFlag = 1;
   stAioAttr.u32FrmNum = 30;
   stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
   stAioAttr.u32ClkSel = 1;
   stAioAttr.u32ChnCnt = AUDIO_CHANNEL_CNT;

	AUDIO_DEV	AoDev = SAMPLE_AUDIO_AO_DEV;
	AO_CHN		AoChn = 0;
	ADEC_CHN	AdChn = 0;

	s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, enPayloadType);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	stAioAttr.u32ChnCnt = 2;
	s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, AoChn, &stAioAttr, NULL);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}
    return HI_SUCCESS;
}

HI_S32 UninitAdec()
{
	AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
	
	SAMPLE_COMM_AUDIO_StopAo(AoDev, AoChn, HI_FALSE);
    SAMPLE_COMM_AUDIO_StopAdec(AdChn);
    SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
    return HI_SUCCESS;
}

HI_S32 UninitAudio()
{
	UninitAdec();
	
    SAMPLE_COMM_AUDIO_StopAenc(AUDIO_CHANNEL_CNT);
    SAMPLE_COMM_AUDIO_StopAi(SAMPLE_AUDIO_AI_DEV, AUDIO_CHANNEL_CNT, HI_FALSE, HI_FALSE);


	for ( int i = 0; i < AUDIO_CHANNEL_CNT; i++)
	{
		SAMPLE_COMM_AUDIO_AencUnbindAi( SAMPLE_AUDIO_AI_DEV, i, i );
	}
	return HI_SUCCESS;
}



void* HideAllFB( void* param)
{
	char szDev[16]={0};
	for ( int i = 1; i < 3; i++ )
	{
		sprintf( szDev, "/dev/fb%d", i );		
		int fd = open(szDev, O_RDWR);
		if ( fd < 0 )
		{
			_DEBUG_("error: open %s error.\n", szDev );
			perror("at: ");
			continue;
		}

	    //int bShow = 0;
		//ioctl( fd, FBIOPUT_SHOW_HIFB, (void*)&bShow );

		struct fb_var_screeninfo var;
		if ( ioctl ( fd, FBIOGET_VSCREENINFO, &var) < 0 )
		{
			_DEBUG_( "get screen info error\n");
			close( fd);
			continue;
		}
		
		unsigned char* fb_mem;
		fb_mem = (unsigned char*) mmap (NULL, var.xres*var.yres, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		if (fb_mem == NULL)
		{
			_DEBUG_( "mmap error:\n" );
            close( fd);
			continue;
		}
		memset( fb_mem, 0, var.xres*var.yres );
		close(fd);
	}
	//pthread_detach(pthread_self());
	return NULL;
}


HI_S32 SET_RGN_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnRGN;

	/*the max chn of vpss,grp and venc is 64*/
    for(i=0; i<16; i++)
    {
        stMppChnRGN.enModId  = HI_ID_RGN;
        stMppChnRGN.s32DevId = 0;
        stMppChnRGN.s32ChnId = 0;

        if(0 == (i%2))
        {
            pcMmzName = NULL;  
        }
        else
        {
            pcMmzName = (HI_CHAR*)"ddr1";
        }

        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnRGN,pcMmzName);
        if (s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }
    
    return HI_SUCCESS;
}

HI_S32 UninitRgn()
{
	/*********************************************
	step 11: Detach osd from chn
   *********************************************/	
   int RgnHandle=0;
	VENC_GRP VencGrp;
	VENC_GRP VencGrpStart= 0;
	MPP_CHN_S stChn;
	HI_S32 s32Ret = HI_FAILURE;
	
   for (int i=0; i<MAXOSD_COUNT; i++)
   {
	   RgnHandle = i;
	   for (int j=0; j<g_nMaxEncChn; j++)
	   {
		   VencGrp = j+VencGrpStart;
		   stChn.enModId = HI_ID_GROUP;
		   stChn.s32DevId = 0;
		   stChn.s32ChnId = VencGrp;

		   s32Ret = HI_MPI_RGN_DetachFrmChn(RgnHandle, &stChn);
		   if(HI_SUCCESS != s32Ret)
		   {
			   SAMPLE_PRT("HI_MPI_RGN_DetachFrmChn (%d) failed with %#x!\n",\
					  RgnHandle, s32Ret);
			   //return HI_FAILURE;
		   }
	   }

		printf("Detach handle:%d from chn success\n", RgnHandle);

   }

   /*********************************************
	step 12: destory region
   *********************************************/
   for (int i=0; i<MAXOSD_COUNT; i++)
   {
	   RgnHandle = i;
	   s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
	   if (HI_SUCCESS != s32Ret)
	   {
		   SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n",\
				   RgnHandle, s32Ret);
	   }
   }
   SAMPLE_PRT("destory all region success!\n");

   return s32Ret;


}

HI_S32 InitRgn()
{
	RGN_ATTR_S stRgnAttr;
	RGN_HANDLE RgnHandle;
	VENC_GRP VencGrp;
	VENC_GRP VencGrpStart= 0;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	HI_S32 s32Ret = HI_FAILURE;

	/****************************************
     step 1: create overlay regions
    ****************************************/
    for (int i=0; i<MAXOSD_COUNT*g_nMaxEncChn; i++)
    {
        stRgnAttr.enType = OVERLAY_RGN;
        stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stRgnAttr.unAttr.stOverlay.stSize.u32Width  = 144;
        stRgnAttr.unAttr.stOverlay.stSize.u32Height = 32;
        stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00*(i%2) + ((i+1)%2)*0x1f;

        RgnHandle = i;

        s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", \
                   RgnHandle, s32Ret);
            HI_MPI_RGN_Destroy(RgnHandle);
            return HI_FAILURE;
        }
        //gs_s32RgnCntCur ++;
        SAMPLE_PRT("the handle:%d,creat success!\n",RgnHandle);
    }
    
    //usleep(SAMPLE_RGN_SLEEP_TIME*5);
    printf("display region to chn success!\n");

	return s32Ret;
}



HI_S32 UninitRgnNo( int RgnHandle, int nChannelNo )
{
	/*********************************************
	step 11: Detach osd from chn
   *********************************************/	
	VENC_GRP VencGrp;
	VENC_GRP VencGrpStart= 0;
	MPP_CHN_S stChn;
	HI_S32 s32Ret = HI_FAILURE;

   VencGrp = nChannelNo;
   stChn.enModId = HI_ID_GROUP;
   stChn.s32DevId = 0;
   stChn.s32ChnId = VencGrp;

   s32Ret = HI_MPI_RGN_DetachFrmChn(RgnHandle, &stChn);
   if(HI_SUCCESS != s32Ret)
   {
	   SAMPLE_PRT("HI_MPI_RGN_DetachFrmChn (%d) failed with %#x!\n",\
			  RgnHandle, s32Ret);
	   //return HI_FAILURE;
   }

	printf("Detach handle:%d from chn success\n", RgnHandle);

   /*********************************************
	step 12: destory region
   *********************************************/

   s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
   if (HI_SUCCESS != s32Ret)
   {
	   SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n",\
			   RgnHandle, s32Ret);
   }

   return s32Ret;
}

HI_S32 InitRgnNo( int RgnHandle, int nWidth , int nHeight )
{
	RGN_ATTR_S stRgnAttr;
	VENC_GRP VencGrp;
	VENC_GRP VencGrpStart= 0;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	HI_S32 s32Ret = HI_FAILURE;

	/****************************************
     step 1: create overlay regions
    ****************************************/

    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width  = nWidth;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = nHeight;
    stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00*(1%2) + ((1+1)%2)*0x1f;

    s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", \
               RgnHandle, s32Ret);
        HI_MPI_RGN_Destroy(RgnHandle);
        return HI_FAILURE;
    }
    //gs_s32RgnCntCur ++;
    SAMPLE_PRT("the handle:%d,creat success!\n",RgnHandle);
   
    //usleep(SAMPLE_RGN_SLEEP_TIME*5);
    printf("display region to chn success!\n");
	return s32Ret;

}

HI_S32 InitHi3531()
{
	//pthread_t th;
	//pthread_create(&th,0,HideAllFB, NULL);

	SAMPLE_VI_MODE_E enViMode;
    HI_U32 u32ViChnCnt;
    HI_S32 s32VpssGrpCnt;

	if ( g_nDeviceType == (int)DeviceModel_HDEncoder ||
        g_nDeviceType == (int)DeviceModel_Dec_Enc)
	{
		enViMode = SAMPLE_VI_MODE_4_1080P;
	    u32ViChnCnt = 4;
	    s32VpssGrpCnt = 8;
	}
	else
	{
		enViMode = SAMPLE_VI_MODE_16_D1;
	    u32ViChnCnt = 8;
	    s32VpssGrpCnt = 16;
	}	
    
    VB_CONF_S stVbConf;
    VPSS_GRP_ATTR_S stGrpAttr;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;

    /******************************************
     step  1: init global  variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

	PIC_SIZE_E szPic = PIC_D1;
	if ( g_nDeviceType == (int)DeviceModel_HDEncoder ||
        g_nDeviceType == (int)DeviceModel_Dec_Enc )
	{
		szPic = PIC_HD1080;
	}
	else
	{
		szPic = PIC_D1;
	}

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize( VIDEO_ENCODING_MODE_PAL, 
				szPic,
                SAMPLE_PIXEL_FORMAT, 
                SAMPLE_SYS_ALIGN_WIDTH );
	
    stVbConf.u32MaxPoolCnt = 64;

    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    /*ddr1 video buffer*/
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 6;
    strcpy(stVbConf.astCommPool[1].acMmzName,"ddr1");


    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        //return -1;
    }

    s32Ret = SAMPLE_COMM_VPSS_MemConfig();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }

	s32Ret = SAMPLE_COMM_VENC_MemConfig();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
        //return HI_FAILURE;
    }

	 s32Ret = SAMPLE_COMM_VDEC_MemConfig();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
        //return HI_FAILURE;
    }
	
    s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DSD0, NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }
    s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DHD1, (char*)"ddr1");
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }

	s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DHD0, NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }

    if ( g_nDeviceType == (int)DeviceModel_HDEncoder ||
        g_nDeviceType == (int)DeviceModel_Encoder )
	{
    	s32Ret = SAMPLE_COMM_VI_MemConfig(enViMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_MemConfig failed with %d!\n", s32Ret);
            //return -1;
        }
    }

	s32Ret = SAMPLE_COMM_VENC_MemConfig();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }

	s32Ret = SET_RGN_MemConfig();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_MemConfig failed with %d!\n", s32Ret);
        //return -1;
    }


/******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
		
    s32Ret = SAMPLE_COMM_VI_Start( enViMode, VIDEO_ENCODING_MODE_PAL );
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
       // return -1;
    }
      
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize( VIDEO_ENCODING_MODE_PAL, PIC_WUXGA, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        //return -1;
    }
    
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
       // return -1;
    }

    if ( g_nDeviceType == (int)DeviceModel_HDEncoder ||
         g_nDeviceType == (int)DeviceModel_Encoder )
    {
	    s32Ret = InitAudio();
    }
	return s32Ret;
}



HI_S32 InitVI()
{

	HI_S32 s32Ret = 0;
	if ( g_nDeviceType == (int)DeviceModel_HDEncoder )
	{
	
		s32Ret = SAMPLE_COMM_VI_BindVpss(SAMPLE_VI_MODE_4_1080P);
	}
	else if(g_nDeviceType == (int)DeviceModel_Dec_Enc)
	{
		return 0;
	}
    else
    {
        s32Ret = SAMPLE_COMM_VI_BindVpss(SAMPLE_VI_MODE_16_D1);
    }
	if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        return -1;
    }

	//InitRgn();
	return 0;
}




HI_S32 UninitVI()
{
	
	SAMPLE_COMM_VI_UnBindVpss(SAMPLE_VI_MODE_1_D1);
	return 0;
}

void UnInitHi3531()
{

	UninitAudio();

	SAMPLE_COMM_VPSS_Stop(4, VPSS_MAX_CHN_NUM);
	/******************************************
	 step 10: exit mpp system
	******************************************/
	
	SAMPLE_COMM_SYS_Exit();

}
HI_S32 SetDispalyFPS( int vodev, int iFPS )
{

	HI_S32 s32Ret = 0;

	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    s32Ret = HI_MPI_VO_GetVideoLayerAttr(vodev, &stLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("get display fps failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
	
	SAMPLE_COMM_VO_StopDevLayer(vodev);
	
	HI_MPI_VO_Enable(vodev);

	stLayerAttr.u32DispFrmRt = iFPS;
	s32Ret = HI_MPI_VO_SetVideoLayerAttr(vodev, &stLayerAttr);

	
	HI_MPI_VO_EnableVideoLayer( vodev );
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("set display fps failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

	return 0;
}


HI_S32 InitDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev)
{
    VoDev = 0;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr; 
    VO_PUB_ATTR_S stVoPubAttrSD; 
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
	HI_S32 s32Ret=0;
	HI_S32 u32WndNum = 0;
	
    VO_WBC_ATTR_S stWbcAttr;
	printf("init display %d\n", nDisplayMode );
	if ( ( nDisplayMode & (int)VO_INTF_VGA ) != 0 || ( nDisplayMode & (int)VO_INTF_HDMI ) != 0 )
	{
		/*
		
		HI_S32 s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DHD1, "ddr1");
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
	        return -1;
	    }
		*/
		/******************************************
	     step 5: start vo HD1 (VGA+HDMI) (DoubleFrame) (WBC source) 
	    ******************************************/
	    printf("start vo hd1: HDMI + VGA, wbc source\n");

		if ( ( nDisplayMode & (int)VO_INTF_CVBS) != 0 )
		{
	    	VoDev = SAMPLE_VO_DEV_DHD1;
		}
		else 
		{
			VoDev = SAMPLE_VO_DEV_DHD0;
		}
		
		u32WndNum = 16;
	    enVoMode = VO_MODE_1MUX;
	    
	    stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E)nResolution;
		stVoPubAttr.enIntfType = 0;

		if ( nDisplayMode & (int)VO_INTF_VGA )
		{
			stVoPubAttr.enIntfType |= VO_INTF_VGA;
			
		}
		if ( nDisplayMode & (int)VO_INTF_HDMI )
		{
			stVoPubAttr.enIntfType |= VO_INTF_HDMI;
		}
		
	    stVoPubAttr.u32BgColor = 0x000000ff;//0x00ffffff
	    stVoPubAttr.bDoubleFrame = HI_TRUE;  // enable doubleframe

		

		HI_U32 u32DispBufLen = 5;
		s32Ret = HI_MPI_VO_SetDispBufLen(VoDev, u32DispBufLen);
		if (s32Ret != HI_SUCCESS)
		{
		    printf("Set display buf len failed with error code %#x!\n", s32Ret);
		
		}

	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	        //return -1;
	    }
	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	        //return -1;
	    }

		 /* if it's displayed on HDMI, we should start HDMI */
	    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
	    {
	        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
	        {
	            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
	            //return -1;
	        }
	    }

	    /******************************************
	     step 9: HD1 switch mode 
	    ******************************************/
	    enPreVoMode = enVoMode;
	    u32WndNum = 1;
	    enVoMode = VO_MODE_1MUX;

	    s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }
		
	    s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }
		
	    s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

        // nChannelNo从1开始
		int VpssGrp = nChannelNo-1;
	    VoChn = 0;
	    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
	        //return -1;
	    }

	}

	if ( ( (nDisplayMode & VO_INTF_CVBS) != 0 ) && 
		( ( (nDisplayMode & VO_INTF_VGA) != 0 ) || ( (nDisplayMode & VO_INTF_HDMI) != 0 ) ) )
	{

		/******************************************
		   step 6: start vo SD0 (CVBS) (WBC target) 
		  ******************************************/
		/*
		s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DSD0, NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
	        return -1;
	    }
	  */
		printf("start vo SD0: wbc from hd1\n");
	    u32WndNum = 1;
	    enVoMode = VO_MODE_1MUX;

	    stVoPubAttrSD.enIntfSync = VO_OUTPUT_PAL;
	    stVoPubAttrSD.enIntfType = VO_INTF_CVBS;
	    stVoPubAttrSD.u32BgColor = 0x000000ff;
	    stVoPubAttrSD.bDoubleFrame = HI_TRUE;
		
	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	       // return -1;
	    }

		s32Ret = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0,  enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StopChn failed!\n");
	        //return -1;
	    }
	    
	    s32Ret = SAMPLE_COMM_VO_StartChn(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 7: start HD1 WBC 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_GetWH(VO_OUTPUT_PAL, \
	                      &stWbcAttr.stTargetSize.u32Width, \
	                      &stWbcAttr.stTargetSize.u32Height, \
	                      &stWbcAttr.u32FrameRate);

		stWbcAttr.u32FrameRate = 30;
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
	       // return -1;
	    }
	    stWbcAttr.enPixelFormat = SAMPLE_PIXEL_FORMAT;
        //////////////////
        //stWbcAttr.stTargetSize.u32Height=stWbcAttr.stTargetSize.u32Height*2;
	    /////////////////
	    s32Ret = HI_MPI_VO_SetWbcAttr(SAMPLE_VO_DEV_DHD1, &stWbcAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	      //  return -1;
	    }
	    
	    s32Ret = HI_MPI_VO_EnableWbc(SAMPLE_VO_DEV_DHD1);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 8: Bind HD1 WBC to SD0 Chn0 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_BindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0, 0);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	}

	
	//HideAllFB(NULL);
	return s32Ret;
}


HI_S32 UnInitDisplay( int nChannelNo, 
					  int nDisplayMode, 
					  int nVoDev )
{
	printf("check point 10 %d %d\n", nDisplayMode, nVoDev );
	
	int VoChn = 0;
	int VpssGrp = nChannelNo-1;
	VoChn = nChannelNo-1;
	HI_S32 sRet=0;
	if ( (nDisplayMode & ((int)VO_INTF_HDMI)) != 0 )
	{
		SAMPLE_COMM_VO_HdmiStop();
	}

	sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "unbind vo wbc error");
	}
	
	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "disable wbc error");
	}

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
	////////////////////////////////////////////
	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 0,0,VPSS_PRE1_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 1,1,VPSS_PRE1_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}
    ////////////////////////////////////////////
	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_1MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop channel error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_1MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
    /*
	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}
	*/
    ////////////////////////////////////////////////////////

    sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 0,0,VPSS_PRE1_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
    /*
	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 1,1,VPSS_PRE1_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}
	*/
    /////////////////////////////////////////////////////////

	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD0, VO_MODE_1MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	
	return 0;
    
	if ( ( (nDisplayMode & (int)VO_INTF_CVBS) != 0 ) && 
		( ( (nDisplayMode & (int)VO_INTF_VGA) != 0 ) || 
		( (nDisplayMode & (int)VO_INTF_HDMI) != 0 ) ) )
	{

		
		
		sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "unbind vo wbc error");
		}

		
    	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "disable wbc error");
		}

		sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_1MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop channel error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}

		sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_1MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
	}
	
	if ((nDisplayMode & (int)VO_INTF_VGA) != 0 || 
		(nDisplayMode & (int)VO_INTF_HDMI) != 0 )
	{

		sRet = SAMPLE_COMM_VO_UnBindVpss(nVoDev, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}
		
		sRet = SAMPLE_COMM_VO_StopChn(nVoDev, VO_MODE_1MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}

		sRet = SAMPLE_COMM_VO_StopDevLayer(nVoDev);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
	}

	return 0;
}


HI_S32 InitMuxDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev)
{
    //return Init9MuxDisplay(nChannelNo,nDisplayMode,nResolution,nFPS,VoDev);
    VoDev = 0;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr; 
    VO_PUB_ATTR_S stVoPubAttrSD; 
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
	HI_S32 s32Ret=0;
	HI_S32 u32WndNum = 0;
	
    VO_WBC_ATTR_S stWbcAttr;
	printf("init display %d\n", nDisplayMode );
	if ( ( nDisplayMode & (int)VO_INTF_VGA ) != 0 || ( nDisplayMode & (int)VO_INTF_HDMI ) != 0 )
	{
		/******************************************
	     step 5: start vo HD1 (VGA+HDMI) (DoubleFrame) (WBC source) 
	    ******************************************/
	    printf("start vo hd1: HDMI + VGA, wbc source\n");

		//if ( ( nDisplayMode & (int)VO_INTF_CVBS) != 0 )
		//{
	    VoDev = SAMPLE_VO_DEV_DHD1;
		
		u32WndNum = 4;
	    enVoMode = VO_MODE_4MUX;
	    
	    stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E)nResolution;
		stVoPubAttr.enIntfType = 0;

		stVoPubAttr.enIntfType = (VO_INTF_HDMI | VO_INTF_VGA);
		
	    stVoPubAttr.u32BgColor = 0x000000ff;
	    stVoPubAttr.bDoubleFrame = HI_FALSE;  // enable doubleframe

	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	        //return -1;
	    }
	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	        //return -1;
	    }

		 /* if it's displayed on HDMI, we should start HDMI */
	    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
	    {
	        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
	        {
	            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
	            //return -1;
	        }
	    }

	    /******************************************
	     step 9: HD1 switch mode 
	    ******************************************/
	    enPreVoMode = enVoMode;
	    u32WndNum = 4;
	    enVoMode = VO_MODE_4MUX;

	    s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }

	    s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }
		
	    s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

		int VpssGrp = nChannelNo-1;
	    VoChn = nChannelNo-1;
		VoDev = SAMPLE_VO_DEV_DHD1;
	    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
	        //return -1;
	    }
	}

	if ( ( nDisplayMode & VO_INTF_CVBS != 0 ) 
        && 
		( ( nDisplayMode & VO_INTF_VGA != 0 ) || ( nDisplayMode & VO_INTF_HDMI != 0 ) ) 
		)
	{
		/******************************************
		   step 6: start vo SD0 (CVBS) (WBC target) 
		  ******************************************/
		/*
		s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DSD0, NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
	        return -1;
	    }
	  */
		printf("start vo SD0: wbc from hd1\n");
	    u32WndNum = 4;
	    enVoMode = VO_MODE_1MUX;//不能用4mux

	    stVoPubAttrSD.enIntfSync = VO_OUTPUT_PAL;
	    stVoPubAttrSD.enIntfType = VO_INTF_CVBS;
	    stVoPubAttrSD.u32BgColor = 0x000000ff;
	    stVoPubAttrSD.bDoubleFrame = HI_FALSE;
		
	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	       // return -1;
	    }

		s32Ret = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0,  enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StopChn failed!\n");
	        //return -1;
	    }
	    
	    s32Ret = SAMPLE_COMM_VO_StartChn(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 7: start HD1 WBC 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_GetWH(VO_OUTPUT_PAL, \
	                      &stWbcAttr.stTargetSize.u32Width, \
	                      &stWbcAttr.stTargetSize.u32Height, \
	                      &stWbcAttr.u32FrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
	       // return -1;
	    }
	    stWbcAttr.enPixelFormat = SAMPLE_PIXEL_FORMAT;
        //////////////////
        //stWbcAttr.stTargetSize.u32Height=stWbcAttr.stTargetSize.u32Height*2;
	    /////////////////

	    s32Ret = HI_MPI_VO_SetWbcAttr(SAMPLE_VO_DEV_DHD1, &stWbcAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	      //  return -1;
	    }
	    
	    s32Ret = HI_MPI_VO_EnableWbc(SAMPLE_VO_DEV_DHD1);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 8: Bind HD1 WBC to SD0 Chn0 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_BindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0, 0);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	}

	HideAllFB(NULL);
	return s32Ret;
}


HI_S32 UnInitMuxDisplay( int nChannelNo, 
					  int nDisplayMode, 
					  int nVoDev )
{
	printf("check point 10 %d %d\n", nDisplayMode, nVoDev );
    //return UnInit9MuxDisplay(nChannelNo,nDisplayMode,nVoDev);
	
	int VoChn = 0;
	int VpssGrp = nChannelNo-1;
	VoChn = nChannelNo-1;

	HI_S32 sRet=0;

	
	if ( (nDisplayMode & ((int)VO_INTF_HDMI)) != 0 )
	{
		SAMPLE_COMM_VO_HdmiStop();
	}

    /*
	sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "unbind vo wbc error");
	}

	
	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "disable wbc error");
	}

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
	
	
	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop channel error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}


	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	


	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	*/
	//return 0;
	if ( ( (nDisplayMode & (int)VO_INTF_CVBS) != 0 ) && 
		( ( (nDisplayMode & (int)VO_INTF_VGA) != 0 ) || 
		( (nDisplayMode & (int)VO_INTF_HDMI) != 0 ) ) )
	{

		
		
		sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "unbind vo wbc error");
		}

		
    	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "disable wbc error");
		}

		sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}	
	
		
		
		
    	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_4MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop channel error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}

		sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_4MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
	}
	
	if ((nDisplayMode & (int)VO_INTF_VGA) != 0 || 
		(nDisplayMode & (int)VO_INTF_HDMI) != 0 )
	{

		sRet = SAMPLE_COMM_VO_UnBindVpss(nVoDev, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}
		
		sRet = SAMPLE_COMM_VO_StopChn(nVoDev, VO_MODE_4MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}

		sRet = SAMPLE_COMM_VO_StopDevLayer(nVoDev);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
		
	}

	
	return 0;
}


HI_S32 Init9MuxDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev)
{
    VoDev = 0;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr; 
    VO_PUB_ATTR_S stVoPubAttrSD; 
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
	HI_S32 s32Ret=0;
	HI_S32 u32WndNum = 0;
	
    VO_WBC_ATTR_S stWbcAttr;
	printf("init display %d\n", nDisplayMode );
	if ( ( nDisplayMode & (int)VO_INTF_VGA ) != 0 || ( nDisplayMode & (int)VO_INTF_HDMI ) != 0 )
	{

		/******************************************
	     step 5: start vo HD1 (VGA+HDMI) (DoubleFrame) (WBC source) 
	    ******************************************/
	    printf("start vo hd1: HDMI + VGA, wbc source\n");

		//if ( ( nDisplayMode & (int)VO_INTF_CVBS) != 0 )
		//{
	    VoDev = SAMPLE_VO_DEV_DHD1;
		
		
		u32WndNum = 9;
	    enVoMode = VO_MODE_9MUX;
	    
	    stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E)nResolution;
		stVoPubAttr.enIntfType = 0;

		stVoPubAttr.enIntfType = (VO_INTF_HDMI | VO_INTF_VGA);
		
	    stVoPubAttr.u32BgColor = 0x000000ff;
	    stVoPubAttr.bDoubleFrame = HI_FALSE;  // enable doubleframe


		

	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	        //return -1;
	    }
	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	        //return -1;
	    }

		 /* if it's displayed on HDMI, we should start HDMI */
	    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
	    {
	        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
	        {
	            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
	            //return -1;
	        }
	    }
	    
		

	    /******************************************
	     step 9: HD1 switch mode 
	    ******************************************/
	    enPreVoMode = enVoMode;
	    u32WndNum = 9;
	    enVoMode = VO_MODE_9MUX;

	    s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }

		
	    s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }
	    

	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }
		
	    s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

		int VpssGrp = nChannelNo-1;
	    VoChn = nChannelNo-1;
		VoDev = SAMPLE_VO_DEV_DHD1;
	    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
	        //return -1;
	    }

	}

	if ( ( nDisplayMode & VO_INTF_CVBS != 0 ) 
        && 
		( ( nDisplayMode & VO_INTF_VGA != 0 ) || ( nDisplayMode & VO_INTF_HDMI != 0 ) ) 
		)
	{

		/******************************************
		   step 6: start vo SD0 (CVBS) (WBC target) 
		  ******************************************/
		/*
		s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DSD0, NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
	        return -1;
	    }
	  */
		printf("start vo SD0: wbc from hd1\n");
	    u32WndNum = 9;
	    enVoMode = VO_MODE_1MUX;//不能用4mux

	    stVoPubAttrSD.enIntfSync = VO_OUTPUT_PAL;
	    stVoPubAttrSD.enIntfType = VO_INTF_CVBS;
	    stVoPubAttrSD.u32BgColor = 0x000000ff;
	    stVoPubAttrSD.bDoubleFrame = HI_FALSE;
		
	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	       // return -1;
	    }

		s32Ret = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0,  enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StopChn failed!\n");
	        //return -1;
	    }
	    
	    s32Ret = SAMPLE_COMM_VO_StartChn(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 7: start HD1 WBC 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_GetWH(VO_OUTPUT_PAL, \
	                      &stWbcAttr.stTargetSize.u32Width, \
	                      &stWbcAttr.stTargetSize.u32Height, \
	                      &stWbcAttr.u32FrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
	       // return -1;
	    }
	    stWbcAttr.enPixelFormat = SAMPLE_PIXEL_FORMAT;

	    s32Ret = HI_MPI_VO_SetWbcAttr(SAMPLE_VO_DEV_DHD1, &stWbcAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	      //  return -1;
	    }
	    
	    s32Ret = HI_MPI_VO_EnableWbc(SAMPLE_VO_DEV_DHD1);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 8: Bind HD1 WBC to SD0 Chn0 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_BindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0, 0);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	}

	
	HideAllFB(NULL);
	return s32Ret;
}


HI_S32 UnInit9MuxDisplay( int nChannelNo, 
					  int nDisplayMode, 
					  int nVoDev )
{

	printf("check point 10 %d %d\n", nDisplayMode, nVoDev );

	
	int VoChn = 0;
	int VpssGrp = nChannelNo-1;
	VoChn = nChannelNo-1;

	HI_S32 sRet=0;

	
	if ( (nDisplayMode & ((int)VO_INTF_HDMI)) != 0 )
	{
		SAMPLE_COMM_VO_HdmiStop();
	}

    /*
	sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "unbind vo wbc error");
	}

	
	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "disable wbc error");
	}

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
	
	
	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop channel error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}


	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	


	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}	



	*/
	//return 0;
	if ( ( (nDisplayMode & (int)VO_INTF_CVBS) != 0 ) && 
		( ( (nDisplayMode & (int)VO_INTF_VGA) != 0 ) || 
		( (nDisplayMode & (int)VO_INTF_HDMI) != 0 ) ) )
	{

		
		
		sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "unbind vo wbc error");
		}

		
    	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "disable wbc error");
		}

		sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}	
	
		
		
		
    	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop channel error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}

		sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
	}
	
	if ((nDisplayMode & (int)VO_INTF_VGA) != 0 || 
		(nDisplayMode & (int)VO_INTF_HDMI) != 0 )
	{

		sRet = SAMPLE_COMM_VO_UnBindVpss(nVoDev, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}
		
		sRet = SAMPLE_COMM_VO_StopChn(nVoDev, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}

		sRet = SAMPLE_COMM_VO_StopDevLayer(nVoDev);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
		
	}

	
	return 0;
}


HI_S32 Init16MuxDisplay( int nChannelNo, 
					int nDisplayMode, 
					int nResolution, 
					int nFPS,
					int &VoDev)
{
    VoDev = 0;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr; 
    VO_PUB_ATTR_S stVoPubAttrSD; 
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
	HI_S32 s32Ret=0;
	HI_S32 u32WndNum = 0;
	
    VO_WBC_ATTR_S stWbcAttr;
	printf("init display %d\n", nDisplayMode );
	if ( ( nDisplayMode & (int)VO_INTF_VGA ) != 0 || ( nDisplayMode & (int)VO_INTF_HDMI ) != 0 )
	{

		/******************************************
	     step 5: start vo HD1 (VGA+HDMI) (DoubleFrame) (WBC source) 
	    ******************************************/
	    printf("start vo hd1: HDMI + VGA, wbc source\n");

		//if ( ( nDisplayMode & (int)VO_INTF_CVBS) != 0 )
		//{
	    VoDev = SAMPLE_VO_DEV_DHD1;
		
		
		u32WndNum = 16;
	    enVoMode = VO_MODE_16MUX;
	    
	    stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E)nResolution;
		stVoPubAttr.enIntfType = 0;

		stVoPubAttr.enIntfType = (VO_INTF_HDMI | VO_INTF_VGA);
		
	    stVoPubAttr.u32BgColor = 0x000000ff;
	    stVoPubAttr.bDoubleFrame = HI_FALSE;  // enable doubleframe


		

	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	        //return -1;
	    }
	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	        //return -1;
	    }

		 /* if it's displayed on HDMI, we should start HDMI */
	    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
	    {
	        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
	        {
	            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
	            //return -1;
	        }
	    }
	    
		

	    /******************************************
	     step 9: HD1 switch mode 
	    ******************************************/
	    enPreVoMode = enVoMode;
	    u32WndNum = 16;
	    enVoMode = VO_MODE_16MUX;

	    s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }

		
	    s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }
	    

	    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	        //return -1;
	    }
		
	    s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start VO failed!\n");
	       // return -1;
	    }

		int VpssGrp = nChannelNo-1;
	    VoChn = nChannelNo-1;
		VoDev = SAMPLE_VO_DEV_DHD1;
	    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
	        //return -1;
	    }

	}

	if ( ( nDisplayMode & VO_INTF_CVBS != 0 ) 
        && 
		( ( nDisplayMode & VO_INTF_VGA != 0 ) || ( nDisplayMode & VO_INTF_HDMI != 0 ) ) 
		)
	{

		/******************************************
		   step 6: start vo SD0 (CVBS) (WBC target) 
		  ******************************************/
		/*
		s32Ret = SAMPLE_COMM_VO_MemConfig(SAMPLE_VO_DEV_DSD0, NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_MemConfig failed with %d!\n", s32Ret);
	        return -1;
	    }
	  */
		printf("start vo SD0: wbc from hd1\n");
	    u32WndNum = 16;
	    enVoMode = VO_MODE_1MUX;//不能用4mux

	    stVoPubAttrSD.enIntfSync = VO_OUTPUT_PAL;
	    stVoPubAttrSD.enIntfType = VO_INTF_CVBS;
	    stVoPubAttrSD.u32BgColor = 0x000000ff;
	    stVoPubAttrSD.bDoubleFrame = HI_FALSE;
		
	    s32Ret = SAMPLE_COMM_VO_StartDevLayer(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, nFPS );
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
	       // return -1;
	    }

		s32Ret = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0,  enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StopChn failed!\n");
	        //return -1;
	    }
	    
	    s32Ret = SAMPLE_COMM_VO_StartChn(SAMPLE_VO_DEV_DSD0, &stVoPubAttrSD, enVoMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 7: start HD1 WBC 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_GetWH(VO_OUTPUT_PAL, \
	                      &stWbcAttr.stTargetSize.u32Width, \
	                      &stWbcAttr.stTargetSize.u32Height, \
	                      &stWbcAttr.u32FrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
	       // return -1;
	    }
	    stWbcAttr.enPixelFormat = SAMPLE_PIXEL_FORMAT;

	    s32Ret = HI_MPI_VO_SetWbcAttr(SAMPLE_VO_DEV_DHD1, &stWbcAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	      //  return -1;
	    }
	    
	    s32Ret = HI_MPI_VO_EnableWbc(SAMPLE_VO_DEV_DHD1);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	    
	    /******************************************
	     step 8: Bind HD1 WBC to SD0 Chn0 
	    ******************************************/
	    s32Ret = SAMPLE_COMM_VO_BindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0, 0);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VO_SetWbcAttr failed!\n");
	       // return -1;
	    }
	}

	
	HideAllFB(NULL);
	return s32Ret;
}


HI_S32 UnInit16MuxDisplay( int nChannelNo, 
					  int nDisplayMode, 
					  int nVoDev )
{

	printf("check point 10 %d %d\n", nDisplayMode, nVoDev );

	
	int VoChn = 0;
	int VpssGrp = nChannelNo-1;
	VoChn = nChannelNo-1;

	HI_S32 sRet=0;

	
	if ( (nDisplayMode & ((int)VO_INTF_HDMI)) != 0 )
	{
		SAMPLE_COMM_VO_HdmiStop();
	}

    /*
	sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "unbind vo wbc error");
	}

	
	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "disable wbc error");
	}

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	
	
	
	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop channel error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}

	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}


	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 0,0,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	

	sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD0, 1,1,VPSS_PRE0_CHN);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "un bind vpss error");
	}	


	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD0, VO_MODE_4MUX);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop chn error");
	}
	
	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD0);
	if ( sRet != 0 )
	{
		SAMPLE_PRT( "stop devlayer error");
	}	



	*/
	//return 0;
	if ( ( (nDisplayMode & (int)VO_INTF_CVBS) != 0 ) && 
		( ( (nDisplayMode & (int)VO_INTF_VGA) != 0 ) || 
		( (nDisplayMode & (int)VO_INTF_HDMI) != 0 ) ) )
	{

		
		
		sRet = SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DHD1, SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "unbind vo wbc error");
		}

		
    	sRet = HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "disable wbc error");
		}

		sRet = SAMPLE_COMM_VO_UnBindVpss( SAMPLE_VO_DEV_DHD1, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}	
	
		
		
		
    	sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DSD0, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop channel error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DSD0);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}

		sRet = SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD1, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}
		
    	sRet = SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD1);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
	}
	
	if ((nDisplayMode & (int)VO_INTF_VGA) != 0 || 
		(nDisplayMode & (int)VO_INTF_HDMI) != 0 )
	{

		sRet = SAMPLE_COMM_VO_UnBindVpss(nVoDev, VoChn,VpssGrp,VPSS_PRE0_CHN);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "un bind vpss error");
		}
		
		sRet = SAMPLE_COMM_VO_StopChn(nVoDev, VO_MODE_9MUX);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop chn error");
		}

		sRet = SAMPLE_COMM_VO_StopDevLayer(nVoDev);
		if ( sRet != 0 )
		{
			SAMPLE_PRT( "stop devlayer error");
		}
		
		
	}

	
	return 0;
}


