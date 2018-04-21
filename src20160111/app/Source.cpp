
#include "Source.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

CSource::CSource( char *szUrl )
{
	strncpy( m_szUrl, szUrl, 255 );

	
	m_funcOnRecvCB = NULL;
	m_pCbObj = NULL;
    m_iRtpFlag = 0;
}
CSource::~CSource()
{

}



void CSource::SetRecvCB(  OnRecvCB callback, void *pCbObj, int rtp)
{
	m_funcOnRecvCB = callback;
	m_pCbObj = pCbObj;
    m_iRtpFlag = rtp;
}



