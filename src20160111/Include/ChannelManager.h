
#ifndef _CHANNELMANAGER_H_
#define _CHANNELMANAGER_H_


#include "NetSource.h"
#include "Decoder.h"
#include "ChannelConfig.h"
//#include "RTSPSource.h"
#include "DecoderSetting.h"
#include "EncoderSetting.h"
#include "DecoderOsd.h"
#include "PtzCtrl.h"
#include "AlarmCtrl.h"

#include <vector>

using namespace std;

class CChannel
{

public:
	CChannel( int nChannelNo,  ChnConfigParam chnparam );
	~CChannel();

	void Open( char *szUrl, int nVideoFormat );
	
	void OpenCVBS( );
	static void OnRecv( unsigned char * buf, int nSize, void *pCbObj ); 
	char m_szUrl[1024];
private:
	CSource *m_pSrc;
	CDecoder *m_pDecoder;
	ChnConfigParam m_chnConfigParam;
	int m_nChannelNo;
};


class CChannelManager
{
public:
	CChannelManager(CDecoderSetting *pDecSet, CEncoderSetting* pEncSet);
	~CChannelManager();
	void Open( int nChannelNo, char *szUrl, int nVideoFormat, int iFPS );

	void Close( int nChannelNo );
	
	void GetChannelUrl( int nChannelNo, char *szUrl, int &nFPS );

    // OSD
    long SetDecOsdMode();
    long SetDecOsd(int chnno, void* pData);
    long SetDecDisplay(void* pData);
    long ClearDecOsd(int chnno);
    long StopEncoderOfDec(int chnno);
    long SetMonitorId(int chnno, int mid);

	static long OnDDUChanged( const struDynamicDecUrl* pDDU, void *pCBObj );
    static long OnDecOsdShowChanged( struDecOsdInfo* pOsd, void *pCBObj );
	static long OnDisplayChanged( const struDisplayResolution* pDR, void *pCBObj );
    static long OnOsdChange(int chnno, void* pData, void* param);
    static long OnNetChange(int chnno, void* pData, void* param);
    static long OnParamChange(int chnno, void* pData, void* param);
    static long OnChannelChange(int chnno, void* pData, void* param);
    static long OnPtzChange(int chnno, void* pData, void* param);
    static long OnPtzPortChange(int chnno, void* pData, void* param);
    static long OnAlarminChange(int chnno, void* pData, void* param);
    static long OnAlarmoutChange(int chnno, void* pData, void* param);

	void SetEncNet(int nChannelNo, EncNetParam encnet );

	int SetEncNet( char *szDeviceId,
						int nStreamNo, 
						EncNetParam encnet );


	void SetEncParam( int nChannelNo, 
							EncoderParam encset );

	int SetEncParam( char *szDeviceId,
						int nStreamNo, 
						EncoderParam encset );

	
	void GetEncNet(int nChannelNo, 
					  int nIndex, 
					EncNetParam &encnet );

	void StopEncNet( int nChannelNo, int iIndex );

	
	void Ptz( int nChannelNo, 
								int nPtzAction, 
								int nParam1, 
								int nParam2 );

	int Ptz(  char *szDeviceId,
							int nPtzAction, 
							int nParam1, 
							int nParam2 );



	void SetSysTime( int nYear, 
						int nMonth, 
						int nDay, 
						int nHour, 
						int nMin, 
						int nSec );
	
	void SaveUrl( int nChannelNo, char *szUrl, int nFPS  );
    void OnDataForwardport(char* szData, int iLen);

	void AddProtocols( void *pProtocol );


	
	static int OnAlarm( char *szAlarmInId, int nAlarminId, int nStatus, void *pObj );
	
	void SetGuard( char *szDeviceId );

	void ResetGuard( char *szDeviceId );

	
	void ClearAlarm( char *szDeviceId );
    
	CDecoderSetting *m_pDecSet;
    CEncoderSetting *m_pEncSet;
private:
	CChannel *m_pChannel[MAXDECCHANN];
    CDecoderOsd *m_pDecoderOsd;
	CChannelConfig *m_pChannelConfig;

	void OpenEnc( int nChannelNo );

	CHiEncoder *m_pEncoder[MAXENCCHAN];
	CPtzCtrl *m_pPtzCtrl;
	CAlarmCtrl *m_pAlarmCtrl;
	pthread_mutex_t m_savlock;
	pthread_mutex_t m_openlock;
	pthread_mutex_t m_protocollock;

	vector<void*> m_pProtocols;
};


#endif



