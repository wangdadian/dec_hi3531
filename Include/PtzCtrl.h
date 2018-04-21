#pragma once 
#include "SerialPort.h"
#include "PublicDefine.h"

enum PtzDeviceType
{
	PtzDeviceType_PELCOD = 0x00, 
};


class CPtzCtrlSession
{
public:
	CPtzCtrlSession(void *pCtrl);
	virtual ~CPtzCtrlSession();

	
	int Ptz(  int nPtzAction, 
			   int iParam1,
			   int iParam2 );

	
	void SetConfig( PtzSetting ptzset );

private:

	int m_LastCmd, m_PanSpd, m_TiltSpd;		

	int CheckSum(unsigned char *pdata, int len);
	
	long GetPelcoDCode( int nAddressCode, int nPtzAction, 
						   int iParam1, 
						   int iParam2, 
					      unsigned char *pdata /*7bytes*/, 
					       int &nLen);
	
	void *m_pCtrl;
	PtzSetting m_ptzsetting;
};

class CPtzCtrl
{
public:
	CPtzCtrl( int nBaudRate, int nStopBits, int nDataBits, int nParity );
	virtual ~CPtzCtrl();
	
	void SetPtzType( int nChannelNo, PtzSetting ptzset );

	void WriteData( char *buf, int len);

	
	int Ptz( int nChannel, 
			   int nPtzAction, 
			   int iParam1,
			   int iParam2 );
	
	int Open( int nBaudRate, int nStopBits, int nDataBits, int nParity );

private:

	
	int set_speed(  int speed);
	int set_Parity( int databits, int stopbits, int parity);
	int OpenDev(char *Dev);
	int m_iFd;
	CPtzCtrlSession *m_ptzctrlsess[MAXENCCHAN];

};


