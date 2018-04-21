#ifndef _DECODER_OSD_H__
#define _DECODER_OSD_H__

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
#include <list>
using namespace std;

typedef struct _tagMyOsdInfo
{
    bool bChanged; // 数据是否已修改，用于是否刷新OSD
    int iChnno; // OSD关联通道号，等于(DecodecOsdParam*)pData->channel
    void* pData;
}struMyOsdInfo;

#define WORD_SPACE 0  // osd显示字之间的间隔像素
#define DATE_STRING_LEN 64

class CDecoderOsd
{
public:
	CDecoderOsd(struDisplayResolution* pHdmiDR/*HDMI显示分辨率信息*/);
	~CDecoderOsd();	
    // 设置显示组合模式, channel为关联通道号
    void SetOsdShowMode(int mode, int channel);
    // 清除chennel通道的osd信息，channel=0时清除所有通道osd信息
    void ClearOsdShow(int channel);
    void OnOsdShowChanged(int iShow);
	void SetOsdDate(void* pData);
	void OnDisplayResolutionChanged(const struDisplayResolution* pDR);
    void SetOsdInfo(void* pData);
    long SetMonitorId(int chnno, int mid);

private: 
    static void* ThOsdWorker(void* param);
    static void* ThMonitorWorker(void* param);
    long SetDisplayResolution(const struDisplayResolution* pDR);
    char* conv_time(int itime);
    long DisplayOsd(bool bShow);
    long DisplayMonID(bool bShow);
    long RestartOsdDisplay();
    bool IsOsdInfoChanged(void* p1, void* p2);
    long SetOsdColor(int colortype, short& OsdColor, short& OsdEdgeColor);
    long SetOsdChangedFlaged(bool);
    int SetOsdPosXY(unsigned int pos, int xy, int chnno);
    // 清空OSD显示
    void ClearOsdDisplay();
private:
	int m_iExitFlag;
    // Dispaly
    struDisplayResolution m_sDR;
    unsigned int m_uiDisplayX;
    unsigned int m_uiDisplayY;
    //osd
    bool m_bOsdWorkerExit;
    pthread_t m_thOsdWorker;
    pthread_t m_thMoniterWorker;
    bool m_bOsdCleared;
    bool m_bMidOsdCleared;
    char **m_szLastDate;
    pthread_mutex_t m_lockOsd;
    pthread_mutex_t m_lockRestartOsdDisplay;
    bool m_bOsdShowFromWeb;
    bool m_bOsdShowMonId;

    CTextOnYuv m_textOnYuv;
    int m_fbfd;
    struct fb_var_screeninfo m_vinfo;
    struct fb_fix_screeninfo m_finfo;
    unsigned long m_screensize;
    char *m_fbp;
    bool m_bFirst;
    bool m_bStartOsdDisplay;
    // 关联通道
    int m_iChannelNo;
    //显示模式
    int m_iMuxMode;
    // multimap: bool--指示osd信息是否改变(时间osd改变不处理)，void--指向osd信息内存
    list<struMyOsdInfo*> m_listOsdDataNow;
    // 通道关联的监视器号码
    int *m_iMonitorIdList;
    //监视器号是否发生改变
    bool m_bMonitorIdChanged;
};

#endif //_DECODER_OSD_H__












