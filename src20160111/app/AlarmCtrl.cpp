
#include "AlarmCtrl.h"
#include "PublicDefine.h"

CAlarmCtrl::CAlarmCtrl()
{
	m_iAlarmFd = 0;
	m_iAlarmFd = open( DEV_TL9800, O_RDWR, 0 );
		m_iExitFlag = 0;

	m_CBAlarmNotify = NULL;
	m_pCaller = NULL;
	
	if ( m_iAlarmFd <= 0 )
	{
		_DEBUG_("open DEV_TL9800 device %s error", DEV_TL9800 );
		perror( "open DEV_TL9800 device error" );
		return;
	}
	
	unsigned int buf[5] = {0};
	buf[0] = 1; 
	if(ioctl(m_iAlarmFd, TL_RS485_CTL, buf) < 0)
	{
		_DEBUG_("RS485_CTL error");
		perror("tl_rs485_ctl: RS485_CTL error");
	}

	pthread_create(&m_th, NULL, AlarmProc, (void*)this );
}

void CAlarmCtrl::SetAlarmGuard( char *szId, int nStatus )
{

	_DEBUG_( "set alarm gurad:%s stauts:%d", szId, nStatus );
	for ( int i = 0 ; i < MAXALARMINCHN; i++ )
	{
		if ( strcmp( m_alarmset.alarmin[i].szId, szId ) == 0 )
		{
			m_alarmset.alarmin[i].iEnable = nStatus;
		}
	}
}

void CAlarmCtrl::SetAlarmGuard( int iChannelNo, int nStatus )
{
	
	if ( iChannelNo < 0 && iChannelNo >= MAXALARMINCHN )
	{
		_DEBUG_("invalid channel no:%d", iChannelNo );
		return;
	}

	_DEBUG_( "set alarm gurad:%d stauts:%d", iChannelNo, nStatus );
	m_alarmset.alarmin[iChannelNo].iEnable = nStatus;
}

CAlarmCtrl::~CAlarmCtrl()
{
	m_iExitFlag = 1;
	if ( m_iAlarmFd > 0 )
	{
		close( m_iAlarmFd );
	}

	if ( m_th != 0 )
	{
		pthread_join( m_th, NULL );		
		m_th = NULL;
	}
}

void CAlarmCtrl::GetAlarmSettings( AlarmSettings &alarmset )
{
	memcpy( &alarmset, &m_alarmset, sizeof( AlarmSettings ));
}

void CAlarmCtrl::SetAlarmSettiings( AlarmSettings alarmset )
{
	memcpy( &m_alarmset, &alarmset, sizeof( AlarmSettings ) );
}

void CAlarmCtrl::TriggerAlarmOut( int iChannelNo )
{
	_DEBUG_("trigger alarm out channel:%d", iChannelNo );
	if ( iChannelNo >= MAXALARMOUTCHN || iChannelNo < 0 )
	{
		_DEBUG_("invalid channel:%d", iChannelNo );
		return ;
	}

	if ( m_alarmset.alarmout[iChannelNo].iNormalStatus > 0 )
	{
		SetAlarmOutStatus(iChannelNo,0);
	}
	else
		SetAlarmOutStatus(iChannelNo, 1);
}

void CAlarmCtrl::ClearAlarmIn( char *szId )
{
	_DEBUG_("clear alarmin channel:%s", szId );
	for ( int i = 0 ;i < MAXALARMINCHN; i++ )
	{
		if ( strcmp( m_alarmset.alarmin[i].szId, szId ) == 0 )
		{
			ClearAlarmOut(m_alarmset.alarmin[i].iTrigerAlarmOut);
		}
	}
}

void CAlarmCtrl::ClearAlarmOut( int iChannelNo )
{
	if ( iChannelNo >= MAXALARMOUTCHN || iChannelNo < 0 )
	{
		return ;
	}

	if ( m_alarmset.alarmout[iChannelNo].iNormalStatus > 0 )
	{
		SetAlarmOutStatus(iChannelNo,1);
	}
	else
		SetAlarmOutStatus(iChannelNo, 0);

}

int CAlarmCtrl::SetAlarmOutStatus( int iChannelNo, int nStatus ) //0: open 1: close;
{

	//_DEBUG_("set alarm out status channel:%d, status:%d", iChannelNo, nStatus );
	if ( iChannelNo >= MAXALARMOUTCHN || iChannelNo < 0 )
	{
		return -1;
	}
	
	if ( nStatus != 0 ) nStatus = 1;
	else nStatus = 0;
	tl_set_alarm_out( iChannelNo, nStatus );
	return 0;
}

void CAlarmCtrl::SetAlarmNotifyCB( CB_AlarmNotify callback, void* pCaller )
{
	m_CBAlarmNotify = callback;
	m_pCaller = pCaller;
}

void CAlarmCtrl::AlarmLoop()
{
	unsigned int uiAlarmInStatus = tl_get_alarm_input();
	
	time_t tmnow;
	tmnow = time(&tmnow);
	for ( int i = 0 ;i < MAXALARMINCHN; i++ )
	{
		m_tmAlarm[i] = tmnow;
		m_uiAlarmInStatus[i] = 0;
		unsigned int nOffset = 1<<i;
		m_uiAlarmInStatus[i] = ( nOffset ) & uiAlarmInStatus;
	}
	
	while(m_iExitFlag == 0 )
	{
		uiAlarmInStatus = tl_get_alarm_input();
		unsigned int nNewStatus[MAXALARMINCHN]={0};
		tmnow = time(&tmnow);
	
		for ( int i = 0 ;i < MAXALARMINCHN; i++ )
		{
			unsigned int nOffset = 1<<i;
			nNewStatus[i] = ( nOffset ) & uiAlarmInStatus;
			if ( m_uiAlarmInStatus[i] != nNewStatus[i] && 
				tmnow - m_tmAlarm[i] >= m_alarmset.alarmin[i].iStableTime )
			{
				if ( m_CBAlarmNotify != NULL && m_alarmset.alarmin[i].iEnable > 0 )
				{
					m_CBAlarmNotify( m_alarmset.alarmin[i].szId, i, nNewStatus[i], m_pCaller );

					if ( nNewStatus[i] != m_alarmset.alarmin[i].iNormalStatus ) //abnormal×´Ì¬£¬ ±¨¾¯·¢Éú
					{
						TriggerAlarmOut(m_alarmset.alarmin[i].iTrigerAlarmOut );
					}
					else //normal×´Ì¬£¬ ±¨¾¯»Ö¸´
					{
						ClearAlarmOut(m_alarmset.alarmin[i].iTrigerAlarmOut  );
					}
				}
				m_uiAlarmInStatus[i] = nNewStatus[i];
			}
			else if ( m_uiAlarmInStatus[i] == nNewStatus[i] )
			{
				m_tmAlarm[i] = tmnow;
			}
		
			m_uiAlarmInStatus[i] = uiAlarmInStatus;
		}

		//_DEBUG_("alarm status:0x%x\n", uiAlarmInStatus );
		usleep(100*1000);
	};


}
void *CAlarmCtrl::AlarmProc( void *arg )
{
	CAlarmCtrl *pThis = (CAlarmCtrl*)arg;
	pThis->AlarmLoop();
	//pthread_detach( pthread_self());
	return NULL;
}

unsigned int CAlarmCtrl::tl_get_alarm_input(void)
{
	unsigned int buf = 0;
	if(ioctl(m_iAlarmFd, TL_ALARM_IN, &buf) < 0)
	{
		_DEBUG_("lib_alarm.c@tl_get_alarm_input: TL_ALM_IN error\n");
		return 0;
	}
	return (buf & 0xFF);
}
int CAlarmCtrl:: tl_set_alarm_out(int channel, int val)
{
	unsigned int buf[5] = {0};
	buf[0] = channel;
	buf[1] = val;
	
	if(ioctl(m_iAlarmFd, TL_ALARM_OUT, buf) < 0)
	{
		_DEBUG_("lib_alarm.c@tl_set_alarm_out: TL_ALARM_OUT error\n");
		return -1;
	}
	return 0;
}





