#include "mytde.h"
#include "Text2Bmp.h"
#include "loadbmp.h"
#include "TextOnYUV.h"

long MyTest2()
{
    HI_U32 u32Size;
    HI_S32 s32Fd;
    HI_U32 u32Times;
    HI_U8* pu8Screen;
    HI_U8* pu8BackGroundVir;

    HI_U32 u32PhyAddr;
    HI_S32 s32Ret = -1;
    HI_U32 i = 0;
    HI_BOOL bShow;
    HIFB_ALPHA_S stAlpha;
    static TDE2_SURFACE_S g_stScreen[2];
    static TDE2_SURFACE_S g_stBackGround;
    memset(&stAlpha, 0, sizeof(HIFB_ALPHA_S));

    
    struct fb_fix_screeninfo stFixInfo;
    struct fb_var_screeninfo stVarInfo;
    struct fb_bitfield stR32 = {10, 5, 0};
    struct fb_bitfield stG32 = {5, 5, 0};
    struct fb_bitfield stB32 = {0, 5, 0};
    struct fb_bitfield stA32 = {15, 1, 0};
    TDE_HANDLE s32Handle;
    TDE2_OPT_S stOpt;
    memset(&stOpt, 0, sizeof(TDE2_OPT_S));
    TDE2_RECT_S stSrcRect;
    TDE2_RECT_S stDstRect;
    CText2Bmp text2bmp;
	int nWidth = 0;;
	int nHeight = 0;
	static unsigned char *pBmp=NULL;
    if(pBmp!=NULL)
    {
        delete [] pBmp;
        pBmp = NULL;
    }
    char szText[64] = {0};
    sprintf(szText, "%s", "IPC-Test++--#*()!!1234:?..._|A魑魅魍魉凤凰盘涅");

    /* 1. open tde device */
    HI_TDE2_Open();

    /* 2. framebuffer operation */
    s32Fd = open("/dev/fb1", O_RDWR);
    if (s32Fd == -1)
    {
        printf("open frame buffer device error\n");
        goto FB_OPEN_ERROR;
    }
    
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.bAlphaEnable = HI_FALSE;
    if (ioctl(s32Fd, FBIOPUT_ALPHA_HIFB, &stAlpha) < 0)
    {
        printf("Put alpha info failed!\n");
        goto FB_PROCESS_ERROR0;
    }

    /* get the variable screen info */
    if (ioctl(s32Fd, FBIOGET_VSCREENINFO, &stVarInfo) < 0)
    {
        printf("Get variable screen info failed!\n");
        goto FB_PROCESS_ERROR0;
    }

	text2bmp.GetBitmap( szText, 32, nWidth, nHeight, pBmp);

    stVarInfo.xres_virtual      = nWidth;
    stVarInfo.yres_virtual      = nHeight*2;
    stVarInfo.xres              = nWidth;
    stVarInfo.yres              = nHeight*2;
    stVarInfo.activate          = FB_ACTIVATE_NOW;
    stVarInfo.bits_per_pixel    = 16;
    stVarInfo.xoffset = 0;
    stVarInfo.yoffset = 0;
    stVarInfo.red   = stR32;
    stVarInfo.green = stG32;
    stVarInfo.blue  = stB32;
    stVarInfo.transp = stA32;

    if (ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVarInfo) < 0)
    {
        printf("process frame buffer device error1111\n");
        goto FB_PROCESS_ERROR0;
    }

    if (ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFixInfo) < 0)
    {
        printf("process frame buffer device error2222\n");
        goto FB_PROCESS_ERROR0;
    }
    
    u32Size     = stFixInfo.smem_len;
    u32PhyAddr  = (HI_U32)stFixInfo.smem_start;
    pu8Screen   = (HI_U8*)mmap(0, u32Size, PROT_READ|PROT_WRITE, MAP_SHARED, s32Fd, 0);
    if (NULL == pu8Screen)
    {
        printf("mmap fb0 failed!\n");
        goto FB_PROCESS_ERROR0;
    }
    memset(pu8Screen, 0x00, stFixInfo.smem_len);

    /* 3. create surface */
    g_stScreen[0].enColorFmt = TDE2_COLOR_FMT_ARGB1555;
    g_stScreen[0].u32PhyAddr = u32PhyAddr;
    g_stScreen[0].u32Width = nWidth;
    g_stScreen[0].u32Height = nHeight;
    g_stScreen[0].u32Stride = nWidth*2;
    g_stScreen[0].bAlphaMax255 = HI_TRUE;
    
    g_stScreen[1] = g_stScreen[0];
    g_stScreen[1].u32PhyAddr = g_stScreen[0].u32PhyAddr + g_stScreen[0].u32Stride * g_stScreen[0].u32Height;
    if (HI_FAILURE == HI_MPI_SYS_MmzAlloc(&(g_stBackGround.u32PhyAddr), ((void**)&pu8BackGroundVir), 
            NULL, NULL, nWidth*nHeight*2))
    {
        TDE_PRINT("allocate memory  failed\n");
        goto FB_PROCESS_ERROR1;
    }
    memset(pu8BackGroundVir, 0x00, nWidth*nHeight*2);
    //TDE_CreateSurfaceByFile(BACKGROUND_NAME, &g_stBackGround, pu8BackGroundVir);
    g_stBackGround.enColorFmt = (TDE2_COLOR_FMT_E)TDE2_COLOR_FMT_ARGB1555;
    g_stBackGround.u32Width = nWidth;
    g_stBackGround.u32Height = nHeight;
    g_stBackGround.u32Stride = 2*nWidth;
    g_stBackGround.u8Alpha0 = 0xff;
    g_stBackGround.u8Alpha1 = 0xff;
    g_stBackGround.bAlphaMax255 = HI_TRUE;
    g_stBackGround.bAlphaExt1555 = HI_TRUE;
    memcpy(pu8BackGroundVir, pBmp, nHeight*nWidth*2);

    bShow = HI_TRUE;
    if (ioctl(s32Fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        fprintf (stderr, "Couldn't show fb\n");
        goto FB_PROCESS_ERROR2;
    }
    
    stOpt.enOutAlphaFrom = TDE2_OUTALPHA_FROM_FOREGROUND;
    stOpt.enColorKeyMode = TDE2_COLORKEY_MODE_FOREGROUND;
    stOpt.unColorKeyValue.struCkARGB.stAlpha.bCompIgnore = HI_TRUE;
    stSrcRect.s32Xpos = 0;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = g_stBackGround.u32Width;
    stSrcRect.u32Height = g_stBackGround.u32Height;
    
    stDstRect.s32Xpos = 0;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = g_stBackGround.u32Width;
    stDstRect.u32Height = g_stBackGround.u32Height;

    /* 1. start job */
    s32Handle = HI_TDE2_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        TDE_PRINT("start job failed!\n");
        goto FB_PROCESS_ERROR2;
    }
    
    /* 2. bitblt background to screen */
    s32Ret = HI_TDE2_QuickCopy(s32Handle, &g_stBackGround, &stSrcRect, 
        &g_stScreen[0], &stSrcRect);
    //s32Ret = HI_TDE2_QuickFill( s32Handle, &g_stBackGround, &stDstRect, 0x8000);
    if(s32Ret < 0)
    {
        TDE_PRINT("Line:%d failed,ret=0x%x!\n", __LINE__, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        goto FB_PROCESS_ERROR2;
    }
    /* 4. bitblt image to screen */
    /*s32Ret = HI_TDE2_Bitblit(s32Handle, &g_stScreen[0], &stDstRect, 
        &g_stBackGround, &stSrcRect, &g_stScreen[0], &stDstRect, &stOpt);
    if(s32Ret < 0)
    {
        TDE_PRINT("Line:%d,HI_TDE2_Bitblit failed,ret=0x%x!\n", __LINE__, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        goto FB_PROCESS_ERROR2;
    }*/

    /* 5. submit job */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 10);
    if(s32Ret < 0)
    {
        TDE_PRINT("Line:%d,HI_TDE2_EndJob failed,ret=0x%x!\n", __LINE__, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        goto FB_PROCESS_ERROR2;
    }

    /* 3. use tde and framebuffer to realize rotational effect */

    /*set frame buffer start position*/
    if (ioctl(s32Fd, FBIOPAN_DISPLAY, &stVarInfo) < 0)
    {
        TDE_PRINT("process frame buffer device error\n");
        goto FB_PROCESS_ERROR2;
    }

    s32Ret = 0;
    
FB_PROCESS_ERROR2:    
    HI_MPI_SYS_MmzFree(g_stBackGround.u32PhyAddr, pu8BackGroundVir);
FB_PROCESS_ERROR1:
    munmap(pu8Screen, u32Size);
FB_PROCESS_ERROR0:
    close(s32Fd);
FB_OPEN_ERROR:
    HI_TDE2_Close();

    return s32Ret;

}


long MyTest()
{
    char* szMyText = "@(上&海I).P。C明定-1_27#";
    CTextOnYuv textOnYuv;
    unsigned char** pCodeList;
    int* pHalfList = NULL;
    int nCount = 0;
    int iFont=64; // 字体点阵大小
    int iFontWidth = 0; // 字体宽度，如果是字符则为iFont的一半，汉字为iFont
    int iTextWidth = 0;//OSD文本宽度
    int iRet = 0;
    int m,i,j;//循环

    int fbfd=0;
    struct fb_var_screeninfo vinfo;
    unsigned long screensize=0;
    char *fbp=0;
    fbfd=open("/dev/fb1",O_RDWR);  //打开帧缓冲设备
    if(!fbfd){
        printf("error\n");
        return -1;
    }
    if(ioctl(fbfd,FBIOGET_VSCREENINFO,&vinfo)){  //获取屏幕可变参数
        printf("error: ioctl FBIOGET_VSCREENINFO\n");
        iRet = -1;
        goto GOTO_ERR1;
    }
    //打印屏幕可变参数
    printf("SCREEN: %d x %d, %dbpp\n",vinfo.xres,vinfo.yres,vinfo.bits_per_pixel);
    screensize=vinfo.xres*vinfo.yres* vinfo.bits_per_pixel/2;  //缓冲区字节大小
    fbp=(char *)mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fbfd,0);//映射
    if((int)fbp==-1){
        printf("error: mmap\n");
        iRet = -1;
        goto GOTO_ERR1;
    }   
    memset(fbp,0,screensize); //清屏

    iRet = textOnYuv.GetCodeList( szMyText, iFont, pCodeList, pHalfList, nCount);
    if(iRet !=0)
        goto GOTO_ERR2;

    for(m=0; m<nCount; m++)
    {
        if(pHalfList[m]==0)
        {
            iFontWidth = iFont;
        }
        else
        {
            iFontWidth = iFont / 2;
        }
        for(j=0; j<iFont; j++)
        {
            for(i=0; i<iFontWidth; i++)
            {
                if(pCodeList[m][j*iFont+i]>0)
                {
                    //白色点阵
                    *((unsigned short *)(fbp + j*vinfo.xres*2 + iTextWidth + i*2))=0xffff;
                }
            }
        }
        // 字间隔 =4,每个像素占2字节
        iTextWidth = iTextWidth + iFontWidth*2 + 8;
        if(iTextWidth > vinfo.xres*2)
        {
            iRet = -1;
            printf("Error: Text Width=%d > %dScreen Width[m=%d,j=%d,i=%d]\n", iTextWidth/2, vinfo.xres, m,j,i);
            goto GOTO_ERR3;
        }

    }
    /*
    char hz[16][2]={
    0x08, 0x00, 0x08, 0x00, 0x08, 0x04, 0x7E, 0x84, 0x08, 0x48, 0x08, 0x28, 0xFF, 0x10, 0x08, 0x10,
    0x28, 0x28, 0x2F, 0x28, 0x28, 0x44, 0x28, 0x84, 0x58, 0x00, 0x48, 0x00, 0x87, 0xFE, 0x00, 0x00,                                                                                  
    }; //16*16字模库中提取的“赵”字对应的字符数组
    int i,j,k;
    for(j=0;j<16;j++){
         for(i=0;i<2;i++){
             for(k=0;k<8;k++){
                 if(hz[j][i]&(0x80>>k))
                     *((unsigned short *)(fbp + j*vinfo.xres*2 + i*16 + k*2))=0xf100;
             }
         }
    }
    */
GOTO_ERR3:
    PTR_DELETE_A( pCodeList);
    PTR_DELETE( pHalfList);

GOTO_ERR2:
    munmap(fbp,screensize);
GOTO_ERR1:
    close(fbfd);
    return iRet;

}



