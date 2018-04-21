
#include "PtzCtrl.h"
#include "define.h"
#include "PublicDefine.h"
#include <stdio.h>	 /*标准输入输出定义 */
#include <stdlib.h>	 /*标准函数库定义 */
#include <unistd.h>	 /*Unix标准函数定义 */
#include <sys/types.h> 	/**/
#include <sys/stat.h>	/**/
#include <fcntl.h>	 /*文件控制定义 */
#include <termios.h>	/*PPSIX终端控制定义 */
#include <errno.h>	 /*错误号定义 */



CPtzCtrlSession::CPtzCtrlSession( void *pCtrl )
{
	m_pCtrl = pCtrl;
	m_LastCmd = 0;
	m_PanSpd = 0; 
	m_TiltSpd = 0;
	memset( &m_ptzsetting, 0, sizeof(m_ptzsetting));
}

CPtzCtrlSession::~CPtzCtrlSession()
{
	
}

void CPtzCtrlSession::SetConfig( PtzSetting ptzset )
{
	memcpy( &m_ptzsetting, &ptzset, sizeof(ptzset));
}

int CPtzCtrlSession::CheckSum(unsigned char *pdata, int len)
{
	int i, sum = 0;
	for (i=0;i<len;i++ ) sum += pdata[i];
	return sum;
}

int CPtzCtrlSession::Ptz( int nPtzAction, 
				   int iParam1,
				   int iParam2 )
{
	int nLen = 0;
	unsigned char pData[256]={0};
	if ( m_ptzsetting.iPtzType == (int)PtzDeviceType_PELCOD )
	{
		GetPelcoDCode( m_ptzsetting.iAddressCode, 
			nPtzAction, 
			iParam1, 
			iParam2, 
			(unsigned char*)pData, 
			nLen );
	}
	if ( nLen > 0 )
	{
		((CPtzCtrl*)m_pCtrl)->WriteData((char*)pData, nLen);
	}
	return 0;
}

long CPtzCtrlSession::GetPelcoDCode( int nAddressCode, 
								   int nPtzAction, 
								   int iParam1, 
								   int iParam2, 
							       unsigned char *pdata /*7bytes*/, 
							       int &nLen )
{
	pdata[0] = 0xFF;
	pdata[1] = nAddressCode;

	int LastCmd = m_LastCmd;
	int PanSpd = m_PanSpd;
	int TiltSpd = m_TiltSpd;
	nLen = 7;
	switch ( nPtzAction )
	{
	case PTZ_OPEN:
	{
		if( iParam1>0 ) pdata[2] |= 0x02;
		
		break;
	}
	case PTZ_CLOSE:
	{
		if (iParam1>0) pdata[2] |= 0x04;
		
		break;
	}
	case PTZ_NEAR:
	{
		if ( iParam1>0 ) pdata[2] |= 0x01;
				break;
	}
	case PTZ_FAR :
	{
		if ( iParam1>0 ) pdata[3] |= 0x80;
			break;
	}

	case PTZ_ZOOMIN:
	{
		if (iParam1>0) pdata[3] |= 0x20;
			break;
	}
	case PTZ_ZOOMOUT:
	{
		if (iParam1>0) pdata[3] |= 0x40;
			break;
	}
	case PTZ_DONW:
	{
		if (iParam1==0)
			pdata[3] = LastCmd	& (~PELCOTILT_DOWN);
		else
			pdata[3] = ((LastCmd | PELCOTILT_DOWN) & (~PELCOTILT_UP));

		if ( pdata[3]&(PELCOPAN_LEFT|PELCOPAN_RIGHT) ) pdata[4] = PanSpd;
			pdata[5] = iParam1 * 60 / 255;
		break;
	}
	case PTZ_UP:
	{
		if (iParam1==0)
				pdata[3] = LastCmd & (~PELCOTILT_UP) ;
		else
			pdata[3] = ((LastCmd | PELCOTILT_UP) & (~PELCOTILT_DOWN));

		if ( pdata[3]&(PELCOPAN_LEFT|PELCOPAN_RIGHT) ) pdata[4] = PanSpd;
		pdata[5] = iParam1 * 60 / 255;
		break;
	}
	case PTZ_LEFT:
	{
		if (iParam1==0)
			pdata[3] = LastCmd & (~PELCOPAN_LEFT);
		else
			pdata[3] = ((LastCmd | PELCOPAN_LEFT) & (~PELCOPAN_RIGHT)); 

		pdata[4] = iParam1 * 180 / 255; 
		if ( pdata[3] &(PELCOTILT_UP|PELCOTILT_DOWN) ) pdata[5] = TiltSpd;
		break;
	}
	case PTZ_RIGHT:
	{
		if (iParam1==0)
			pdata[3] = LastCmd & (~PELCOPAN_RIGHT);
		else
			pdata[3] = ((LastCmd | PELCOPAN_RIGHT) & (~PELCOPAN_LEFT));

		pdata[4] = iParam1 * 180 / 255; 
		if ( pdata[3] &(PELCOTILT_UP|PELCOTILT_DOWN) ) pdata[5] = TiltSpd;
		break;
	}
	case PTZ_GETPOSITION:
	{
		if ( (iParam1<1)||(iParam1>0xFF) )
		{
			_DEBUG_("view call:invalid param,0x01~0xff");
			return -1;
		}
		
		if ( iParam1 == 94 )	//yaan "set zone start"
		{
			pdata[3] = 0x11;
			pdata[5] = 0x00;
		}
		else if ( iParam1 == 44 )  //samlink	"run Pattern"
		{
			pdata[3] = 0x23;
		}
		else
		{
			pdata[3] = 0x07;
			pdata[5] = iParam1;
		}
		break;	
	}
	case PTZ_CLEARPRESET:
		{
			if ( (iParam1<1)||(iParam1>0xFF) )
			{
				_DEBUG_("clear preset:invalid param,0x01~0xff");
				return -1;
			}
			
			pdata[3] = 0x05;
			pdata[5] = iParam1;
			break;	
		}
		
	case PTZ_SETPOSITION:
	{
		if ( (iParam1<1)||(iParam1>0xFF) )
		{
			_DEBUG_("set preset:invalid param,0x01~0xff");
			return -1;
		}
		
		if ( iParam1 == 42 ) //samlink "set Pattern start"
		{
			pdata[3] = 0x1F;
		}
		else if ( iParam1 == 43 ) //samlink "set pattern stop"
		{
			pdata[3] = 0x21;
		}
		else
		{
			pdata[3] = 0x03;
			pdata[5] = iParam1;
		}
		//pdata[6] = CheckSum(pdata+1,5);
		//SendData( subrack, pdata, 7);
		break;	
	}	
	case PTZ_WIPE:
	{
		if ( iParam1 == 0 )
		{
			pdata[3] = 0x0B;
		}
		else
		{
			pdata[3] = 0x09;
		}
		pdata[5] = 1;
		//pdata[6] = CheckSum( pdata+1, 5);
		//SendData( subrack, pdata, 7 );	
		break;
	}
	default :
		return -1;
	}

	if ( !(pdata[3]&1) )
	{
		LastCmd = pdata[3];
		PanSpd = pdata[4];
		TiltSpd = pdata[5];
	}
	pdata[6] = CheckSum(pdata+1,5);
	return 0;

}


CPtzCtrl::CPtzCtrl( int nBaudRate, int nStopBits, int nDataBits, int nParity )
{
	m_iFd = 0;

	/*
	typedef struct _PtzPortSetting
{
	int iBaude; // 0:2400 1:4800(缺省) 2:9600 3:19200 4:38400 5:51200 
	int iDataBits; //0: 5 1:6 2:7 3:8(缺省)
	int iParity;   //0:无(缺省) 1:奇校验 2: 偶校验
	int iStopBits; //0:1(缺省) 1:1.5 2:2  
}PtzPortSetting;*/

	switch( nBaudRate )
	{
		case 0:
			nBaudRate = 2400;
			break;
		case 1:
			nBaudRate = 4800;
			break;
		case 2:
			nBaudRate = 9600;
			break;
		case 3:
			nBaudRate = 19200;
			break;
		case 4:
			nBaudRate = 38400;
			break;
		case 5:
			nBaudRate = 51200;
			break;	
		default:
			nBaudRate = 4800;
			break;
	};

	switch( nDataBits )
	{
		case 0:
			nDataBits = 7;
			break;
		case 1:
			nDataBits = 7;
			break;
		case 2:
			nDataBits = 7;
			break;
		case 3:
			nDataBits = 8;
			break;

		default:
			nDataBits = 8;
			break;
	};

	switch( nStopBits)
	{
		case 0:
			nStopBits = 1;
			break;
		case 1:
			nStopBits = 1;
			break;
		case 2:
			nStopBits = 2;
			break;
		default:
			nStopBits = 1;
			break;
	};

	switch( nParity )
	{
		case 0:
			nParity = 'N';
			break;
		case 1:
			nParity = 'O';
			break;
		case 2:
			nParity = 'E';
			break;
		default:
			nParity = 1;
			break;
	};

	Open( nBaudRate, nStopBits, nDataBits, nParity );
	for ( int i = 0 ; i < g_nMaxEncChn; i++ )
	{
		m_ptzctrlsess[i] = NULL;
	}

	
}

CPtzCtrl::~CPtzCtrl()
{
	for ( int i = 0 ; i < g_nMaxEncChn; i++ )
	{
		if ( m_ptzctrlsess[i] != NULL )
		{
			delete m_ptzctrlsess[i];
			m_ptzctrlsess[i] = NULL;
		}
	}

	if ( m_iFd != 0 )
	{
		close(m_iFd );
		m_iFd = 0;
	}
}

void CPtzCtrl::SetPtzType( int nChannelNo, PtzSetting ptzset )
{
	//memcpy( &m_ptzsetting, &ptzset, sizeof(ptzset));

	if ( nChannelNo >= g_nMaxEncChn )
	{
		_DEBUG_( "channeo %d > max enc chan", nChannelNo );
		return;
	}
	
	if ( m_ptzctrlsess[nChannelNo] == NULL ) 
	{
		m_ptzctrlsess[nChannelNo] = new CPtzCtrlSession( (void*)this );
	}
	
	m_ptzctrlsess[nChannelNo]->SetConfig( ptzset );
}

int CPtzCtrl::Ptz( int nChannel, 
				   int nPtzAction, 
				   int iParam1,
				   int iParam2 )
{

	if ( nChannel >= g_nMaxEncChn )
	{
		return -1;
	}
	
	if ( m_ptzctrlsess[nChannel] == NULL ) 
	{
		return -1;
	}
	
	m_ptzctrlsess[nChannel]->Ptz(nPtzAction, iParam1,  iParam2);
	return 0;
}

void CPtzCtrl::WriteData( char *buf, int len)
{
	if ( len > 0 )
	{
		printf("send 485 port: ");
		for ( int i = 0; i < len; i++ )
		{
			printf("0x%x ", buf[i] );
		}
		printf(" [len: %d]\n", len);
		//m_ptzport.Write((char*)buf, len);
		write( m_iFd,(void*) buf, len );

		/*
		char szRet[64]={0};
		int ret = read(m_iFd, szRet, 5 );
		printf( "read len:%d  %s ", ret, szRet );

		for ( int i = 0; i < ret; i++ )
		{
			printf("0x%x ", szRet[i] );
		}
		printf("\n");
		*/
	}
}


static int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300 };
static int name_arr[]  = { 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300 };

/**
*@brief  设置串口通信速率
*@param  fd     类型 int  打开串口的文件句柄
*@param  speed  类型 int  串口速度
*@return  void
***/
int CPtzCtrl::set_speed(  int speed)
{
	unsigned int i;
	int status;
	struct termios Opt;
	tcgetattr (m_iFd, &Opt);

	Opt.c_cflag |= (CLOCAL | CREAD);
	Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	//raw input
	//Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);	//raw input
	Opt.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);	 //disable software flow control
	Opt.c_cflag &= ~CRTSCTS;	 //disable hardware flow control
	Opt.c_oflag &= ~OPOST;

	for(i = 0; i < sizeof (speed_arr) / sizeof (int); i++)
	{
		if(speed == name_arr[i])
		{
			tcflush(m_iFd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);

			status = tcsetattr(m_iFd, TCSANOW, &Opt);
			if(status != 0)
			{
				perror("tcsetattr fd1");
				return -1;
			}
			return 0;
		}
	//tcflush(fd, TCIOFLUSH);
	}

	return -1;
}

/**
 *@brief   设置串口数据位，停止位和效验位
 *@param  fd     类型  int  打开的串口文件句柄*
 *@param  databits 类型  int 数据位   取值 为 7 或者8*
 *@param  stopbits 类型  int 停止位   取值为 1 或者2*
 *@param  parity  类型  int  效验类型 取值为N,E,O,,S
 */
int CPtzCtrl::set_Parity( int databits, int stopbits, int parity)
{
	struct termios options;
	if(tcgetattr(m_iFd, &options) != 0)
	{
	perror("SetupSerial 1");
	return -1;
	}

	/* 设置数据位位 */
	options.c_cflag &= ~CSIZE;
	switch(databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr, "Unsupported data size\n");
			return -1;
	}

	/* 设置奇偶校验位 */
	switch(parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;	/* Clear parity enable */
		options.c_iflag &= ~INPCK;	/* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);	/* 设置为奇效验 */
		options.c_iflag |= INPCK;	/* Disnable parity checking */
	break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;	/* Enable parity */
		options.c_cflag &= ~PARODD;	/* 转换为偶效验 */
		options.c_iflag |= INPCK;	/* Disnable parity checking */
		break;
	case 'S':
	case 's':	 /*as no parity */
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported parity\n");
		return -1;
	}

	/* 设置停止位 */
	switch(stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported stop bits\n");
		return -1;
	}

	/* Set input parity option */
	if(parity != 'n' && parity != 'N')
	options.c_iflag |= INPCK;

	options.c_cc[VTIME] = 0;	// 15 seconds
	options.c_cc[VMIN] = 4;//0;

	tcflush(m_iFd, TCIFLUSH);	/* Update the options and do it NOW */
	if(tcsetattr (m_iFd, TCSANOW, &options) != 0)
	{
		perror("SetupSerial 3");
		return -1;
	}

	return 0;
}

/**
 *@breif 打开串口
 */
int CPtzCtrl::OpenDev(char *Dev)
{
	//int fd = open (Dev, O_RDWR);	//| O_NOCTTY | O_NDELAY
	m_iFd = open(Dev, O_RDWR | O_NOCTTY);
	if(-1 == m_iFd)
	{	 /*设置数据位数 */
		perror("Can't Open Serial Port");
		return -1;
	}
	else
	{
		return m_iFd;
	}
}

int CPtzCtrl::Open( int nBaudRate, int nStopBits, int nDataBits, int nParity )
{
	if ( m_iFd != 0 )
	{
		close(m_iFd);
		m_iFd = 0;
	}


	try
	{

		if ( OpenDev( (char*)DEV_RS485 )> 0 )
		{
		  set_speed( nBaudRate );

		  switch( nParity )
		  {
			case 0:
				nParity = 'N';
				break;
			case 1:
				nParity = 'E';
				break;
			case 2: 
				nParity = 'O';
				break;
			default:
				nParity = 'N';
				break;
		  };
		  set_Parity( nDataBits, nStopBits, nParity );
			
		  _DEBUG_("open serail port ok %d:%d%c%d.", nBaudRate, nDataBits, nParity, nStopBits );
		}
		else return -1;

	}
	catch (const char * msg)
	{
		_DEBUG_("open serail port error %d:%d%c%d.", nBaudRate, nDataBits, nParity, nStopBits );
	}
	return 0;
}





