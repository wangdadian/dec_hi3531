
#include "AT200Protocol.h"
#include <string.h>


CAT200Protocol::CAT200Protocol( int nPort, CChannelManager *pChnManager  ) : CProtocol( nPort, pChnManager )
{
	m_iExitFlag = 0;
	m_iCmdIndex = 0;
	pthread_mutex_init( &m_lock, NULL );
	m_pServer = new CSyncNetServer( CAT200Protocol::THWorker , (void*)this );
	m_pServer->StartServer( nPort);
}

CAT200Protocol::~CAT200Protocol()
{
	m_iExitFlag = 1;
	
	if ( m_pServer != NULL)
	{
		delete m_pServer;
		m_pServer = NULL;
	}
	pthread_mutex_destroy( &m_lock );
}

int CAT200Protocol::StopEncoder(int chnno)
{
    //_DEBUG_(" CALLED!");
    return 0;
}

void CAT200Protocol::THWorker(CSyncServerNetEnd *net, void *p)
{
	_DEBUG_("client connect:%s\n", net->getRemoteAddr());
    if(p == NULL)
    {
        _DEBUG_("p==NULL! return\n");
        return ;
    }
	CAT200Protocol *pThis = ( CAT200Protocol*)p;
    char tmp[60]={0};
    int ret = 0;
	while( pThis->m_iExitFlag == 0 )
	{
		if ( !net->IsOK() )
        {
            _DEBUG_("net is not ok!");
            break;
        }
		memset(tmp, 0, sizeof(tmp));
		ret = net->RecvNT( tmp, sizeof(tmp), 100 );
		if ( ret > 0 )
		{
			pThis->ProcessBuffer(tmp, ret);
		}
		else if ( ret < 0 )
        {
            _DEBUG_("recv data error!");
            break;
        }
        else
        {
            //_DEBUG_("no data received!");
        }
	};

	_DEBUG_("client disconnect:%s\n", net->getRemoteAddr());

}




int CAT200Protocol::CheckSwitch(char *cmd)
{
	unsigned int m_node = 0, node = 0, f_node = 0, out = 0,iret = -1;
	uint64_t in = 0ULL;
	char pszRtn[50];

	printf( "CheckSwitch:cmd={%s}", cmd);

	int ret = sscanf(cmd, "#%d,%d,[J%d==C%llu]", &node, &f_node, &out, &in);	//切换本地干线

	if ( ret != 4 )
	{
		ret = sscanf(cmd, "#%d,%d,%d,[J%d==C%llu]", &node, &m_node, &f_node, &out, &in);	//切换远程干线
	
	}

	if ( ret == 4 || ret == 5 )
	{		
		printf( "CheckSwitch:[%llu] ==> [%d] \n", in,out);
		m_pChnManager->SaveUrl(1, (char*)"cvbs/1", 25);
		return iret;
	}

	
	return iret;
}

void CAT200Protocol::ProcessBuffer(char *buf, int &iSize)
{
	int iRv=0;
	//g_pLog->Info(m_iLogModule,"ProcessBuffer:buf=%s", buf);
	
	for (int i=0;i<iSize;i++)
	{
		if(buf[i] == '#')
		{
			m_iCmdIndex = 0;
			m_szCmd[m_iCmdIndex] = buf[i];
		}
		else if(buf[i] == 0x0D)
		{
			m_szCmd[++m_iCmdIndex] = '\0';
			iRv = CheckSwitch(m_szCmd);
			
			printf("ProcessBuffer:SwitchRtn=%d\n", iRv);
			if ( iRv >= 0) 
			{
				m_iCmdIndex = 0;
				continue;
			}
			
			iRv = CheckCtrl(m_szCmd);
			printf("ProcessBuffer:CtrlRtn=%d\n", iRv);
			if ( iRv >= 0)
			{
				m_iCmdIndex = 0;
				continue;
			}
		}
		else
		{
			if(m_iCmdIndex >= 127)
				m_iCmdIndex = 0;
			else
				m_szCmd[++m_iCmdIndex] = buf[i];
		}

	}
	
}


int CAT200Protocol::CheckCtrl(char *cmd)
{
	char op = ' ';
	unsigned int node = 0, f_node = 0,prm = 0;
	uint64_t cam=0ULL;
	int iret = -1;
	
	printf( "CheckCtrl:cmd=%s\n", cmd);
	
	int ret = sscanf(cmd, "#%d,%d,[C%llu=%c%d]", &node, &f_node, &cam, &op, &prm);
	if ( ret!=5)
	{
		ret = sscanf(cmd, "#0,%d,%d,[C%llu=%c%d]", &node, &f_node, &cam, &op, &prm);
		if ( ret!=5 ) 
		{
			printf( "CheckCtrl:cmd=[%s] 格式错误!", cmd);	
			return 0;
		}
	}

	return iret;	
}



