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

#define BUF_SIZE 1 * 1024 * 1024
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
	m_nLastFormat = PT_H264;
	m_nFormat = H264_VIDEO;

	m_isTSDiscontinue = 0;

	m_iDecFlag = 0;
	//StartDecoder();
	m_nInited = 1;

	m_pFrame = NULL;
	m_hPool = VB_INVALID_POOLID;
	m_iFrameIndex = 0;
 
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
	//pthread_create( &m_getframeth,NULL, GetFrameProc, (void*)this);
	pthread_mutex_init(&m_lock, NULL);
	//fes = fopen( "test.au", "w" );
    //_DEBUG_("################################## fes [0x%x]",fes);	
}


CDecoder::~CDecoder()
{
    //_DEBUG_("~CDecoder start");
    if(fes!=NULL)
    {
        //_DEBUG_("close fes.");
	    //fclose( fes );
    }

	m_iExitFlag = 1;
    //_DEBUG_("join m_getframeth");
	if ( m_getframeth != 0 )
	{
		pthread_join( m_getframeth, NULL );
		m_getframeth = 0;
        //_DEBUG_("free m_pFrameBuf");
		PTR_FREE( m_pFrameBuf );
		
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

	pthread_mutex_destroy( &m_lock );
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
			//_DEBUG_("bFirstFind=%d, i=%d, m_nCurIndex=%d", bFirstFind, i, m_nCurIndex);
			if (bFirstFind)
			{
				int nHeaderLen = (VISS_HEADER_SIZE + QBOX_HEADER_SIZE);
				lFrameSize -= nHeaderLen;
				memmove(pbFrameData, (pbFrameData + nHeaderLen), lFrameSize);
				//_DEBUG_("move qbox&viss header, memmove len=%d, buffer len=%d", nHeaderLen, lFrameSize);
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
	if ( m_nLastFormat != m_nFormat 
		&& m_nContinus >= 50 )
	{
		UnInitVdec();
		//_DEBUG_("before change vdec, m_nContinus=%d, m_nLastFormat=%d, m_nFormat=%d",m_nContinus, m_nLastFormat, m_nFormat);
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
		//_DEBUG_("after change vdec, m_nContinus=%d, m_nLastFormat=%d, m_nFormat=%d",m_nContinus, m_nLastFormat, m_nFormat);
	}
    /*
	else
	{
		_DEBUG_("m_nLastFormat=%d, m_nFormat=%d, m_nContinus=%d",  m_nLastFormat, m_nFormat, m_nContinus);
	}
	*/
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
    /*
    // test code
	static FILE* fp_ts = NULL;
	if ( fp_ts == NULL)
	{
		fp_ts=fopen("/opt/dws/ts_video.ts", "wb"); 
		_DEBUG_("----------------------------fopen_ts");
	}
    static FILE* fp_es = NULL;
    if ( fp_es == NULL)
    {
        fp_es=fopen("/opt/dws/es_video.es", "wb");
        _DEBUG_("----------------------------fopen_es");
    }
    if ( fp_ts)
    {
          fwrite((char*)pData,  1, nLen, fp_ts);
          fflush(fp_ts);
    }
    */
	//_DEBUG_("chan:%d len:%d 0x%x\n", m_nDecChannelNo, nLen, (int)this );
	HI_U32 s32Ret=0;
	if ( m_nInited != 1 ) return;

	//pthread_mutex_lock(&m_lock);

	if ( m_nCurIndex + nLen > BUF_SIZE )
	{
		_DEBUG_( "drop data! buff len=%d > %d\n", m_nCurIndex+nLen, BUF_SIZE);
		m_nCurIndex = 0;
	}

	memcpy( m_pBuf + m_nCurIndex, pData, nLen );
	m_nCurIndex += nLen;
    //_DEBUG_("#########Current Total Size: %d, Input Data Size: %d", m_nCurIndex, nLen);
	unsigned int uiValue = 0;
	if(m_nLastFormat==27) // h264
	{
		uiValue = 2*1024;
	}
	else
	{
		uiValue = 2*1024; 
	}
	//_DEBUG_("uiValue=%d",uiValue);
	if ( m_nCurIndex < uiValue)
	{
		//pthread_mutex_unlock(&m_lock);
		//printf("cp 1 size:%d\n", m_nCurIndex);
		//_DEBUG_("get data=%d, m_nCurIndex=%d<20*1024, return",nLen, m_nCurIndex);
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
    
	nQboxRet = RemoveQbox( m_pBuf, m_nCurIndex, nQboxLen );

	//_DEBUG_( "nQboxLen:%d ret:%d\n", nQboxLen, nQboxRet );
	if (nQboxRet >= 0 && nQboxLen == 0 ) 
	{
		//printf("buffer ...\n");
		//pthread_mutex_unlock(&m_lock);
		//printf("cp 2\n");

		/*
		if ( m_isTS == 0 && m_isPS == 0 ) 
        {
            _DEBUG_("m_isTS==0 && m_isPS==0 return!!!!!!!!!!!!!!!!!!!!!!!!!");
            //return;
		}
		*/
        return;
		//###########################################################
        //针对mepg码流时打开，其他注释掉代码
        //_DEBUG_("nQboxRet >=0 && nQboxLen==0! m_nCurIndex=%d", m_nCurIndex);
        //m_nCurIndex=0;
        //return;
        //###########################################################
		
	}
	
	if ( nQboxLen > 0 )
	{
		nNextDataLen = nQboxLen;
        //fwrite( m_pBuf, nQboxLen, 1, fes ); 
	}
	else
	{
		nNextDataLen = m_nCurIndex;
	}
	//RemoveQbox();

	//printf( "input ts\n");
	m_pTs2Es->InputTSData( m_pBuf, nNextDataLen, false );
    
    VDEC_STREAM_S stStream;
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
		//_DEBUG_( "iesdatalen:%d audio:%d video format:%d audio format:%d last format:%d\n", 
		//	iESDataLen, iAudioDataLen, (int)eVideoCodecType, eAudioCodecType, m_nLastFormat);

		if ( iESDataLen > 0 )
		{
			if ( eVideoCodecType != (int)UNKNOWN_VIDEO  )
			{
				//_DEBUG_("########### eVideoCodecType = %d\n",eVideoCodecType);
				if ( eVideoCodecType != m_nLastFormat   )
				{
					m_nContinus = 0;
					//_DEBUG_("last %d, video type %d, m_nContinus=%d\n", m_nLastFormat, eVideoCodecType,m_nContinus);
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
				//_DEBUG_("UNKNOWN_VIDEO.\n");
			}
			//_DEBUG_("beforce changevdec m_nContinus=%d",m_nContinus);
			ChangeVdec();
			if ( m_nLastFormat == m_nFormat )
			{
				stStream.u64PTS = 0;
				stStream.pu8Addr =(HI_U8*) pbESData + 0;
				stStream.u32Len = iESDataLen;
                /*
                //TEST
                if ( fp_es)
                {
                      fwrite((char*)pbESData,  1, iESDataLen, fp_es);
                      fflush(fp_es);
                }
                */
				s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
				m_nDecBytes += iESDataLen;
				//printf("ok\n");

			}
            /*
			else//test20150121
			{
				_DEBUG_("format not equal[last %d != %d now], m_nContinus=%d!\n", m_nLastFormat, m_nFormat,m_nContinus);
			}
			*/
			//printf( "find ts video. len:%d format:%d\n", iESDataLen, eVideoCodecType );
		}
		//iAudioDataLen = 0;
		if ( iAudioDataLen > 0 )
		{
			//_DEBUG_("iAudioDataLen > 0");
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
		//_DEBUG_("iFlag == 1, nQboxLen=%d, return", nQboxLen);
		return;
	}
    else
    {
        //_DEBUG_("Flag==0! m_nCurIndex=%d", m_nCurIndex);
    }

	//printf( "input ps\n");
	//if ( m_isTS > 0 ) return;

	if ( H264_VIDEO != m_nLastFormat )
	{
		m_nContinus = 0;
		//_DEBUG_("m_nConinus=%d, last=%d", m_nContinus, m_nLastFormat);
	}

	m_nLastFormat = H264_VIDEO;//AVC_VIDEO_STREAM_TYPE;
	m_nContinus++;
	//_DEBUG_("###############m_nContinus=%d",m_nContinus);
	if ( m_nContinus >= 10000000 )
	{
		m_nContinus = 100;
	}

	ChangeVdec();


	m_pPs2Es->InputPSData(m_pBuf, nNextDataLen, false);
	pbESData = NULL;
	iESDataLen = 0;
	nTotalLen = 0;
	nOutSCRTime =0;
	iFlag = 0;
	while( m_pPs2Es->GetESData(pbESData, iESDataLen,  pbAudioData, iAudioDataLen, nOutSCRTime ) >= 0 )
	{
        //_DEBUG_("###################\n Buffer=%d, ES data=%d", m_nCurIndex, iESDataLen);
		if (  iESDataLen > 0 && pbESData != NULL )
		{
			stStream.u64PTS = 0;
			stStream.pu8Addr =(HI_U8*) pbESData + 0;
			stStream.u32Len = iESDataLen;
			//printf("send stream 2 ");
			s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
            m_nDecBytes += iESDataLen;
            iFlag = 1;
            /*
            //TEST
            if ( fp_es)
            {
                  fwrite((char*)pbESData,  1, iESDataLen, fp_es);
                  fflush(fp_es);
            }
            */
			//printf("ok\n");
			//printf( "find ps video. len:%d\n", iESDataLen );
			
		}
        /* 音频解码 // commented by wdd,20151102
		if (  iAudioDataLen > 0 && pbAudioData != NULL )
		{
			AUDIO_STREAM_S stAudioStream;
			stAudioStream.pStream = pbAudioData;
			stAudioStream.u32Len = iAudioDataLen;
			HI_MPI_ADEC_SendStream(m_nDecChannelNo, &stAudioStream, HI_IO_NOBLOCK);
		}
		*/
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
	}

	//printf( "send stream\n");
	//_DEBUG_("##############################");
	//_DEBUG_("HI_MPI_VDEC_SendStream");
	stStream.u64PTS = 0;
	stStream.pu8Addr =(HI_U8*) m_pBuf + 0;
	stStream.u32Len = nNextDataLen;
	//printf("send stream 2 ");
	m_nDecBytes += nNextDataLen;
	s32Ret=HI_MPI_VDEC_SendStream( m_nDecChannelNo, &stStream, HI_IO_NOBLOCK);
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
	_DEBUG_("unint vdec\n");
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




