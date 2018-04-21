#pragma once

#include <pthread.h>
#include "PublicDefine.h"



typedef int (*CB_AlarmNotify)( char *szAlarmInId, int nAlarminId, int nStatus, void *pObj );

enum GuardStatus
{
	
	GuardStatus_ResetGurad = 0x00,
	GuardStatus_SetGuard = 0x01, 
};

class CAlarmCtrl
{
public:
	CAlarmCtrl();
	
	virtual ~CAlarmCtrl();

	void GetAlarmSettings( AlarmSettings &alarmset );

	void SetAlarmSettiings( AlarmSettings alarmset );

	int SetAlarmOutStatus( int iChannelNo, int nStatus ); //0: open 1: close;

	void SetAlarmNotifyCB( CB_AlarmNotify callback, void* pCaller );

	void SetAlarmGuard( char *szId, int nStatus );

	void SetAlarmGuard( int iChannelNo, int nStatus );


	
	void TriggerAlarmOut( int iChannelNo );

	
	void ClearAlarmIn( char *szId );

	
	void ClearAlarmOut( int iChannelNo );
	
private:

	static void *AlarmProc( void *arg );

	AlarmSettings m_alarmset;

	unsigned int tl_get_alarm_input(void);
	
	int  tl_set_alarm_out(int channel, int val);
	
	void AlarmLoop();
	
	int m_iExitFlag;
	int m_iAlarmFd;
	time_t m_tmAlarm[MAXALARMINCHN];
	unsigned int m_uiAlarmInStatus[MAXALARMINCHN];
	unsigned int m_uiAlarmOutStatus[MAXALARMOUTCHN];
	int m_nAlarmGuard[MAXALARMINCHN];
	void *m_pCaller;
	CB_AlarmNotify m_CBAlarmNotify;
	pthread_t m_th;
};


