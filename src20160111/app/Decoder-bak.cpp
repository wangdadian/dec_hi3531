#include "Decoder.h"
#include <arpa/inet.h>
#include "TS2ES.h"
#include "ES2TS.h"
#include "qbox.h"
#include <stdio.h>
#include "TextOnYUV.h"
#include "Hi3531Global.h"
#include "define.h"
#include "mytde.h"
#include "Text2Bmp.h"
#include "MDProtocol.h"
#include "DecoderSetting.h"
#include "mdrtc.h"
#include <stdlib.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>


#define BUF_SIZE 1024 * 1024
bool CDecoder::m_bOsdWorkerThStarted=false;
bool CDecoder::m_bFbOpend=false;
bool CDecoder::m_bOsdWorkerExit=false;
bool CDecoder::m_bMonitorStarted=false;
bool CDecoder::m_bOsdShowFromWeb = false;

//HI_S32 gs_VoDevCnt = 4;

int memfind(unsigned char* pData, int iSize, unsigned char* pSub, int iSub)
{
	if( pData == NULL || iSize < (iSub + 1) || pSub == NULL || iSub <= 0 )
	{
		return -1;
	}
	for( int i = 0; i <= (iSize - iSub - 1) ; i++ )
	{
		if ( (memcmp((pData + i), pSub, iSub) == 0) && (memcmp((pData + i + 1), pSub, iSub) != 0 ) )
		{
			return i;
		}
	}
	return -1;
}
FILE *fes =NULL;

CDecoder::CDecoder( int nVideoOutResolution, 
				 int nDecChannelNo, 
				 int nWbc )
{	
	m_nInited = 0;
	m_isTS = 0;
	m_isQbox = 0;
	m_isPS = 0;
	m_isES = 0;

	m_nVideoOutResolution = nVideoOutResolution;
	m_nDecChannelNo = nDecChannelNo;
	m_nWbc = nWbc;

	m_pTs2Es = new CTS2ES();
	m_pPs2Es = new CPS2ES();
	m_pBuf = (HI_U8 *)malloc( BUF_SIZE );
	m_pAudioBuf = (HI_U8 *)malloc( (SAMPLE_AUDIO_PTNUMPERFRM + 4)*10 );
	m_pAudioSendData = (HI_U8 *)malloc( (SAMPLE_AUDIO_PTNUMPERFRM + 4)*10 );
	m_nAudioIndex = 0;
	m_nCurIndex = 0;
	m_iMpeg2 = 1;

	m_nPayloadType = PT_H264;
	m_enAudPayloadType = PT_G711A;

	m_nAudioCounter = 0;
	m_nContinus = 0;
	m_nLastFormat = AVC_VIDEO_STREAM_TYPE;
	m_nFormat = AVC_VIDEO_STREAM_TYPE;

	m_isTSDiscontinue = 0;

	m_iDecFlag = 0;
	//StartDecoder();
	m_nInited = 1;

	m_pFrame = NULL;
	m_hPool = VB_INVALID_POOLID;
	m_iFrameIndex = 0;
    //Dispaly
    m_thOsdWorker = 0;
    memset(&m_sDR, 0, sizeof(struDisplayResolution));
    m_iDisplayX = SCR_WIDTH1400;
    m_iDisplayY = SCR_HEIGTH300;
    m_eVoMode = VO_MODE_1MUX;
    // osd
    /////
    /*
    struDecOsdInfo sosd;
    CDecoderSetting::GetDecOsdInfo(&sosd, NULL);
    m_bShowOsd = sosd.show==0? false:true;
    if(m_bShowOsd)
    {
        sprintf(m_szOsdText, "%s", sosd.text);
    }
    else
    {
        memset(m_szOsdText, 0, 64*sizeof(char));
    }
    */
    m_bShowOsd = false;
    memset(m_szOsdText, 0, 64*sizeof(char));
    //
    struDecOsdInfo sosdweb;
    CDecoderSetting::GetDecOsdInfoWeb(&sosdweb, NULL);
    m_bOsdShowFromWeb = sosdweb.show==0? false:true;
    
    DecodecOsdParam* ptrDate = new DecodecOsdParam;
    m_osdDateParam = (void*)ptrDate;
    DecodecOsdParam* ptrFont = new DecodecOsdParam;
    m_osdTextParam = (void*)ptrFont;
    m_bTextChanged = false;
    m_bOsdCleared = false;
    m_bFirst = true;
    memset(m_szLastDate, 0, 64*sizeof(char));

    pthread_mutex_init(&m_lockOsd, NULL);
    pthread_mutex_init(&m_lockOpenfb, NULL);
    m_thMoniterWorker=0;

    m_fbfd = 0;
    m_screensize = 0;
    m_fbp = 0;
    if(m_bFbOpend==false &&m_bOsdShowFromWeb==true)
        OpenFbDev();
	/*
	
	HI_U32  u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(VIDEO_ENCODING_MODE_NTSC, 
                PIC_HD1080, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

	

    m_hPool   = HI_MPI_VB_CreatePool( u32BlkSize, 10,NULL );
    if (m_hPool == VB_INVALID_POOLID)
    {
        SAMPLE_PRT("HI_MPI_VB_CreatePool failed! \n");
        goto END_VENC_USER_1;
    }


	memset(&stMem , 0, sizeof(SAMPLE_MEMBUF_S));
    stMem.hPool = hPool;
	*/
	m_iExitFlag = 0;
    m_getframeth = 0;
	m_pFrameBuf = (HI_U8*)malloc( 1920*1200*3 );
    if(m_bMonitorStarted==false)
	{
        pthread_create( &m_thMoniterWorker, NULL, ThMonitorWorker, (void*)this);
	}
	//pthread_create( &m_getframeth,NULL, GetFrameProc, (void*)this);
	pthread_mutex_init(&m_lock, NULL);
	fes = fopen( "test.au", "w" );
    //_DEBUG_("################################## fes [0x%x]",fes);	
}


CDecoder::~CDecoder()
{
    //_DEBUG_("~CDecoder start");
    if(fes!=NULL)
    {
        //_DEBUG_("close fes.");
	    fclose( fes );
    }
    //_DEBUG_("munmap");
    munmap(m_fbp, m_screensize);
    m_fbp = 0;
    close(m_fbfd);
    m_bFbOpend = false;

	m_iExitFlag = 1;
    m_bOsdWorkerExit = true;
    //_DEBUG_("join m_getframeth");
	if ( m_getframeth != 0 )
	{
		pthread_join( m_getframeth, NULL );
		m_getframeth = 0;
        //_DEBUG_("free m_pFrameBuf");
		PTR_FREE( m_pFrameBuf );
		
	}
    //_DEBUG_("join m_thMoniterWorker");
    if ( m_thMoniterWorker!= 0 )
	{
		pthread_join( m_thMoniterWorker, NULL );
		m_thMoniterWorker = 0;
	}
    //_DEBUG_("join m_thOsdWorker");
	if ( m_thOsdWorker != 0 )
	{
		pthread_join( m_thOsdWorker, NULL );
		m_thOsdWorker = 0;
	}
    
	if ( m_iDecFlag == 1 )
    {   
        //_DEBUG_("UnInitVdec");
		UnInitVdec();
    }
	else if ( m_iDecFlag == 2)
    {   
        //_DEBUG_("UnInitVInput1");
		UnInitVInput1();
    }

    //_DEBUG_("free m_pBuf");
	if (m_pBuf != NULL )
	{
		free( m_pBuf );
		m_pBuf = NULL;
	}
    //_DEBUG_("free m_pAudioBuf");
	if (m_pAudioBuf != NULL )
	{
		free( m_pAudioBuf );
		m_pAudioBuf = NULL;
	}
    //_DEBUG_("free m_pAudioSendData");
	if (m_pAudioSendData != NULL )
	{
		free( m_pAudioSendData );
		m_pAudioSendData = NULL;
	}
	//_DEBUG_("delete m_pTs2Es");
	if ( m_pTs2Es != NULL )
	{
		delete m_pTs2Es;
		m_pTs2Es = NULL;
	}
    //_DEBUG_("delete m_pPs2Es");
	if ( m_pPs2Es != NULL )
	{
		delete m_pPs2Es;
		m_pPs2Es = NULL;
	}
    //_DEBUG_("delete m_osdDateParam");
    PTR_DELETE(m_osdDateParam);
    //_DEBUG_("delete m_osdTextParam");
    PTR_DELETE(m_osdTextParam);
    //_DEBUG_("delete end");
	pthread_mutex_destroy( &m_lock );
    pthread_mutex_destroy( &m_lockOsd );
    pthread_mutex_destroy( &m_lockOpenfb);
    //_DEBUG_("~CDecoder end");
}


void CDecoder::RemoveVissHead( )
{

	unsigned char szVissHead[4] = {0x3E, 0x3E, 0x3E, 0x3E };
	unsigned int qboxpos = 0;
	while ( qboxpos < m_nCurIndex  )
	{
		int nextqboxpos = memfind(m_pBuf+qboxpos, m_nCurIndex, szVissHead, sizeof(VissHead));
		if ( nextqboxpos < 0 )
		{
			break;
		}
		//printf( "find visshead %d\n", nextqboxpos );
		qboxpos += nextqboxpos;

		//进一步读取qbox信息确定码流是m-jpeg还是h.264
		if (  m_nCurIndex-qboxpos >= sizeof(VissHead) )
		{
			//qbox->HeaderSize
			unsigned int boxsize = sizeof(VissHead);
			if ( boxsize > 0 && boxsize+qboxpos < m_nCurIndex )
			{
				memmove( m_pBuf + qboxpos, m_pBuf+qboxpos+boxsize, boxsize );
				m_nCurIndex -= boxsize;
				qboxpos += boxsize;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
}
void* CDecoder::ThMonitorWorker(void * param)
{
    m_bMonitorStarted = true;
    CDecoder* pThis = (CDecoder*)param;
    sleep(3);
    _DEBUG_("start!");
    while( pThis->m_iExitFlag ==0)
    {
        if(pThis->m_bFbOpend==false)
        {
            pThis->OpenFbDev();
        }
        sleep(1);
    }
    _DEBUG_("exit!");
    return NULL;
}
int CDecoder::ReplaceQBoxLength(BYTE *pbData, int iDataLength)
{
	// Slice的头
	char szNal[4] = {0,0,0,1};

	int iSliceLength;
	int iCurrent = 0;

	while ( iCurrent <= (iDataLength-4) )
	{
		// 获取此段Slice的长度
		iSliceLength = (*(pbData + iCurrent) << 24) + (*(pbData + iCurrent + 1) << 16)
			+ (*(pbData + iCurrent + 2) << 8) + (*(pbData + iCurrent + 3));

		//printf( "slice length:%d\n", iSliceLength );
		// 判断长度是否合法
		if ( iSliceLength<0 || //(iCurrent + iSliceLength)>iDataLength ||
			iSliceLength > 1000000 )
		{
			return -1;
		}

		// 将长度替换为Slice头
		memcpy( (pbData + iCurrent), szNal, sizeof(szNal) );

		iCurrent += sizeof(szNal) + iSliceLength;
	}

	m_isQbox++;
	if ( m_isQbox <=  0 ) m_isQbox = 1;
	return 0;
}

int CDecoder::RemoveQbox(BYTE *pbFrameData, int lFrameSize, int &nRemoveIndex)
{
	int nRet = -1;
	if ( NULL==pbFrameData || 0>=lFrameSize )
	{
		printf("NULL==pbFrameData || 0>=lFrameSize\n");
		return nRet;
	}

	// 查找VISS头标志">>>>"(为避免有的帧的最后几个字节出现'>', 多判断一个标志头后不是0x3E的字节)
	bool bFirstFind = false;
	for(int i = 0; i<(lFrameSize - 4);i++ )
	{
		if ( 0x3E==*(pbFrameData + i) 
			&& 0x3E==*(pbFrameData + i + 1)
			&& 0x3E==*(pbFrameData + i + 2)
			&& 0x3E==*(pbFrameData + i + 3)
			&& 0x3E!=*(pbFrameData + i + 4) )
		{

			//printf( "find viss head:%d\n", i );
			// 查找到VISS头
			_DEBUG_("bFirstFind=%d, i=%d, m_nCurIndex=%d", bFirstFind, i, m_nCurIndex);
			if (bFirstFind)
			{
				int nHeaderLen = (VISS_HEADER_SIZE + QBOX_HEADER_SIZE);
				lFrameSize -= nHeaderLen;
				memmove(pbFrameData, (pbFrameData + nHeaderLen), lFrameSize);
				_DEBUG_("move qbox&viss header, memmove len=%d, buffer len=%d", nHeaderLen, lFrameSize);
				m_nCurIndex -= nHeaderLen;
				i -= nHeaderLen;
				nRemoveIndex = i;
				//printf("replase qbox len\n");
				if ( ReplaceQBoxLength(pbFrameData, i) < 0 )
				{
					_DEBUG_("ReplaceQBoxLength return <0");
					/*lFrameSize -= i;
					memmove(pbFrameData, (pbFrameData + i), lFrameSize);
					nRemoveIndex = 0;
					m_nCurIndex -= i;*/
					nRemoveIndex = -1;
				}
			}

			// 去除QBox头前的数据
			if ( !bFirstFind && i>0 )
			{
				BYTE* pTmp = pbFrameData+i+VISS_HEADER_SIZE+QBOX_HEADER_SIZE;
			    int iLength = (*(pTmp) << 24) + (*(pTmp + 1) << 16)
			                  + (*(pTmp + 2) << 8) + (*(pTmp + 3));
				
				// 判断长度是否合法
				if ( iLength<0 || 
					iLength > 1000000 )
				{
					_DEBUG_("first find, invaild length=%d", iLength);
					nRemoveIndex = lFrameSize;
					nRet = 0;
					return nRet;
				}
				lFrameSize -= i;
				memmove(pbFrameData, (pbFrameData + i), lFrameSize);
				_DEBUG_("first memove, move len=%d, buffer len=%d", i, lFrameSize );
				i = 0;
				nRemoveIndex = 0;
			}

			bFirstFind = true;
		}
	}

	if (bFirstFind)
	{
		nRet = 0;
	}
	return nRet;
}

void CDecoder::ChangeVdec()
{
	if ( m_nLastFormat != m_nFormat && m_nContinus >= 50 )
	{
		UnInitVdec();
		_DEBUG_("before change vdec, m_nLastFormat=%d, m_nFormat=%d", m_nLastFormat, m_nFormat);
		switch( m_nLastFormat )
		{
		case MPEG2_VIDEO_STREAM_TYPE:
			m_nPayloadType = PT_MPEG2VIDEO;
			break;
		default:
			m_nPayloadType = PT_H264;
			break;
		};

		InitVdec();
		m_nFormat = m_nLastFormat;
		_DEBUG_("after change vdec, m_nLastFormat=%d, m_nFormat=%d", m_nLastFormat, m_nFormat);
	}
	else
	{
		//_DEBUG_("m_nLastFormat=%d, m_nFormat=%d, m_nContinus=%d",  m_nLastFormat, m_nFormat, m_nContinus);
	}
}

/*
void CDecoder::ShowDate(bool bShow, int x, int y )
{
    printf("\n####Date: show %d; x %d; y %d\n\n\n", bShow,  x, y);

    
}

void CDecoder::ShowOSD( bool bShow, int x, int y, char *szText )
{
    printf("\n#####Text: show %d; x %d; y %d; text %s\n\n", bShow, x, y, szText);

    
}
*/
void* CDecoder::ThOsdWorker(void* param)
{
    if(m_bOsdWorkerThStarted==true)
        return NULL;
    CDecoder* pThis = (CDecoder*)param;
    _DEBUG_("start!");
    _DEBUG_("########################OSD Worker Thread Start [%d]", pThis->m_nDecChannelNo);    
    while( m_bOsdWorkerExit == false )
    {
        pthread_mutex_lock(&pThis->m_lockOsd);
        if(pThis->m_bOsdCleared == false &&
            (pThis->m_bShowOsd==false||m_bOsdShowFromWeb==false))
        {
            // 清空OSD显示
            _DEBUG_("########### Clear OSD");
            pThis->DisplayOsd(false);
        }
        else if(pThis->m_bShowOsd == true && m_bOsdShowFromWeb==true)
        {
            // 使能OSD显示
            pThis->DisplayOsd(true);
            //_DEBUG_("########### SHOW OSD");
        }

        pthread_mutex_unlock(&pThis->m_lockOsd);
        myMsleep(400);
    }
    _DEBUG_("##########################OSD Worker Thread Exit [%d]", pThis->m_nDecChannelNo);
    m_bOsdWorkerThStarted = false;
    _DEBUG_("exit!");
    return NULL;
}

void CDecoder::SetOsdShow(bool bShow)
{
    pthread_mutex_lock(&m_lockOsd);
    m_bShowOsd = bShow;
    pthread_mutex_unlock(&m_lockOsd);
}
void CDecoder::SetOsdShowWeb(bool bShow)
{
    pthread_mutex_lock(&m_lockOsd);
    m_bOsdShowFromWeb = bShow;
    pthread_mutex_unlock(&m_lockOsd);
}

void CDecoder::SetOsdDate(void* pData)
{
    if(pData == NULL) return;
    pthread_mutex_lock(&m_lockOsd);
    memcpy(m_osdDateParam, pData, sizeof(DecodecOsdParam));
    DecodecOsdParam* ptr = (DecodecOsdParam*)m_osdDateParam;
    char *p = ptr->buffer_content;
    int iServerTime = (int)((p[0]&0xffffffff)<<24) + (int)((p[1]&0xffffffff)<<16) + (int)((p[2]&0xffffffff)<<8) + (int)p[3];
    int iLocalRtcTime  = (int)CRtc::read_rtc(0);
    int iLocalSysTime = 0;
    //_DEBUG_("Server Time: %d", iServerTime);
    //_DEBUG_("Local RTC Time: %d", iLocalRtcTime);
    //误差超过5秒，则更新本地时间
    if( abs(iServerTime-iLocalRtcTime) > 3 )
    {
        //_DEBUG_("Set Local Date!!!!!");
        //设置本地时间
        CRtc::write_rtc((time_t)(iServerTime), 0);
        // 更新系统时间
        char szCmd[100] = {0};
        sprintf(szCmd, "/bin/date -s \"%s\"", conv_time(iServerTime));
        system(szCmd);
        goto GOTO_RET1;
    }
    iLocalSysTime = (int)time(NULL);
    //_DEBUG_("Local SYS Time: %d", iLocalSysTime);
    // 本地硬件时间和系统时间，以硬件时间为准
    if( abs(iLocalSysTime-iLocalRtcTime) > 5 )
    {
        // 更新系统时间
        char szCmd[100] = {0};
        sprintf(szCmd, "/bin/date -s \"%s\"", conv_time(iLocalRtcTime));
        system(szCmd);
        goto GOTO_RET1;

    }
    /*
    FILE* fp = popen("/bin/date", "r");
    int iSysTime = 0;
    char szBuf[100] = {0};
    fgets(szBuf, 100, fp);
    if( strstr(szBuf, "UTC") != NULL )
        iSysTime += 8*3600;// 如果是UTC时间，在中国时区需+8小时
    pclose(fp);
    */
    //printf("\n####Date: x %d; y %d; time: %s\n\n",  ptr->x_time, ptr->y_time, conv_time(m_iSysTime));
GOTO_RET1:
    pthread_mutex_unlock(&m_lockOsd);
}

void CDecoder::SetOsdText(void* pData)
{
    ///////////////////////////////////////////////////////////////////////////
    // TEST
    if(0)
    {//
        HI_S32 s32Ret = 0;
        VO_VIDEO_LAYER_ATTR_S stLayerAttr;
        s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoDev, &stLayerAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("get display fps failed with %#x!\n", s32Ret);
            return ;
        }
        SAMPLE_COMM_VO_StopDevLayer(VoDev);
        HI_MPI_VO_Enable(VoDev);
        stLayerAttr.u32DispFrmRt = 25;
        stLayerAttr.stImageSize = {1024, 768};
        s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoDev, &stLayerAttr);
        HI_MPI_VO_EnableVideoLayer( VoDev );
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("set display fps failed with %#x!\n", s32Ret);
            return ;
        }
    }
    ////////////////////////////////////////////////////////////////////////////
    if(pData == NULL) return;
    pthread_mutex_lock(&m_lockOsd);
    memcpy(m_osdTextParam, pData, sizeof(DecodecOsdParam));
    DecodecOsdParam* ptr = (DecodecOsdParam*)m_osdTextParam;
    if( strcmp(m_szOsdText, ptr->buffer_content) == 0 )
    {
        m_bTextChanged = false;
    }
    else
    {
        sprintf(m_szOsdText, "%s", ptr->buffer_content);
        m_bTextChanged = true;
    }
    // UTF-8转换成GBK
    //char szTmpBuf[200] = {0};
	//printf("osd before...%s\n", m_szOsdText );
    //utf8togbk(szTmpBuf, m_szOsdText, 200);
    //sprintf(m_szOsdText, "%s", szTmpBuf);

    //printf("\n#####Text: x %d; y %d; text %s\n\n", ptr->x_time, ptr->y_time, m_szOsdText);

    pthread_mutex_unlock(&m_lockOsd);
}

void CDecoder::SetDispaly(const struDisplayResolution* pDR, SAMPLE_VO_MODE_E eVoMode)
{
    if(pDR == NULL || pDR->channel-1!=0)
    {
        return;
    }
    //return;
	//
    m_eVoMode = eVoMode;
    memcpy(&m_sDR, pDR, sizeof(struDisplayResolution));
    /*0-未知、未设置
    1-1920x1080
    2-800x600
    3-1024x768
    4-1280x1024
    5-1366x768
    6-1440x900
    7-1280x800
    */
    unsigned int iDisplayX=SCR_WIDTH800;
    switch(m_sDR.resolution)
    {
        case 0:
        case 2:
            iDisplayX=SCR_WIDTH800;
            break;
        case 1:
        case 6:
            iDisplayX=SCR_WIDTH1400;
            break;
        case 3:
        case 4:
        case 5:
        case 7:
            iDisplayX=SCR_WIDTH1000;
            break;
        default:
            iDisplayX=SCR_WIDTH800;
            break;
    }
    m_iDisplayY=SCR_HEIGTH300;
    if(m_iDisplayX==iDisplayX)
    {
        return;
    }
    else
    {
        m_iDisplayX = iDisplayX;
        OpenFbDev();
    }
}

char* CDecoder::conv_time(int itime)

{
    //setenv("TZ", "GMT-8", 1);
    //tzset();
    static char szBuff[80] = {0};
    memset(szBuff, 0, 80);
    struct tm mytm;
    localtime_r((time_t*)&itime, &mytm);
    sprintf(szBuff, "%d-%02d-%02d %02d:%02d:%02d", mytm.tm_year+1900, \
           mytm.tm_mon+1, mytm.tm_mday, mytm.tm_hour, \
           mytm.tm_min, mytm.tm_sec);
    return szBuff;
}

long CDecoder::DisplayOsd(bool bShow)
{
    // 清空OSD显示
    if(bShow == false)
    {
        m_bOsdCleared = true;
        memset(m_fbp, 0, m_screensize);
        m_bFirst = true;
        m_bTextChanged = true;
        //memset(m_szOsdText, 0 , 64);
        //memset(m_szLastDate, 0 , 64);
        return 0;
    }
    
    //_DEBUG_("#######start0000!");
    // 时间字符串
    char szDate[64] = {0};
    // 时间点阵码列表
    unsigned char* pDateCodeList[64];
    // 文字是否为字符或汉字--时间
    int pDateHalfList[64];
    // 指示上述两个变量的长度
    int nDateCount = 64;
    // 文字点阵码列表
    unsigned char* pTextCodeList[64];
    // 文字是否为字符或汉字
    int pTextHalfList[64];
    // 指示上述两个变量的长度
    int nTextCount = 64;
    // 函数返回值
    int iRet = 0;
    //循环体变量
    int m,i,j;
    // 时间字符串宽度，用于间隔字符
    unsigned int uiDateStrWidth = 0;
    // 文字字符串宽度，用于间隔字符
    unsigned int uiTextStrWidth = 0; 
    // OSD高度
    unsigned int uiOsdHeight = 0; 
    // 字体颜色
    short sWhiteColor = 0xffff;// 白色
    // 描边颜色
    short sBlackColor = 0x8000;// 黑色
    //
    // OSD属性
    // 字体点阵大小
    int iFont = 64;
    // 字体宽度，如果是字符则为iFont的一半，汉字为iFont
    int iFontWidth = 0;
    //时间坐标
    unsigned int iTimeX = 200;
    unsigned int iTimeY = 80;
    // 文字坐标
    unsigned int iTextX = 200;
    unsigned int iTextY = 150;
    unsigned int iIndex = 0;
    bool bFirst = false;
    unsigned int iStride = m_vinfo.xres * 2;
    bool bDateChanged = false;
    int iLocalTime = (int)time(NULL);//(int)CRtc::read_rtc(0);
    
    
    // 获取当前时间的字符串    
    sprintf(szDate, "%s", conv_time(iLocalTime));
    //_DEBUG_("Get Loacal Date: %d [%s]", iLocalTime, szDate);
    if(strcmp(m_szLastDate, szDate) != 0)
    {
        bDateChanged = true;
    }

    //屏幕的第一个像素不为白色，说明被其他代码意外刷新，则重新显示OSD信息
    if( *((unsigned short *)(m_fbp)) != 0xffff )
    {
        m_bFirst = true;
    }
    if( m_bFirst == true)
    {
        memset(m_fbp, 0, m_screensize);
        bFirst = true;
        m_bTextChanged = true;
        bDateChanged = true;
        memset(m_szLastDate, 0 , 64);
    }

    // 如果时间和文字OSD均未改变，则直接返回不刷新屏幕
    if(m_bTextChanged == false && bDateChanged == false)
    {
        //_DEBUG_("Date and Text NOT CHANGED!!");
        return 0;
    }
    //_DEBUG_("#######start1111!");
    // 获取时间字符串点阵列表
    if(bDateChanged == true)
    {
        iRet = m_textOnYuv.GetCodeList( szDate, iFont, &pDateCodeList[0], &pDateHalfList[0], nDateCount);
        if(iRet != 0)
        {
            iRet = -1;
            goto GOTO_ERR2;
        }
    }
    //
    // 获取文字字符串点阵列表(文字不经常更改)
    if(m_bTextChanged == true)
    {
        iRet = m_textOnYuv.GetCodeList( m_szOsdText, iFont, &pTextCodeList[0], &pTextHalfList[0], nTextCount);
        if(iRet != 0)
        {
            iRet = -1;
            goto GOTO_ERR3;
        }
    }
    //_DEBUG_("#######start2222!");
    
    //将屏幕的第一个像素置为白色，用于指示屏幕是否被其他代码刷新
    *((unsigned short *)(m_fbp))=0xffff;
    //
    // 先判断高度是否超过最大分辨率
    uiOsdHeight = iTimeY + iFont;
    if( uiOsdHeight > m_iDisplayY)
    {
        _DEBUG_("ERROR: OSD(date) Height %d > %d Screen Height!", uiOsdHeight, m_iDisplayY);
        iRet = -1;
        goto GOTO_ERR4;
    }
    // 更新时间点阵数据
    if(bFirst == true || bDateChanged == true)
    {
        uiDateStrWidth = iTimeX;
        // 获取本次获取的时间字符串和上次的时间字符串的不同处的位置索引，并确定相同字符串的宽度
        for(i=0; i<nDateCount; i++)
        {
            if(pDateHalfList[i]==0)
            {
                iFontWidth = iFont;
            }
            else
            {
                iFontWidth = iFont / 2;
            }
            // 确定未改变的字符串宽度
            if(i != 0)
            {
                uiDateStrWidth = uiDateStrWidth + iFontWidth + WORD_SPACE;
            }

            if(m_szLastDate[i] != szDate[i])
            {
                iIndex = i;
                break;
            }
        }
        //printf("%s\n%s\ni=%d\n", m_szLastDate, szDate, i);
        for(m=iIndex; m<nDateCount; m++)
        {
            if(pDateHalfList[m]==0)
            {
                iFontWidth = iFont;
            }
            else
            {
                iFontWidth = iFont / 2;
            }
            if( (uiDateStrWidth+iFontWidth+WORD_SPACE) > m_iDisplayX)
            {
                iRet = -1;
                _DEBUG_("Error: Time Width=%d > %d Screen Width\n", (uiDateStrWidth+iFontWidth+WORD_SPACE), m_iDisplayX);
                //goto GOTO_ERR5;
                break;
            }

            for(j=0; j<iFont; j++)
            {
                memset(m_fbp + (j+iTimeY)*iStride + uiDateStrWidth*2, 0, iStride-uiDateStrWidth*2);
                for(i=0; i<iFontWidth; i++)
                {
                    if(pDateCodeList[m][j*iFontWidth+i]==255)
                    {
                        //白色点阵
                        *((unsigned short *)(m_fbp + (j+iTimeY)*iStride + uiDateStrWidth*2 + i*2)) = sWhiteColor;
                    }
                    else if(pDateCodeList[m][j*iFontWidth+i]==128)
                    {
                        //黑色描边
                        *((unsigned short *)(m_fbp + (j+iTimeY)*iStride + uiDateStrWidth*2 + i*2)) = sBlackColor;
                    }
                }
            }
            if(m != nDateCount-1)
            {
                uiDateStrWidth = uiDateStrWidth + iFontWidth + WORD_SPACE;
            }
        }
    }

    // 先判断高度是否超过最大分辨率
    uiOsdHeight = iTextY + iFont;
    if( uiOsdHeight > m_iDisplayY)
    {
        _DEBUG_("ERROR: OSD(text) Height %d > %d Screen Height!", uiOsdHeight, m_iDisplayY);
        iRet = -1;
        goto GOTO_ERR5;
    }

    // 更新文字点阵数据
    if(m_bTextChanged == true || bFirst == true)
    {
        // 开始打印文字部分点阵，重新计算字符串宽度
        uiTextStrWidth = iTextX;
        //memset(m_fbp + iTextY*iStride, 0, iStride*(m_vinfo.yres-iTextY));
        memset(m_fbp + iTextY*iStride, 0, iStride*iFont);
        for(m=0; m<nTextCount; m++)
        {
            if(pTextHalfList[m]==0)
            {
                iFontWidth = iFont;
            }
            else
            {
                iFontWidth = iFont / 2;
            }
            if((uiTextStrWidth+iFontWidth+WORD_SPACE) > m_iDisplayX)
            {
                iRet = -1;
                _DEBUG_("Error: Text Width %d > %d Screen Width[word index=%d]\n", (uiTextStrWidth+iFontWidth+WORD_SPACE), m_iDisplayX, m+1);
                //goto GOTO_ERR5;
                break;
            }
            for(j=0; j<iFont; j++)
            {
                for(i=0; i<iFontWidth; i++)
                {
                    if(pTextCodeList[m][j*iFontWidth+i]==255)
                    {
                        //白色点阵
                        *((unsigned short *)(m_fbp + (j+iTextY)*iStride + uiTextStrWidth*2 + i*2)) = sWhiteColor;
                    }
                    else if(pTextCodeList[m][j*iFontWidth+i]==128)
                    {
                        //黑色描边
                        *((unsigned short *)(m_fbp + (j+iTextY)*iStride + uiTextStrWidth*2 + i*2)) = sBlackColor;
                    }
                }
            }
            if(m != nTextCount-1)
            {
                uiTextStrWidth = uiTextStrWidth + iFontWidth + WORD_SPACE;
            }
        }
    }
    //_DEBUG_("#######start3333!");

GOTO_ERR5:
    m_bFirst = false;
    m_bTextChanged = false;
    sprintf(m_szLastDate, "%s", szDate);
    m_bOsdCleared = false;
GOTO_ERR4:
    //PTR_DELETE_A(pTextCodeList)
    //PTR_DELETE_A(pTextHalfList);
GOTO_ERR3:
    //PTR_DELETE_A(pDateCodeList)
    //PTR_DELETE_A(pDateHalfList);

GOTO_ERR2:
    return iRet;
}

long CDecoder::OpenFbDev()
{
    //_DEBUG_("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");
    pthread_mutex_lock(&m_lockOpenfb);
    m_bOsdWorkerExit = true;
	if ( m_thOsdWorker != 0 )
	{
		pthread_join( m_thOsdWorker, NULL );
		m_thOsdWorker = 0;
	}
    long lRet = -1;
    struct fb_bitfield stR32 = {10, 5, 0};
    struct fb_bitfield stG32 = {5, 5, 0};
    struct fb_bitfield stB32 = {0, 5, 0};
    struct fb_bitfield stA32 = {15, 1, 0};
    //
    // 打开帧缓冲设备
    if(m_fbfd)
    {
        // 清屏
        memset(m_fbp, 0, m_screensize);
        munmap(m_fbp, m_screensize);
        m_fbp = 0;
        close(m_fbfd);
        m_fbfd=0;
        m_bFbOpend = false;
    }
    m_bFirst = true;
    m_fbfd = open("/dev/fb1", O_RDWR);  
    if( m_fbfd<= 0 )
    {
        _DEBUG_("open /dev/fb1 error!\n");
        goto GOTO_ERR1;
    }
    if( ioctl(m_fbfd, FBIOGET_VSCREENINFO, &m_vinfo) )
    {  
        // 获取屏幕可变参数
        _DEBUG_("error: ioctl FBIOGET_VSCREENINFO\n");
        goto GOTO_ERR1;
    }

    m_vinfo.xres_virtual = m_iDisplayX;
    m_vinfo.yres_virtual = m_iDisplayY;
    m_vinfo.xres = m_iDisplayX;
    m_vinfo.yres = m_iDisplayY;
    m_vinfo.activate  		= FB_ACTIVATE_NOW;
    m_vinfo.bits_per_pixel	= 16;
    m_vinfo.xoffset = 0;
    m_vinfo.yoffset = 0;
    m_vinfo.red   = stR32;
    m_vinfo.green = stG32;
    m_vinfo.blue  = stB32;
    m_vinfo.transp = stA32;

    if (ioctl(m_fbfd, FBIOPUT_VSCREENINFO, &m_vinfo) < 0)
    {
        _DEBUG_("process frame buffer device error:FBIOPUT_VSCREENINFO\n");
        goto GOTO_ERR1;
    }
    if (ioctl(m_fbfd, FBIOGET_FSCREENINFO, &m_finfo) < 0)
    {
        _DEBUG_("error: ioctl FBIOGET_FSCREENINFO\n");
        goto GOTO_ERR1;
    }

    // 打印屏幕可变参数
    //printf("SCREEN: %d x %d, %dbpp\n", m_vinfo.xres, m_vinfo.yres, m_vinfo.bits_per_pixel);
    // 缓冲区字节大小
    //m_screensize = m_vinfo.xres * m_vinfo.yres * m_vinfo.bits_per_pixel / 2;
    m_screensize = m_finfo.smem_len;
    // 映射
    m_fbp = (char *)mmap(0, m_screensize, PROT_READ|PROT_WRITE, MAP_SHARED, m_fbfd, 0);
    if( (int)m_fbp == -1 )
    {
        _DEBUG_("error: mmap\n");
        goto GOTO_ERR1;
    }
    lRet = 0;
GOTO_ERR1:
    if(lRet != 0)
    {
        if(m_fbfd)
        {
            close(m_fbfd);
            m_fbfd = 0;
        }
        m_bFbOpend = false;
        pthread_mutex_unlock(&m_lockOpenfb);
        return lRet;
    }
    // 清屏
    memset(m_fbp, 0, m_screensize);
    m_bOsdWorkerExit = false;
    pthread_create(&m_thOsdWorker, NULL, ThOsdWorker, (void*)this);
    m_bFbOpend = true;
    pthread_mutex_unlock(&m_lockOpenfb);

    return 0;
}


void CDecoder::SendAudioFrame( unsigned char *pData, int nLen )
{

	//fwrite( pData, nLen, 1, fes );
	HISI_AUDIO_HEAD audiohead;

	/*
	memcpy( &audiohead, pData, 4 );
	_DEBUG_( "type:%d seq:%d len:%d reserv:%d", 
		audiohead.bType,
		audiohead.bCounter, 
		audiohead.bLen, 
		audiohead.bReserved );
	*/

	audiohead.bReserved = 0;
	audiohead.bLen = nLen/2;
	audiohead.bCounter = 1;
	audiohead.bType = 1;
	
	memcpy( m_pAudioSendData, &audiohead, sizeof(audiohead) );
	memcpy( m_pAudioSendData+sizeof(audiohead), pData, nLen );
	
	AUDIO_STREAM_S stAudioStream;
	memset( &stAudioStream, 0, sizeof(stAudioStream));
	stAudioStream.pStream = m_pAudioSendData;
	stAudioStream.u32Len = nLen+sizeof(audiohead) ;
	HI_S32 s32Ret = HI_MPI_ADEC_SendStream(m_nDecChannelNo, &stAudioStream, HI_IO_NOBLOCK);

	if (HI_SUCCESS != s32Ret)
	{
		//_DEBUG_("send stream error:0x%x", s32Ret );
		//UninitAdec();
		//InitAdec();

	}
}

void *CDecoder::GetFrameProc(void*  arg)
{

	//return NULL;
	CDecoder *pThis = ( CDecoder*)arg;
	pThis->GetFrame();

	
	return NULL;
}


/*
int CDecoder::CloneFrame(VIDEO_FRAME_INFO_S oldframe, VIDEO_FRAME_INFO_S &newframe )
{

	HI_U32             u32LStride;
    HI_U32             u32CStride;
    HI_U32             u32LumaSize;
    HI_U32             u32ChrmSize;
    HI_U32             u32Size;
	HI_U32             u32Stride;
	HI_U32 	           u32Height;
	HI_U32             u32Width;

	u32Height = oldframe.stVFrame.u32Height;
	u32Width = oldframe.stVFrame.u32Width;
	u32Stride = oldframe.stVFrame.u32Stride[0];
	u32LStride  = u32Stride;
    u32CStride  = u32Stride;

    u32LumaSize = (u32LStride * u32Height);
    u32ChrmSize = (u32CStride * u32Height) >> 2;
    u32Size = u32LumaSize + (u32ChrmSize << 1);
	u32Stride = oldframe.stVFrame.u32Stride[0];

	memcpy( &newframe, &oldframe, sizeof(oldframe));

	
    VB_BLK VbBlk;
    HI_U32 u32PhyAddr;
    HI_U8 *pVirAddr;

	if ( m_hPool == VB_INVALID_POOLID )
	{
		HI_U32 u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(VIDEO_ENCODING_MODE_NTSC,
					PIC_HD1080, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	
		m_hPool   = HI_MPI_VB_CreatePool( u32BlkSize, 10,NULL );
		if (m_hPool == VB_INVALID_POOLID)
		{
			SAMPLE_PRT("HI_MPI_VB_CreatePool failed! \n");
			return -1;
		}
	}

    VbBlk = HI_MPI_VB_GetBlock(m_hPool, u32Size, NULL);
    if (VB_INVALID_HANDLE == VbBlk)
    {
        SAMPLE_PRT("HI_MPI_VB_GetBlock err! size:%d\n",u32Size);
        return -1;
    }
    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u32PhyAddr)
    {
        return -1;
    }

    pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
	
    if (NULL == pVirAddr)
    {
		HI_MPI_VB_ReleaseBlock(VbBlk);
        return -1;
    }

    newframe.u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
    if (VB_INVALID_POOLID == newframe.u32PoolId)
    {
		HI_MPI_VB_ReleaseBlock(VbBlk);
        return -1;
    }
    SAMPLE_PRT("pool id :%d, phyAddr:%x,virAddr:%x\n" ,newframe.u32PoolId,u32PhyAddr,(int)pVirAddr);

    newframe.stVFrame.u32PhyAddr[0] = u32PhyAddr;
    newframe.stVFrame.u32PhyAddr[1] = newframe.stVFrame.u32PhyAddr[0] + u32LumaSize;
    newframe.stVFrame.u32PhyAddr[2] = newframe.stVFrame.u32PhyAddr[1] + u32ChrmSize;

    newframe.stVFrame.pVirAddr[0] = pVirAddr;
    newframe.stVFrame.pVirAddr[1] = newframe.stVFrame.pVirAddr[0] + u32LumaSize;
    newframe.stVFrame.pVirAddr[2] = newframe.stVFrame.pVirAddr[1] + u32ChrmSize;

    newframe.stVFrame.u32Width  = u32Width;
    newframe.stVFrame.u32Height = u32Height;
    newframe.stVFrame.u32Stride[0] = u32LStride;
    newframe.stVFrame.u32Stride[1] = u32CStride;
    newframe.stVFrame.u32Stride[2] = u32CStride;
    newframe.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    newframe.stVFrame.u32Field = VIDEO_FIELD_INTERLACED;
	
	unsigned char* pYuv = NULL;
	pYuv = (unsigned char*)HI_MPI_SYS_Mmap( oldframe.stVFrame.u32PhyAddr[0], u32Size );
	memcpy( newframe.stVFrame.pVirAddr[0], pYuv,  u32LumaSize);
	memcpy( newframe.stVFrame.pVirAddr[1], pYuv+u32LumaSize, u32ChrmSize );
	memcpy( newframe.stVFrame.pVirAddr[2], pYuv+u32LumaSize+u32ChrmSize, u32ChrmSize );
	
	char szTimeNow[1024]={0};
	GetDateTimeNow(szTimeNow );
	m_textonyuv.DrawText(szTimeNow, 64, 200, 400, 
				oldframe.stVFrame.u32Stride[0], 
				oldframe.stVFrame.u32Height,
			   (unsigned char*) pVirAddr );
	
	m_textonyuv.DrawText("大家好abcde", 64, 200, 460, 
		oldframe.stVFrame.u32Stride[0], 
		oldframe.stVFrame.u32Height,
		 (unsigned char*)pVirAddr );
	
	HI_MPI_SYS_Munmap( (void*)pYuv, u32Size );
    //HI_MPI_SYS_Munmap(pVirAddr, u32Size);
	HI_MPI_VB_ReleaseBlock(VbBlk);
	return 0;

}
*/

int CDecoder::CloneFrame(VIDEO_FRAME_INFO_S oldframe, VIDEO_FRAME_INFO_S &newframe )
{

	HI_U32             u32LStride;
    HI_U32             u32CStride;
    HI_U32             u32LumaSize;
    HI_U32             u32ChrmSize;
    HI_U32             u32Size;
	HI_U32             u32Stride;
	HI_U32 	           u32Height;
	HI_U32             u32Width;

	u32Height = oldframe.stVFrame.u32Height;
	u32Width = oldframe.stVFrame.u32Width;
	u32Stride = oldframe.stVFrame.u32Stride[0];
	u32LStride  = u32Stride;
    u32CStride  = u32Stride;

    u32LumaSize = (u32LStride * u32Height);
    u32ChrmSize = (u32CStride * u32Height) >> 2;
    u32Size = u32LumaSize + (u32ChrmSize << 1);
	u32Stride = oldframe.stVFrame.u32Stride[0];
	VB_BLK VbBlk;
	HI_U32 u32PhyAddr;
	HI_U8 *pVirAddr=NULL;



	if ( m_pFrame == NULL )
	{

		m_pFrame = new VIDEO_FRAME_INFO_S();
		if ( m_hPool == VB_INVALID_POOLID )
		{
			HI_U32 u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(VIDEO_ENCODING_MODE_NTSC,
						PIC_HD1080, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		
			m_hPool   = HI_MPI_VB_CreatePool( u32BlkSize, 10,NULL );
			if (m_hPool == VB_INVALID_POOLID)
			{
				SAMPLE_PRT("HI_MPI_VB_CreatePool failed! \n");
				return -1;
			}
		}

	    VbBlk = HI_MPI_VB_GetBlock(m_hPool, u32Size, NULL);
	    if (VB_INVALID_HANDLE == VbBlk)
	    {
	        SAMPLE_PRT("HI_MPI_VB_GetBlock err! size:%d\n",u32Size);
	        return -1;
	    }
	    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
	    if (0 == u32PhyAddr)
	    {
	        return -1;
	    }

	    

	    m_pFrame->u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
	    if (VB_INVALID_POOLID == m_pFrame->u32PoolId)
	    {
			HI_MPI_VB_ReleaseBlock(VbBlk);
	        return -1;
	    }
	    SAMPLE_PRT("pool id :%d, phyAddr:%x,virAddr:%x\n" ,m_pFrame->u32PoolId,u32PhyAddr,(int)pVirAddr);

		
		m_pFrame->stVFrame.u32PhyAddr[0] = u32PhyAddr;
		m_pFrame->stVFrame.u32PhyAddr[1] = m_pFrame->stVFrame.u32PhyAddr[0] + u32LumaSize;
		m_pFrame->stVFrame.u32PhyAddr[2] = m_pFrame->stVFrame.u32PhyAddr[1] + u32ChrmSize;


		pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
		m_pFrameBuf = pVirAddr;
		if (NULL == pVirAddr)
		{
			HI_MPI_VB_ReleaseBlock(VbBlk);
			return -1;
		}


		
		m_pFrame->stVFrame.pVirAddr[0] = pVirAddr;
		m_pFrame->stVFrame.pVirAddr[1] = (HI_VOID*)((char*)m_pFrame->stVFrame.pVirAddr[0] + u32LumaSize);
		m_pFrame->stVFrame.pVirAddr[2] = (HI_VOID*)((char*)m_pFrame->stVFrame.pVirAddr[1] + u32ChrmSize);
	
	}
	else
	{
		
		u32PhyAddr = m_pFrame->stVFrame.u32PhyAddr[0];
		pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
	}

	m_pFrame->stVFrame.u32Width  = u32Width;
    m_pFrame->stVFrame.u32Height = u32Height;
    m_pFrame->stVFrame.u32Stride[0] = u32LStride;
    m_pFrame->stVFrame.u32Stride[1] = u32CStride;
    m_pFrame->stVFrame.u32Stride[2] = u32CStride;
    m_pFrame->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    m_pFrame->stVFrame.u32Field = oldframe.stVFrame.u32Field;

	
    m_pFrame->stVFrame.u16OffsetTop = oldframe.stVFrame.u16OffsetTop;
    m_pFrame->stVFrame.u16OffsetBottom = oldframe.stVFrame.u16OffsetBottom;
    m_pFrame->stVFrame.u16OffsetLeft = oldframe.stVFrame.u16OffsetLeft;
    m_pFrame->stVFrame.u16OffsetRight = oldframe.stVFrame.u16OffsetRight;
    m_pFrame->stVFrame.u64pts = oldframe.stVFrame.u64pts;
    m_pFrame->stVFrame.u32TimeRef = oldframe.stVFrame.u32TimeRef;
    m_pFrame->stVFrame.u32PrivateData = oldframe.stVFrame.u32PrivateData;


	unsigned char* pYuv = NULL;
	pYuv = (unsigned char*)HI_MPI_SYS_Mmap( oldframe.stVFrame.u32PhyAddr[0], u32Size );
	memcpy( m_pFrame->stVFrame.pVirAddr[0], pYuv,  u32LumaSize);
	memcpy( m_pFrame->stVFrame.pVirAddr[1], pYuv+u32LumaSize, u32ChrmSize );
	memcpy( m_pFrame->stVFrame.pVirAddr[2], pYuv+u32LumaSize+u32ChrmSize, u32ChrmSize );

	/*
	char szTimeNow[1024]={0};
	GetDateTimeNow(szTimeNow );
	m_textonyuv.DrawText(szTimeNow, 64, 200, 400, 
				oldframe.stVFrame.u32Stride[0], 
				oldframe.stVFrame.u32Height,
			   (unsigned char*) m_pFrame->stVFrame.pVirAddr[0] );
	
	m_textonyuv.DrawText("大家好abcde", 64, 200, 460, 
		oldframe.stVFrame.u32Stride[0], 
		oldframe.stVFrame.u32Height,
		 (unsigned char*)m_pFrame->stVFrame.pVirAddr[0] );
		 */
	
	HI_MPI_SYS_Munmap( (void*)pYuv, u32Size );
    HI_MPI_SYS_Munmap(pVirAddr, u32Size);
	//HI_MPI_VB_ReleaseBlock(VbBlk);
	return 0;

}


void CDecoder::GetFrame()
{

	while( m_iExitFlag == 0 )
	{

		VIDEO_FRAME_INFO_S pstFrameInfo;
		VIDEO_FRAME_INFO_S newFrame;
		HI_U32 u32BlockFlag = HI_TRUE;
		HI_S32 s32Ret;
		s32Ret= HI_MPI_VDEC_GetImage_TimeOut( m_nDecChannelNo, &pstFrameInfo,  100 );

		//s32Ret = HI_MPI_VPSS_UserGetFrame( m_nDecChannelNo, 0, &pstFrameInfo );
		if ( s32Ret == HI_SUCCESS )
		{

			/*
			_DEBUG_( "frame info w:%d h:%d format:%d", pstFrameInfo.stVFrame.u32Width, 
				pstFrameInfo.stVFrame.u32Height, 
				pstFrameInfo.stVFrame.enPixelFormat );
			*/
		

			//CloneFrame( pstFrameInfo, newFrame );


				/*

				int nSize = pstFrameInfo.stVFrame.u32Stride[0]*pstFrameInfo.stVFrame.u32Height*3/2;
				unsigned char* pYuv = NULL;
				pYuv = (unsigned char*)HI_MPI_SYS_Mmap( pstFrameInfo.stVFrame.u32PhyAddr[0], nSize );

				//memcpy( m_pFrameBuf, pYuv, nSize );
				//HI_MPI_SYS_MmzFlushCache( pstFrameInfo.stVFrame.u32PhyAddr[0], pYuv, nSize ); 
				
				char szTimeNow[1024]={0};
				GetDateTimeNow(szTimeNow );


				int iFont = 32;

				if ( pstFrameInfo.stVFrame.u32Width > 1280 )
				{
					iFont = 64;

				}
				else if ( pstFrameInfo.stVFrame.u32Width < 704 )
				{
					
					iFont = 16;
				}
				
				m_textonyuv.DrawText(szTimeNow, iFont, 200, 400, 
							pstFrameInfo.stVFrame.u32Stride[0], 
							pstFrameInfo.stVFrame.u32Height,
						   (unsigned char*) pYuv );
				
				m_textonyuv.DrawText("大家好ab", iFont, 200, 460, 
					pstFrameInfo.stVFrame.u32Stride[0], 
					pstFrameInfo.stVFrame.u32Height,
				     (unsigned char*)pYuv );

				
				HI_MPI_SYS_Munmap( (void*)pYuv, nSize );
				*/
			//memcpy( pYuv, m_pFrameBuf, nSize );
		
			//usleep(1);

		//	VIDEO_FRAME_INFO_S newframe;
		//	memcpy( &newframe, &pstFrameInfo, sizeof( VIDEO_FRAME_INFO_S ) );

		//	newframe.stVFrame.u32PhyAddr[0] = (HI_U32)m_pFrameBuf;

			s32Ret = HI_MPI_VPSS_UserSendFrame( m_nDecChannelNo, &pstFrameInfo);
			//_DEBUG_("send frame ret:0x%x", s32Ret );
			HI_MPI_VDEC_ReleaseImage( m_nDecChannelNo, &pstFrameInfo);
			//HI_MPI_VDEC_ReleaseImage( m_nDecChannelNo, &newFrame );
		}
		else 
		{
			//_DEBUG_("get image error:0x%x", s32Ret );
		}
		
			
	}
}



void CDecoder::DecodeData(  unsigned char * pData, int nLen )
{
	//TEST
	/*static int icount=0;
	icount++;
	if(icount==100)
	{
		_DEBUG_("############# recved data [%d]", nLen);
		icount=0;
	}*/
	static FILE* fp_ts = NULL;
	if ( fp_ts == NULL)
	{
		//fp_ts=fopen("/opt/dws/ts_video.ts", "wb"); 
		//_DEBUG_("----------------------------fopen_ts");
	}
    static FILE* fp_es = NULL;
    if ( fp_es == NULL)
    {
        //fp_es=fopen("/opt/dws/es_video.es", "wb");
        //_DEBUG_("----------------------------fopen_es");
    }
    if ( fp_ts)
    {
          fwrite((char*)pData,  1, nLen, fp_ts);
          fflush(fp_ts);
    }

	//_DEBUG_("chan:%d len:%d 0x%x\n", m_nDecChannelNo, nLen, (int)this );
	if ( m_nInited != 1 ) return;

	//pthread_mutex_lock(&m_lock);

	if ( m_nCurIndex + nLen > 1024 * 1024 )
	{
		_DEBUG_( "drop data! buff len=%d > %d\n", m_nCurIndex+nLen, 1024*1024);
		m_nCurIndex = 0;
	}

	memcpy( m_pBuf + m_nCurIndex, pData, nLen );
	m_nCurIndex += nLen;

	if ( m_nCurIndex < 2*1024)
	{
		//pthread_mutex_unlock(&m_lock);
		//printf("cp 1 size:%d\n", m_nCurIndex);
		return;
	}
	//_DEBUG_("recv data:%d\n", nLen);


	time_t tmnow;
	tmnow= time(&tmnow);

	if ( tmnow - m_lastStat >= 10 )
	{
		//_DEBUG_("decode byte rate:%d KB payload type:%d", m_nDecBytes / (1024*( tmnow-m_lastStat)), 
		//	(int)m_nPayloadType );
		m_lastStat = tmnow;
		m_nDecBytes = 0;
	}
	
	int nQboxLen = 0; 
	int nQboxRet = 0;
	int nNextDataLen = 0;
	if(m_nFormat != MPEG2_VIDEO_STREAM_TYPE)
	{
		nQboxRet = RemoveQbox( m_pBuf, m_nCurIndex, nQboxLen );

		//printf( "nQboxLen:%d ret:%d\n", nQboxLen, nQboxRet );
		if (nQboxRet >= 0 && nQboxLen == 0 ) 
		{
			//printf("buffer ...\n");
			//pthread_mutex_unlock(&m_lock);
			//printf("cp 2\n");

			//if ( m_isTS == 0 && m_isPS == 0 ) 
			//###########################################################
	        //针对mepg码流时打开，其他注释掉下2行代码
	        //_DEBUG_("nQboxRet >=0 && nQboxLen==0! m_nCurIndex=%d", m_nCurIndex);
	        //m_nCurIndex=0;
	        //###########################################################
			return;
		}


	}
	
	if ( nQboxLen > 0 )
	{
		nNextDataLen = nQboxLen;
//		fwrite( m_pBuf, nQboxLen, 1, fes ); 
	}
	else
	{
		nNextDataLen = m_nCurIndex;
	}
	//RemoveQbox();

	//printf( "input ts\n");
	m_pTs2Es->InputTSData( m_pBuf, nNextDataLen, false );

	byte *pbESData = NULL;
	int iESDataLen = 0;

	byte *pbAudioData = NULL;
	int iAudioDataLen = 0;

	int nTotalLen = 0;
	uint64_t nOutSCRTime =0;
	int iFlag = 0;
	int eVideoCodecType=0;
	int eAudioCodecType = 0;
	//printf("cp10\n");
	while( 1 )
	{
	    if(m_pTs2Es->GetESData(pbESData, 
							iESDataLen,  
							pbAudioData, 
							iAudioDataLen, 
							nOutSCRTime, 
							eVideoCodecType, 
							eAudioCodecType ) < 0)
        {   
        	//_DEBUG_("GetESData failed, break!");
			break;
        }
		//_DEBUG_("GetESData ok, continue!");
		m_isTS++;
		if ( m_isTS <= 0 )
		{
			m_isTS = 1;
		}
		//printf( "iesdatalen:%d audio:%d video format:%d audio format:%d last format:%d\n", 
		//	iESDataLen, iAudioDataLen, (int)eVideoCodecType, eAudioCodecType, m_nLastFormat);

		if ( iESDataLen > 0 )
		{
			if ( eVideoCodecType != (int)UNKNOWN_VIDEO  )
			{
				//printf("########### eVideoCodecType = %d\n",eVideoCodecType);
				if ( eVideoCodecType != m_nLastFormat   )
				{
					m_nContinus = 0;
					_DEBUG_("last %d, video type %d, m_nContinus=%d\n", m_nLastFormat, eVideoCodecType,m_nContinus);
				}

				m_nLastFormat = eVideoCodecType;
				m_nContinus++;

				if ( m_nContinus >= 10000000 )
				{
					m_nContinus = 100;
				}
			}
			else//test20150121
			{
				_DEBUG_("UNKNOWN_VIDEO.\n");
			}
			//_DEBUG_("beforce changevdec m_nContinus=%d",m_nContinus);
			ChangeVdec();
			if ( m_nLastFormat == m_nFormat )
			{
				VDEC_STREAM_S stStream;
				stStream.u64PTS = 0;
				stStream.pu8Addr =(HI_U8*) pbESData + 0;
				stStream.u32Len = iESDataLen;
                //TEST
                if ( fp_es)
                {
                      fwrite((char*)pbESData,  1, iESDataLen, fp_es);
                      fflush(fp_es);
                }
				HI_U32 s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
				m_nDecBytes += iESDataLen;
				//printf("ok\n");

			}
			else//test20150121
			{
				_DEBUG_("format not equal[last %d != %d now], m_nContinus=%d!\n", m_nLastFormat, m_nFormat,m_nContinus);
			}
			//printf( "find ts video. len:%d format:%d\n", iESDataLen, eVideoCodecType );
		}
		//iAudioDataLen = 0;
		if ( iAudioDataLen > 0 )
		{

			//SendAudioFrame( pbAudioData, iAudioDataLen);
			/*
			if ( iAudioDataLen + m_nAudioIndex > (SAMPLE_AUDIO_PTNUMPERFRM )*10  )
			{
				m_nAudioIndex = 0;
			}
			else
			{
				memcpy( m_pAudioBuf+m_nAudioIndex, pbAudioData, iAudioDataLen );
				m_nAudioIndex += iAudioDataLen;
			}

			while ( m_nAudioIndex >= (SAMPLE_AUDIO_PTNUMPERFRM ) )
			{
				//printf( "send audio.\n" );
				SendAudioFrame( m_pAudioBuf, SAMPLE_AUDIO_PTNUMPERFRM );
				m_nAudioIndex -= (SAMPLE_AUDIO_PTNUMPERFRM );
				if (m_nAudioIndex > 0 )
				{
					memmove( m_pAudioBuf, m_pAudioBuf + (SAMPLE_AUDIO_PTNUMPERFRM), m_nAudioIndex );
				}
				else
				{
					m_nAudioIndex = 0;
				}
			}
			*/
			
		}

		iFlag = 1;
	}

	//printf("cp20\n");

	//printf("input ts ok\n");
	if ( iFlag == 1 )
	{

		//printf("find ts packet\n");
		if (nQboxLen > 0 )
		{
			m_nCurIndex -= nQboxLen;
			memmove( m_pBuf, m_pBuf+nQboxLen, m_nCurIndex );
            //_DEBUG_("Flag==1! m_nCurIndex=%d", m_nCurIndex);
		}
		else
		{
			m_nCurIndex = 0;
		}
		//pthread_mutex_unlock(&m_lock);
		return;
	}
    else
    {
        //_DEBUG_("Flag==0! m_nCurIndex=%d", m_nCurIndex);
    }

	//printf( "input ps\n");
	//if ( m_isTS > 0 ) return;

	if ( AVC_VIDEO_STREAM_TYPE != m_nLastFormat )
	{
		m_nContinus = 0;
		_DEBUG_("m_nConinus=%d, last=%d", m_nContinus, m_nLastFormat);
	}

	m_nLastFormat = AVC_VIDEO_STREAM_TYPE;//AVC_VIDEO_STREAM_TYPE;
	m_nContinus++;
	if ( m_nContinus >= 10000000 )
	{
		m_nContinus = 100;
	}

	ChangeVdec();

	/*

	m_pPs2Es->InputPSData(m_pBuf, nNextDataLen, false  );

	pbESData = NULL;
	iESDataLen = 0;
	bool bIsVideoStream = false;

	nTotalLen = 0;
	nOutSCRTime =0;
	iFlag = 0;
	
	while( m_pPs2Es->GetESData(pbESData, iESDataLen,  pbAudioData, iAudioDataLen, nOutSCRTime ) >= 0 )
	{
		
		printf( "find ps video %d. len:%d\n", bIsVideoStream, iESDataLen);
		if (  iESDataLen > 0 && pbESData != NULL )
		{
			
			VDEC_STREAM_S stStream;
			stStream.u64PTS = 0;
			stStream.pu8Addr =(HI_U8*) pbESData + 0;
			stStream.u32Len = iESDataLen;
			//printf("send stream 2 ");
			m_nDecBytes += iESDataLen;
			HI_U32 s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
			//printf("ok\n");
			//printf( "find ps video. len:%d\n", iESDataLen );
			iFlag = 1;
		}

		if (  iAudioDataLen > 0 && pbAudioData != NULL )
		{
			AUDIO_STREAM_S stAudioStream;
			stAudioStream.pStream = pbAudioData;
			stAudioStream.u32Len = iAudioDataLen;
			HI_MPI_ADEC_SendStream(m_nDecChannelNo, &stAudioStream, HI_IO_NOBLOCK);
		}
	}

	if ( iFlag == 1 )
	{
		if (nQboxLen > 0 )
		{
			m_nCurIndex -= nQboxLen;
			memmove( m_pBuf, m_pBuf+nQboxLen, m_nCurIndex );
		}
		else
		{
			m_nCurIndex = 0;
		}
		//pthread_mutex_unlock(&m_lock);
		return;
	}*/

	//printf( "send stream\n");
	//_DEBUG_("##############################");
	VDEC_STREAM_S stStream;
	stStream.u64PTS = 0;
	stStream.pu8Addr =(HI_U8*) m_pBuf + 0;
	stStream.u32Len = nNextDataLen;
	//printf("send stream 2 ");
	m_nDecBytes += nNextDataLen;
	HI_U32 s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
	//printf("ok\n ");
	m_nCurIndex -= nNextDataLen;
	if ( m_nCurIndex > 0 )
	{
		memmove( m_pBuf, m_pBuf+nNextDataLen, m_nCurIndex );
	}
	//printf("send %d remain:%d\n", nNextDataLen, m_nCurIndex );
	//pthread_mutex_unlock(&m_lock);
	return;
	
	

}


/******************************************************************************
* function : create vdec chn
******************************************************************************/
HI_S32 CDecoder::VDEC_CreateVdecChn(HI_S32 s32ChnID, SIZE_S *pstSize, PAYLOAD_TYPE_E enType, VIDEO_MODE_E enVdecMode)
{
    VDEC_CHN_ATTR_S stAttr;
    VDEC_PRTCL_PARAM_S stPrtclParam;
    HI_S32 s32Ret;

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));

    stAttr.enType = enType;
    stAttr.u32BufSize = pstSize->u32Height * pstSize->u32Width;//This item should larger than u32Width*u32Height/2
    stAttr.u32Priority = 1;//此处必须大于0
    stAttr.u32PicWidth = pstSize->u32Width;
    stAttr.u32PicHeight = pstSize->u32Height;
    switch (enType)
    {
        case PT_H264:
		case PT_MPEG2TS:
	    stAttr.stVdecVideoAttr.u32RefFrameNum = 5;
	    stAttr.stVdecVideoAttr.enMode = enVdecMode;
	    stAttr.stVdecVideoAttr.s32SupportBFrame = 1;
            break;
		case PT_MPEGVIDEO:
		case PT_MPEG2VIDEO:	
		stAttr.u32PicWidth  = 720;
		stAttr.u32PicHeight	= 576;		
	    stAttr.stVdecVideoAttr.u32RefFrameNum = 5;
	    stAttr.stVdecVideoAttr.enMode = enVdecMode;
	    stAttr.stVdecVideoAttr.s32SupportBFrame = 1;
            break;
			
        case PT_JPEG:
            stAttr.stVdecJpegAttr.enMode = enVdecMode;
            break;
        case PT_MJPEG:
            stAttr.stVdecJpegAttr.enMode = enVdecMode;
            break;
        default:
            SAMPLE_PRT("err type \n");
            return HI_FAILURE;
    }

    s32Ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VDEC_GetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_GetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    stPrtclParam.s32MaxSpsNum = 21;
    stPrtclParam.s32MaxPpsNum = 22;
    stPrtclParam.s32MaxSliceNum = 100;
    s32Ret = HI_MPI_VDEC_SetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_SetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : force to stop decoder and destroy channel.
*            stream left in decoder will not be decoded.
******************************************************************************/
void CDecoder::VDEC_ForceDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }

    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }
}

/******************************************************************************
* function : wait for decoder finished and destroy channel.
*            Stream left in decoder will be decoded.
******************************************************************************/
void CDecoder::VDEC_WaitDestroyVdecChn(HI_S32 s32ChnID, VIDEO_MODE_E enVdecMode)
{
    HI_S32 s32Ret;
    VDEC_CHN_STAT_S stStat;

    memset(&stStat, 0, sizeof(VDEC_CHN_STAT_S));

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
       // return;
    }

    /*** wait destory ONLY used at frame mode! ***/
    if (VIDEO_MODE_FRAME == enVdecMode)
    {
        while (1)
        {
            //printf("LeftPics:%d, LeftStreamFrames:%d\n", stStat.u32LeftPics,stStat.u32LeftStreamFrames);
            usleep(40000);
            s32Ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VDEC_Query failed errno 0x%x \n", s32Ret);
                return;
            }
            if ((stStat.u32LeftPics == 0) && (stStat.u32LeftStreamFrames == 0))
            {
                printf("had no stream and pic left\n");
                break;
            }
        }
    }
    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
        return;
    }
}


/*
HI_S32 CDecoder::CreateAudioDec()
{

	AIO_ATTR_S stAioAttr ;
	memset( &stAioAttr, 0, sizeof( AIO_ATTR_S ));
	
	stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.u32ClkSel = 1;
	stAioAttr.u32ChnCnt = 16;
	
	
    HI_S32 s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr, false );
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    int AdChn = 0;
    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, m_enAudPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, AoChn, pstAioAttr, gs_pstAoReSmpAttr);
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

    pfd = SAMPLE_AUDIO_OpenAdecFile(AdChn, gs_enPayloadType);
    if (!pfd)
    {
        SAMPLE_DBG(HI_FAILURE);
        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdFileAdec(AdChn, pfd);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
    
    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");

    //SAMPLE_COMM_AUDIO_StopAo(AoDev, AoChn, gs_bAioReSample);
   // SAMPLE_COMM_AUDIO_StopAdec(AdChn);
  //  SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);

}
*/


HI_S32 CDecoder::UnInitVdec()
{
	printf("unint vdec\n");
	VDEC_WaitDestroyVdecChn( m_nDecChannelNo, VIDEO_MODE_STREAM );
	int VpssGrp = m_nDecChannelNo;
	SAMLE_COMM_VDEC_UnBindVpss( m_nDecChannelNo, VpssGrp);
	return 0;
}


HI_S32 CDecoder::InitVdec()
{
	/******************************************
	   step 5: start vdec & bind it to vpss or vo
	  ******************************************/ 

    SIZE_S stSize;
	HI_S32 s32Ret = SAMPLE_COMM_SYS_GetPicSize( VIDEO_ENCODING_MODE_PAL, PIC_WUXGA, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return -1;
    }

    /*** create vdec chn ***/
    int VdChn = m_nDecChannelNo;
	
    s32Ret = VDEC_CreateVdecChn(VdChn, &stSize, m_nPayloadType, VIDEO_MODE_STREAM);
    if (HI_SUCCESS !=s32Ret)
    {
  	  SAMPLE_PRT("create vdec chn failed! 0x%x\n", s32Ret);
  	  //return HI_FAILURE;
    }
 
     /*** bind vdec to vpss ***/
  	int VpssGrp = VdChn;
    s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
    if (HI_SUCCESS !=s32Ret)
    {
  	  SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
  	 // return HI_FAILURE;
    }
    // 重新刷新OSD屏幕
    //m_bFirst1=m_bFirst2=true;
	return s32Ret;
}

void CDecoder::StartDecoder()
{
	//pthread_mutex_lock(&m_lock);

	
	
	m_nCurIndex = 0;
	if ( m_pTs2Es != NULL )
	{
		delete m_pTs2Es;
		m_pTs2Es = NULL;
	}

	m_isTS = 0;
	m_isTSDiscontinue = 0;
	m_isQbox = 0;
	m_isPS = 0;
	m_isES = 0;
	m_pTs2Es = new CTS2ES();

	//m_pTs2Es->Init();
	m_nContinus = 0;
	
	if ( m_iDecFlag == 2 )
	{
		UnInitVInput1();
	}
	
	if ( m_iDecFlag != 2 )
	{
		printf("start decoder\n");
		
		UnInitVdec();
		InitVdec();

		m_nDecBytes = 0;
		time_t tm;
		time(&tm);
		m_lastStat = tm;
	}
	//pthread_mutex_unlock(&m_lock);
	m_iDecFlag = 1;
}


void CDecoder::StartCVBS()
{
	if ( m_iDecFlag == 1 )
	{
		UnInitVdec();
	}

	if ( m_iDecFlag != 1 )
	{
		printf( "start cvbs\n");
		UnInitVInput1();
		InitVInput1();
	}

	m_iDecFlag = 2;
}

HI_S32 CDecoder::InitVInput1()
{
  
   return InitVI();
}	

HI_S32 CDecoder::UnInitVInput1()
{
	return UninitVI();
}




