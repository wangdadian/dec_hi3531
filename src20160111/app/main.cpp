
#include "ChannelManager.h"
#include "MDProtocol.h"
#include "AT200Protocol.h"
#include "Forwardport.h"
#include "Hi3531Global.h"
#include "2301DecoderProtocol.h"
#include "OnvifRcvProtocol.h"
#include "UpProtocol.h"
#include "FlashRW.h"
#include <pthread.h>
#include "WebSvr.h"
#include "encoder.h"
#include "DecoderSetting.h"
#include "EncoderSetting.h"
#include "Text2Bmp.h"
#include "Vmux6671Protocol.h"
#include "ZyProtocol.h"
#include "TJGSProtocol.h"
#include  <arpa/inet.h>
#include "StProtocol.h"
#include  <net/if.h>
#include  <net/if_arp.h>
#include  <sys/ioctl.h>
#include  <sys/stat.h>
#include  <arpa/inet.h>
#include  <fcntl.h>

int g_exit = 0;

void SAMPLE_VDEC_HandleSig( int sigid )
{
	g_exit =1;
}

HI_S32 InitRS485( char *sz485Dev, int iEnable )
{
	unsigned int buf[5] = {0};
	if(iEnable)
	{
		buf[0] = 1; 
	}
	else
	{
		buf[0] = 0;
	}

	int fd_ctl = open( sz485Dev, O_RDWR, 0 );

	if ( fd_ctl <= 0 )
	{
		_DEBUG_("open RS485_CTL device %s error", sz485Dev );
		perror( "open RS485_CTL device error" );
		return -1;
	}
	
	if(ioctl(fd_ctl, TL_RS485_CTL, buf) < 0)
	{
		_DEBUG_("RS485_CTL error");
		perror("tl_rs485_ctl: RS485_CTL error");
		return -1;
	}

	close( fd_ctl );
	return 0;
}


//static sem_t semForWD;
void *watchdogThrFxn(void *arg)
{
	printf("$$$$$$$$$$$$$$$$$$watchdogThrFxn id:%d\n",getpid());
    int wd_fd = -1;
	while( g_exit == 0 )
	{
	    wd_fd = -1;
        wd_fd = open(DEV_WD, O_RDWR, 0);
        if(wd_fd == -1)
        {
            printf("Cannot open %s (%s)\n", DEV_WD, strerror(errno));
        }
        else
        {
		    ioctl(wd_fd, WDIOC_KEEPALIVE, NULL);
            close(wd_fd);
	        wd_fd = -1;
        }
		//_DEBUG_("feed dog\n");
		// 20秒喂一次狗
		for ( int i = 0; i < 20; i++ )
		{
			if ( g_exit != 0 )
			{
				break;
			}
			usleep(1000*1000);
		}
	}
	return NULL;
}

void PrintUsage()
{
	printf( "usage: -t{--type} <type> -h{--help}\n"
		   "\ttype:\n"
		   "\tdecoder:  \n"
		   "\tencoder: \n"
		   "\thdencoder: \n"
	);
}


void getlocalip(char *ip)	//读取系统实际IP
{
	int skfd;
	struct ifreq ifr;
	struct sockaddr addr;
	struct sockaddr *padd;
	struct sockaddr_in * aa;
	char *ifname = (char*)"eth0";
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if( skfd < 0 )
	{
		//printf("socket error");
	}
	
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) >= 0)
	{
		strcpy(ifr.ifr_name, ifname);
		if ( skfd >= 0 )
		{
			strcpy(ifr.ifr_name, ifname);
			ifr.ifr_addr.sa_family = AF_INET;
			if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) 
			{
				addr = ifr.ifr_addr;
				padd = &addr;
				aa = (struct sockaddr_in *)padd;
			//	printf("IP addr : %s\n",inet_ntoa(aa->sin_addr));
				strcpy(ip,inet_ntoa(aa->sin_addr));
			} 
            else
            {
    			//printf("Ip address not found");
            }
		}
	}
	
	close(skfd);
}

int LoadDeviceModel( CDecoderSetting *pDS )
{
	return pDS->GetDeviceModel( g_nDeviceType, NULL );
}

int main( int argc, char **argv )
{
	printf("####version:%s\n####builddate:%s\n", GetVersion(), GetBuildDate() );

    // 设置时区
    setenv("TZ", "GMT-8", 1);
    tzset();
    
	signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGXFSZ,SIG_IGN);
	signal(SIGILL, SIG_IGN);
	signal(SIGBUS, SIG_IGN);
	signal( SIGIOT, SIG_IGN);
	signal(SIGABRT, SIG_IGN);
	pthread_t th;
	pthread_create(&th,NULL,  watchdogThrFxn, NULL );

	CFlashRW *flashrw = new CFlashRW();

	int nDeviceType = DeviceModel_Decoder;

	int c;
	opterr = 0;
	char szType[256]={0};	
	while ((c = getopt (argc, argv, "t:h")) != -1)
	switch (c)
	{
	case 't': 
        if ( optarg != NULL )
        {
            strcpy( szType, optarg );
            if ( strcmp( szType, "decoder") == 0 )
            {
                nDeviceType = (int)DeviceModel_Decoder;
            }
            else if ( strcmp(szType, "encoder" ) == 0 )
            {
                nDeviceType = (int)DeviceModel_Encoder;
            }
            else if ( strcmp(szType, "hdencoder") == 0 )
            {
                nDeviceType = (int)DeviceModel_HDEncoder;
            }
        }
		break;
	case 'h':
		PrintUsage();
		exit(0);
		break;
	default:
		break;
	}

	
	CDecoderSetting *pDS = new CDecoderSetting();
    pDS->Init();

	LoadDeviceModel(pDS);
	SetDeviceType( g_nDeviceType);
	InitHi3531();
	if ( g_nDeviceType == (int)DeviceModel_Encoder ||
		g_nDeviceType == (int)DeviceModel_HDEncoder)
	{
		InitVI();
	}

	CChannelManager *pChnManager=NULL;

    CWebSvr *p_WebSvr = new CWebSvr;
	p_WebSvr->init();
	getlocalip(g_szLocalIp);

    CEncoderSetting *pES = new CEncoderSetting();
    pES->Init();

	pChnManager = new CChannelManager(pDS, pES);

	
	printf( "start listening ... MD@4501\n");
	CMDProtocol mdprotocol( 4501, pChnManager );
	pChnManager->AddProtocols(&mdprotocol);
	
/*	
	CZyProtocol zyprotocol( 9999, pChnManager );
	pChnManager->AddProtocols(&zyprotocol);
    */


    // 天津高速协议
	printf( "start listening ... tjgs@5060\n");
	CTJGSProtocol tjgsprotocol(5061, pChnManager );
	pChnManager->AddProtocols(&tjgsprotocol);

	printf( "start listening ... AT200@5000\n");
	CAT200Protocol at200protocol( 5000, pChnManager );
	pChnManager->AddProtocols(&at200protocol);

	printf( "start listening ... 2301@9997\n");
	C2301DecoderProtocol dec2301protocol( 9997, pChnManager );
	pChnManager->AddProtocols(&dec2301protocol);
/*
    // SCOM协议
	printf( "start listening ... SCOM-ST@9000\n");
	CStProtocol stProtocol( 9000, pChnManager );
	pChnManager->AddProtocols(&stProtocol);
	
#ifdef ROM32_DEF
	printf( "start listening ... ONVIF-Receiver@8080\n");
	COnvifRcvProtocol orProtocol( 8080, pChnManager );
	pChnManager->AddProtocols(&orProtocol);
#endif
 */   
    printf( "start listening ... Upgrade@20152\n");
	CUpProtocol upProtocol( 20152, pChnManager );
	pChnManager->AddProtocols(&upProtocol);

    printf( "start listening ... Vmux6671@5501\n");
    CVmux6671Protocol vumx6671Protocol( 5501, pChnManager );
    pChnManager->AddProtocols(&vumx6671Protocol);

    // 透明端口
    ForwardportSetting sfp;
    memset(&sfp, 0, sizeof(ForwardportSetting));
    pChnManager->m_pEncSet->GetEncForwardport(&sfp);
    if(sfp.iEnable != 0)
    {
        printf("start listening ... Forward %s@%d\n", sfp.iType==0?(char*)"udp":(char*)"tcp", sfp.iPort);
    }
    CForwardport forwardportprotocol(&sfp, pChnManager);
    pChnManager->AddProtocols(&forwardportprotocol);


	if ( g_nDeviceType == (int)DeviceModel_Decoder )
	{
    // webserver
	}

	while( g_exit == 0 )
	{
		sleep(1);
	}

	if (pChnManager != NULL )
	{
		delete pChnManager;
		pChnManager = NULL;
	}

	UnInitHi3531();
	InitRS485((char*)DEV_TL9800, 0);
	signal(SIGKILL, SIG_DFL );
	signal(SIGQUIT, SIG_DFL );
	signal(SIGINT,SIG_DFL);
	signal(SIGFPE,SIG_DFL);
	signal(SIGSEGV,SIG_DFL);
	signal(SIGTERM,SIG_DFL);
	signal(SIGHUP,SIG_DFL);
	signal(SIGABRT,SIG_DFL);
	signal(SIGPIPE,SIG_DFL);
	signal(SIGXFSZ, SIG_DFL);
    return 0;
}



