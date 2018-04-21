#include "Protocol.h"

CProtocol::CProtocol( int nPort, CChannelManager *pChnManager  ) 
{
	m_nPort = nPort;
	m_pChnManager = pChnManager;
	
}


CProtocol::~CProtocol()
{
	
}

int CProtocol::OnAlarm( char *szAlarmInId, int nAlarminId, int nStatus )
{
	return 0;
}

int CProtocol::StopEncoder(int chnno)
{
    //_DEBUG_("############ CProtocol::StopEncoder CALLED [%d]!", chnno);
    return 0;
}

//读取系统实际IP
void CProtocol::get_ip(char *ip)
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


void CProtocol::NTOHS( unsigned short &ut )
{
	ut = ntohs(ut);
}

void CProtocol::HTONS( unsigned short &ut )
{
	ut = htons(ut);
}

void CProtocol::NTOHL( unsigned int &lt )
{
	lt = ntohl( lt );
}

void CProtocol::HTONL( unsigned int &lt )
{
	lt = htonl( lt );
}



