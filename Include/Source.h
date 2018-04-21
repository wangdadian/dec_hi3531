#ifndef SOURCE_H_
#define SOURCE_H_

typedef void (*OnRecvCB)( unsigned char * buf, int nSize, void *pCbObj );


class CSource
{
public:
	CSource( char *szUrl );
	virtual ~CSource();

	
	void SetRecvCB(  OnRecvCB callback, void *pCbObj, int rtp=0);

protected:
	char m_szUrl[255];
	
	OnRecvCB m_funcOnRecvCB;
	void *m_pCbObj;
	int m_nChannelNo;
    int m_iRtpFlag;// 0-no rtp, 1-rtp
};

#endif


