#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>



#define LOGCONTENTMAXSIZE 2048

//extern char* GetTimeNow(char* buff);


#define BASE_TL_PRIVATE			192/* 192-255 are private */

#define TL_DEV_REG_RW 			_IOWR('V', BASE_TL_PRIVATE + 9, unsigned int)
#define TL_PW_VALID				_IOW('V', BASE_TL_PRIVATE + 10, unsigned char)
#define TL_AUDIO_SWITCH			_IOW('V', BASE_TL_PRIVATE + 12, unsigned int)
#define TL_RS485_CTL			_IOW('V', BASE_TL_PRIVATE + 13, unsigned int)
#define TL_GET_FORMAT			_IOWR('V', BASE_TL_PRIVATE + 14, unsigned int)
#define TL_KEYPAD_CTL			_IOW('V', BASE_TL_PRIVATE + 15, unsigned int)
#define TL_ALARM_OUT			_IOW('V', BASE_TL_PRIVATE + 16, unsigned int)
#define TL_BUZZER_CTL			_IOW('V', BASE_TL_PRIVATE + 17, unsigned int)
#define TL_ALARM_IN				_IOWR('V', BASE_TL_PRIVATE + 18, unsigned int)
#define TL_SET_HARDWARE_TYPE	_IOWR('V', BASE_TL_PRIVATE + 19, int)
#define TL_SATA_STATUS		    _IOWR('V', BASE_TL_PRIVATE + 20, int)
#define TL_SCREEN_CTL			_IOW('V', BASE_TL_PRIVATE + 21, unsigned int)
#define TL_POWER_CTL			_IOW('V', BASE_TL_PRIVATE + 22, unsigned int)
#define TL_DVR_RST			    _IOWR('V', BASE_TL_PRIVATE + 23, int)

enum ePTZAction
{
	PTZ_STOP = 0x00,				//00H－停止
	PTZ_RIGHT,						//01H－向右转
	PTZ_LEFT,						//02H－向左转
	PTZ_UP,							//03H－向上转
	PTZ_DONW,						//04H－向下转
	PTZ_RIGHTUP,					//05H－向右上转
	PTZ_RIGHTDOWN,					//06H－向右下转
	PTZ_LEFTUP,						//07H－向左上转
	PTZ_LEFTDOWN,					//08H－向左下转
	PTZ_ZOOMIN,						//09H－放大
	PTZ_ZOOMOUT,					//0AH－缩小
	PTZ_NEAR,						//0BH－近调焦
	PTZ_FAR,						//0CH－远调焦
	PTZ_LBIG,						//0DH－光圈大
	PTZ_LSMALL,						//0EH－光圈小
	PTZ_SETPOSITION,				//0FH－设置预置位
	PTZ_GETPOSITION,				//10H－读取预置位
	PTZ_OPEN,						//11H－开关开
	PTZ_CLOSE,						//12H－开关关
	PTZ_WASH,						//13H－WASH
	PTZ_WIPE,						//14H－WIPE
	PTZ_CLEARPRESET,				//15H－Clear Preset
	PTZ_SHUTTER,					//16H－快门
	PTZ_PLUS,						//17H－增益
	PTZ_TV_PTZ_LIGHT,				//18H－TV_PTZ_LIGHT;
	PTZ_TV_PTZ_MENU,				//19H－TV_PTZ_MENU ;
	PTZ_CCTV_PTZ_RESET,				//1AH－CCTV_PTZ_RESET;
	PTZ_CCTV_PTZ_CHROMAADJUST,		//1BH－CCTV_PTZ_CHROMAADJUST;
	PTZ_CCTV_PTZ_SATURATIONADJUST,	//1CH－CCTV_PTZ_SATURATIONADJUST;
	PTZ_CCTV_PTZ_CONTRASTADJUST,	//1DH－CCTV_PTZ_CONTRASTADJUST;
	PTZ_SETAUX,						//1EH－SETAUX 调用扩展动作
	PTZ_CLEARAUX,					//1FH－CLEARAUX 停止扩展动作
	PTZ_VALIDATE,					//20H－登入验证
	PTZ_VALIDATESTOP,				//21H－结束登入验证
	PTZ_SET_ZERO_POSITION,			//22H－设置起始角度原点
	PTZ_SET_PAN_POSITION,			//23H－调用水平角度方位
	PTZ_SET_TILT_POSITION,			//24H－调用垂直角度方位
	PTZ_SET_ZOOM_POSITION,			//25H－调用镜头缩放角度方位
	PTZ_QUERY_PAN_POSITION,			//26H－查询水平角度方位
	PTZ_QUERY_TILT_POSITION,		//27H－查询垂直角度方位
	PTZ_QUERY_ZOOM_POSITION,		//28H－查询镜头缩放角度方位
	PTZ_GAMMA,		                //29H－设置GAMMA值
    PTZ_BRIGHTNESS,		            //30H－设置亮度值
    PTZ_SHARPNESS,					//31H-   设置锐度
    PTZ_SETGARRISON, 				//32H-   布防
    PTZ_CLEARALARM,                  //33H-   清除报警
};

enum eDeviceType
{
	DeviceModel_Encoder = 0x0,
	DeviceModel_HDEncoder = 0x1,
	DeviceModel_Decoder = 0x2,
};

extern int g_nMaxEncChn;
extern int g_nMaxViChn;
extern int g_nDeviceType;

extern char g_szSipServerIp[256];
extern char g_szSipServerId[256];
extern char g_szLocalIp[50];

#define MAXDECCHANN 4
#define MAXDISPLAYNUM 3
#define MAXENCCHAN 100
#define MAXALARMINCHN 4
#define MAXALARMOUTCHN 1

#define MAXNET_COUNT  3
#define MAXOSD_COUNT 7
#define MAXVI_COUNT 16




#define	PELCOPAN_RIGHT	0x02
#define	PELCOPAN_LEFT	0x04
#define	PELCOTILT_UP	0x08
#define	PELCOTILT_DOWN	0x10

#define	PELCO_ZOOMOUT	0x40
#define	PELCO_ZOOMIN	0x20


#define DEV_WD "/dev/watchdog"
#define DEV_RS485 "/dev/ttyAMA2"
#define DEV_TL9800 "/dev/tl_R9800" 

enum StreamCastType
{
	StreamCastType_Multicast = 0x01,
	StreamCastType_Unicast = 0x02,
	StreamCastType_TcpServer = 0x03,
	StreamCastType_TcpClient = 0x04, 
	StreamCastType_Callback = 0x05,
	StreamCastType_RTSP_OVER_RTP = 0x06,
	StreamCastType_MulticastTcp = 0x07,
};


#pragma pack (push,1)

typedef struct _EncNetParam
{
	int iEnable; //0: disable 1: enable
	int iIndex;
	int iNetType; //0: udp 1: tcp 2: rtsp 3: RTP
	char szPlayAddress[256];
	int iPlayPort;
	int iMux; //0: es 1: ps 2: ts 
}EncNetParam;

typedef struct _EncNetSettings
{
	int iChannelNo;
	EncNetParam net[MAXNET_COUNT];
}EncNetSettings;

typedef struct _OsdParam
{	
	int iEnable;
	int iX;
	int iY;
	int iType; //0: date 1: time  2: date and time 3: content 4: name
	char szOsd[256];
}EncOsdParam;


typedef struct _EncOsdSettings
{
	int iChannelNo; 
	char szId[256];
	int iFont; //0: 16点阵 1: 32
	EncOsdParam osd[MAXOSD_COUNT]; 
}EncOsdSettings;

typedef struct __EncoderParam
{
	int iProfile; // 0: baseline 1: main profile 2: high profile default: 2
	int iBitRate;    // KB   < 10 * 1024   default: 1500 
	int iFPS;  // 1~30    default: 25
 	int iResolution; //0: QCIF, 1: CIF,  2:D1, 3:720P, 4:1080P  default: 2 
	int iCbr;  //0: cbr, 1: vbr, 2: fqp default:0 	
	int iGop; //default: 10
	int iIQP;//default:20
	int iPQP; //default: 23
	int iMinQP; //default: 24
	int iMaxQP;  //default: 32
	int iAudioEnable; //0: enable 1: disable
	int iAudioFormat; //1:G711A 2:G711u   
	int iAudioRate; //8000 12000 11025 16000 22050 2400 32000 44100  48000
	int iBits; //0: 8bits 1: 16bits 2: 32bits
}EncoderParam;

enum MD_OSD_SHOW_MODE // 可以进行或操作
{
    MD_OSD_SHOW_NONE = 0,  // 不显示
    MD_OSD_SHOW_INFO = 1,  // 内容信息
    MD_OSD_SHOW_MONID = 2, // 监视器ID
};
enum MD_DISPLAY_MODE
{
    MD_DISPLAY_MODE_MESS = 0, // 不同显示通道关联不同解码通道
    MD_DISPLAY_MODE_1MUX = 1,  // 所有显示通道关联同一个解码通道，单屏显示
    MD_DISPLAY_MODE_4MUX = 2, // 4分屏显示
    MD_DISPLAY_MODE_9MUX=3,  // 9分屏显示
    MD_DISPLAY_MODE_16MUX = 4, // 16分屏显示
};

enum DEC_OSD_POSXY
{
    DEC_OSD_X = 1, // X
    DEC_OSD_Y = 2, // Y
};
enum DEC_OSD_COLOR
{
    DEC_OSD_COLOR_WHITE = 0xffff,// 白色
    DEC_OSD_COLOR_BLACK = 0x8000,// 黑色
    DEC_OSD_COLOR_RED = 0xfc00,// 红色
    DEC_OSD_COLOR_GREEN = 0x83e0, // 绿色
    DEC_OSD_COLOR_BLUE = 0x801f, // 蓝色
    DEC_OSD_COLOR_YELLOW = 0xffe0, // 黄色
    DEC_OSD_COLOR_PURPLE = 0xb00c, // 紫色
    DEC_OSD_COLOR_CYAN = 0x83ff, // 青色
    DEC_OSD_COLOR_MAGENTA = 0xfc1f, // 洋红
};

enum OsdType
{
	OsdType_Date, 
	OsdType_Time, 
	OsdType_DateAndTime, 
	OsdType_Content,
	OsdType_Name,
};

enum ResolutionType
{
	ResolutionType_QCIF, 
	ResolutionType_CIF, 
	ResolutionType_D1, 
	ResolutionType_720P, 
	ResolutionType_1080P
};

enum MuxType
{
	MuxType_ES = 0x00, 
	MuxType_PS, 
	MuxType_TS
};

enum NetType
{
	NetType_UDP = 0x00, 
	NetType_TCP, 
	NetType_RTSP,
};

typedef struct _AlarmInSetting
{
	int iEnable;
	char szId[100];
	int iNormalStatus; //0: always open 1: always close
	int iTrigerAlarmOut; // triger alarm out.  -1: disable   >=0 : alarm out channel no.
	int iStableTime;   // stable time. second    1~600 
	int iOutDelay; //output delay time second.   0-300
}AlarmInSetting;

typedef struct _AlarmOutSetting
{
	int iEnable;  //
	char szId[100];
	int iNormalStatus; //0: open 1: close; 
	int iStatus; //  0: open 1: close;
}AlarmOutSetting;

typedef struct _AlarmSettings
{
	AlarmInSetting alarmin[MAXALARMINCHN];
	AlarmOutSetting alarmout[MAXALARMOUTCHN];
}AlarmSettings;

typedef struct _PtzSetting
{
	int iPtzType; //0: pelcoD
	int iAddressCode;
}PtzSetting;

typedef struct _PtzPortSetting
{
	int iBaude; // 0:2400 1:4800(缺省) 2:9600 3:19200 4:38400 5:51200 
	int iDataBits; //0: 5 1:6 2:7 3:8(缺省)
	int iParity;   //0:无(缺省) 1:奇校验 2: 偶校验
	int iStopBits; //0:1(缺省) 1:1.5 2:2
}PtzPortSetting;

typedef struct _ForwardportSetting
{
	int iEnable; // 使能， 0-否，1-是
	int iType; //0-udp，1-tcp
	int iPort;   //端口
}ForwardportSetting;

typedef struct _EncSetting
{
	EncOsdSettings osd;
	EncNetSettings net;
	EncoderParam enc;
	int iEnable;
	char szName[256];
	char szDeviceId[256];
}EncSetting;

typedef struct _EncSettings
{
	int iChannelCount;
	EncSetting encset[MAXENCCHAN];
	PtzSetting ptz[MAXENCCHAN/2];
	PtzPortSetting ptzport;
	AlarmSettings alarm;
}EncSettings;

#pragma pack (pop)


//获取时间字符串
inline char* GetDateTimeNowUs(char* buff)
{
	struct timeval tv;
	struct tm stm;
	struct tm stmres;
	gettimeofday(&tv,NULL);
	stm = *(localtime_r(&tv.tv_sec,&stmres));
	sprintf(buff, "%04d-%02d-%02d %02d:%02d:%02d:%06ld",
		stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec, tv.tv_usec);
	return buff;
}

//获取时间字符串
inline char* GetDateTimeNow(char* buff)
{
	struct timeval tv;
	struct tm stm;
	struct tm stmres;
	gettimeofday(&tv,NULL);
	stm = *(localtime_r(&tv.tv_sec,&stmres));
	sprintf(buff, "%04d-%02d-%02d %02d:%02d:%02d",
		stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
	return buff;
}

inline void myMsleep(unsigned int mstime) //参数单位: 毫秒
{
	   struct timeval tv;
	   tv.tv_sec = mstime/1000;
	   tv.tv_usec = mstime%1000 * 1000;
	   select(0,NULL, NULL, NULL, &tv);
}

//获取时间字符串
inline char* GetTimeNowOnly(char* buff)
{
	struct timeval tv;
	struct tm stm;
	struct tm stmres;
	gettimeofday(&tv,NULL);
	stm = *(localtime_r(&tv.tv_sec,&stmres));
	sprintf(buff, "%02d:%02d:%02d",
		stm.tm_hour, stm.tm_min, stm.tm_sec);
	return buff;
}


//获取时间字符串
inline char* GetDateNowOnly(char* buff)
{
	struct timeval tv;
	struct tm stm;
	struct tm stmres;
	gettimeofday(&tv,NULL);
	stm = *(localtime_r(&tv.tv_sec,&stmres));
	sprintf(buff, "%04d-%02d-%02d",
		stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday );
	return buff;
}

inline void TruncateLogFile()
{
    static int iTime = time(NULL);
    int iTimeNow = time(NULL);
    if((iTimeNow - iTime)<10)
    {
        return;
    }
    iTime = iTimeNow;
    //printf("\n###################### TruncateLogFile called!\n\n");
    long MAX_LOG_SIZE = 20*1024*1024;//允许log最大长度
    long CLR_LOG_SIZE = 12*1024*1024;//裁剪长度
    char* log_file_path = "/tmp/declog";
    int fd = -1;
    void* start = NULL;
    long lFileSize = 0;
    char* szBuff = NULL;
    struct stat sb;
    fd = open(log_file_path, O_RDWR);
    if(fd < 0 )
    {
        printf("\n######################TruncateLogFile: fd <0\n\n");
        return;
    }
    fstat(fd, &sb);
    lFileSize = sb.st_size;
    if(lFileSize < MAX_LOG_SIZE)
    {
        printf("\n###################### Log file size = %d, no truncated!\n\n", lFileSize);
        close(fd);
        return;
    }
    printf("\n###################### Log file size = %d, need to be truncated!\n", lFileSize);
    start = mmap(NULL, lFileSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(start == MAP_FAILED)
    {
        close(fd);
        printf("\n###################### start mmap failed!\n\n");
        if(lFileSize > MAX_LOG_SIZE)
        {
            goto GOTO_ERR1;
        }
        return;
    }
    // 裁剪文件
    szBuff = new char[lFileSize-CLR_LOG_SIZE];
    if(! szBuff)
    {
        munmap(start, lFileSize);
        close(fd);
        goto GOTO_ERR1;
    }
    memcpy(szBuff, (char*)start+CLR_LOG_SIZE, lFileSize-CLR_LOG_SIZE);
    munmap(start, lFileSize);
    close(fd);
    
GOTO_ERR1:
    // 清空所有LOG数据
    FILE *logfile = fopen( log_file_path, "w" );
    if(logfile)
    {
        if(szBuff)
        {
            fwrite(szBuff, 1, lFileSize-CLR_LOG_SIZE, logfile);
            fflush(logfile);
        }
        fclose(logfile);
    }
    delete [] szBuff;
}


//日志宏原子，只做终端打印，不调用日志对象接口处理
#ifndef _DEBUG_
#define _DEBUG_(...) \
	do {\
		char szTime[32]={0};\
		GetDateTimeNowUs(szTime);\
		char _log_szBuf[LOGCONTENTMAXSIZE]={0};\
		snprintf(_log_szBuf, sizeof(_log_szBuf),__VA_ARGS__);\
		printf("[_DEBUG_] [%s %s %s:%d %s] %s\n",szTime,MODULENAME,__FILE__,__LINE__,__FUNCTION__,_log_szBuf);\
		if(1) \
        { \
    		TruncateLogFile(); \
    		FILE *logfile; \
    		logfile = fopen( "/tmp/declog", "a+" );\
    		if ( logfile != NULL ) \
    		{\
    			fprintf( logfile, "[_DEBUG_] [%s %s %s:%d %s] %s\n",szTime,MODULENAME,__FILE__,__LINE__,__FUNCTION__,_log_szBuf);\
    			fclose( logfile ); \
    		}\
        } \
	} while(0)
#endif





