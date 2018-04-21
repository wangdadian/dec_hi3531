#include "OnvifRcvProtocol.h"
//#include "wsdd.nsmap"
#include "ReceiverBinding.nsmap"
#include "soapH.h"


COnvifRcvProtocol::COnvifRcvProtocol( int nPort, CChannelManager *pChnManager ) : CProtocol( nPort, pChnManager )
{
    m_iExitFlag = 0;
    pthread_mutex_init( &m_lock, NULL );
    m_iExitFlag = 0;
    m_threceiverWorker = 0;
    m_thdiscoveryWorker = 0;
    m_pSoap_Receiver = NULL;
    m_pSoap_discovery = NULL;
    m_iChnno = 0;

    InitReceiver();
    SetCB_SetMediaUri(SetMediaUri,(void*)this);
    SetCB_SetNetIp(SetNetIp, (void*)this);
    SetCB_GetNetGW(GetNetGW, (void*)this);
    SetCB_SetNetGW(SetNetGW, (void*)this);
    SetCB_GetHostname(GetHostname, (void *)this);
    SetCB_SetHostname(SetHostname, (void *)this);
    SetCB_GetSystime(GetSystime, (void *)this);
    SetCB_SetSystime(SetSystime, (void *)this);
    SetCB_GetNtp(GetNtp, (void *)this);
    SetCB_SetNtp(SetNtp, (void *)this);
    SetCB_GetModel(GetModel, (void *)this);
    SetCB_GetSerial(GetSerial, (void *)this);
    SetCB_GetBuildtime(GetBuildtime, (void *)this);

    pthread_create(&m_threceiverWorker, NULL, receiverThWorker, (void*)this);
    pthread_create(&m_thdiscoveryWorker, NULL, discoveryThWorker, (void*)this);
}

COnvifRcvProtocol::~COnvifRcvProtocol() 
{
    m_iExitFlag = 1;
    if( m_thdiscoveryWorker != 0 )
    {
        pthread_join(m_thdiscoveryWorker, NULL);
        m_thdiscoveryWorker = 0;
    }
    if( m_threceiverWorker != 0 )
    {
        pthread_join(m_threceiverWorker, NULL);
        m_threceiverWorker = 0;
    }

    pthread_mutex_destroy( &m_lock );
}

int COnvifRcvProtocol::bind_server_udp1(int server_s, short port)
{
	struct sockaddr_in local_addr;
	memset(&local_addr,0,sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(port);
	return bind(server_s,(struct sockaddr*)&local_addr,sizeof(local_addr));

}
int COnvifRcvProtocol::create_server_socket_udp()
{
    int server_udp;
	unsigned char one = 1;
	int sock_opt = 1;
	
	//server_udp = socket(PF_INET, SOCK_DGRAM, 0);
    server_udp = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_udp == -1) {
        printf("unable to create socket\n");
    }

    /* reuse socket addr */
    if ((setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
                    sizeof (sock_opt))) == -1) {
        printf("setsockopt\n");
    }
    if ((setsockopt(server_udp, IPPROTO_IP, IP_MULTICAST_LOOP,
                       &one, sizeof (unsigned char))) == -1) {
        printf("setsockopt\n");
    }

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

	if(setsockopt(server_udp,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))==-1){
		perror("memberchip error\n");
	}

    return server_udp;
}

void* COnvifRcvProtocol::receiverThWorker(void *param)
{
    if(param == NULL)
    {
        return NULL;
    }
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
	
	int retval=0;
	int fault_flag = 0;
    int iRet = 0;
	while(pThis->m_iExitFlag == 0)
    {
        pThis->m_pSoap_Receiver = soap_new();
		soap_init(pThis->m_pSoap_Receiver);
        soap_set_namespaces(pThis->m_pSoap_Receiver, namespaces); 
		if (!soap_valid_socket(soap_bind(pThis->m_pSoap_Receiver, NULL, 8080, 100)))
		{	 
			soap_print_fault(pThis->m_pSoap_Receiver, stdin);
		}
        iRet= soap_accept(pThis->m_pSoap_Receiver);  
        if (iRet < 0) {  
            soap_print_fault(pThis->m_pSoap_Receiver, stdin);  
            return NULL;  
        }  
		printf("soap_serve receiver starting..\n");
		retval = soap_serve(pThis->m_pSoap_Receiver); //阻塞在这里
		printf("retval=%d\n",retval);
		if(retval && !(fault_flag))
		{
			fault_flag = 1;
		}
		else if(!retval)
		{
			fault_flag = 0;
		}
        soap_destroy(pThis->m_pSoap_Receiver);
        soap_end(pThis->m_pSoap_Receiver);
        soap_done(pThis->m_pSoap_Receiver);
        free(pThis->m_pSoap_Receiver);
        pThis->m_pSoap_Receiver = NULL;
	}

    return NULL;
}

void* COnvifRcvProtocol::discoveryThWorker(void *param)
{
    if(param == NULL)
    {
        return NULL;
    }
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;

	int server_udp = -1;
	int retval=0;
	int fault_flag = 0;
	short port = 3702;


	while(pThis->m_iExitFlag == 0)
    {
        pThis->m_pSoap_discovery = soap_new();
        server_udp = pThis->create_server_socket_udp();
        pThis->bind_server_udp1(server_udp, port);

        soap_init1(pThis->m_pSoap_discovery, SOAP_IO_UDP);
		pThis->m_pSoap_discovery->master = server_udp;
		pThis->m_pSoap_discovery->socket = server_udp;
		pThis->m_pSoap_discovery->errmode = 0;
		pThis->m_pSoap_discovery->bind_flags = 1;
		if (!soap_valid_socket(soap_bind(pThis->m_pSoap_discovery, NULL, port, 100)))
		{	 
			soap_print_fault(pThis->m_pSoap_discovery, stdin);
		}
		fprintf(stderr,"soap_serve remote discovery starting..\n");
		retval = soap_serve(pThis->m_pSoap_discovery); //阻塞在这里
		printf("retval=%d\n", retval);
		if(retval && !(fault_flag))
		{
			fault_flag = 1;
		}
		else if(!retval)
		{
			fault_flag = 0;
		}

        soap_destroy(pThis->m_pSoap_discovery);
        soap_end(pThis->m_pSoap_discovery);
        soap_done(pThis->m_pSoap_discovery);
        free(pThis->m_pSoap_discovery);
        pThis->m_pSoap_discovery = NULL;
        if(server_udp != -1)
        {
            close(server_udp);
            server_udp = -1;
        }
	}

    return NULL;
}

int COnvifRcvProtocol::SetMediaUri(char* szUri, int chnno, void* param)
{
    if(param == NULL)
    {
        return NULL;
    }
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    _DEBUG_("Set Receiver Media URI = %s on channel %d", szUri, chnno);
    // 停止解码器
    pThis->m_pChnManager->Close(chnno);
    pThis->m_iChnno = 0;
    // 启动解码器
    if(!szUri || !*szUri)
    {
        // URI空，不再启动解码器
        _DEBUG_("stop decoder.");
        return 0;
    }
    int iFps = 30;
    pThis->m_pChnManager->SaveUrl(chnno, szUri, iFps);
    pThis->m_pChnManager->Open( chnno, szUri, 0xff, iFps);
    pThis->m_iChnno = chnno;
    _DEBUG_("start decoder [URL=%s] on channel %d", szUri, chnno);
    return 0;
}

int COnvifRcvProtocol::SetNetIp(char* ip, char* netmask, void* param)
{
    if(ip == NULL || netmask==NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->SetNetIp(ip, netmask);

}

int COnvifRcvProtocol::GetNetGW(void* gw, void* param)
{
    if(gw == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->GetGateway((char*)gw);
}

int COnvifRcvProtocol::SetNetGW(void* gw, void* param)
{
    if(gw == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->SetGateway((char*)gw);

}

int COnvifRcvProtocol::GetHostname(void* name, void* param)
{
    if(name == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->GetHostname((char*)name, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::SetHostname(void* name, void* param)
{
    if(name == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->SetHostname((char*)name, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::GetSystime(void* time, void* param)
{
    if(time == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->GetSysTime((char*)time, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::SetSystime(void* time, void* param)
{
    if(time == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->SetSysTime((char*)time, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::GetNtp(void* ntp, void* param)
{
    if(ntp == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    struNtpInfo *ntpinfo = (struNtpInfo*)ntp;
    return pThis->m_pChnManager->m_pDecSet->GetNtpInfo(ntpinfo);
}

int COnvifRcvProtocol::SetNtp(void* ntp, void* param)
{
    if(ntp == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->SetNtp((char*)ntp, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::GetBuildtime(void* time, void* param)
{
    if(time == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->GetDevBuildtime((char*)time, (void*)pThis->m_pChnManager->m_pDecSet);
}

int COnvifRcvProtocol::GetModel(void* model, void* param)
{
    if(model == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    int imodel = 0;
    char szmodel[40] = {0};
    char hostname[80] = {0};
    pThis->m_pChnManager->m_pDecSet->GetHostname(hostname, (void*)pThis->m_pChnManager->m_pDecSet);
    if(pThis->m_pChnManager->m_pDecSet->GetDeviceModel(imodel, (void*)pThis->m_pChnManager->m_pDecSet) != 0)
    {
        return -1;
    }
    switch(imodel)
    {
        case 0:
            sprintf(szmodel, "%s", "SD-Encoder");
            break;
        case 1:
            sprintf(szmodel, "%s", "HD-Encoder");
            break;
        case 2:
            sprintf(szmodel, "%s", "Decoder");
            break;
        default:
            sprintf(szmodel, "%s", "Device");
            break;
    };
    sprintf((char*)model, "%s@%s", szmodel, hostname);
    return 0;
}

int COnvifRcvProtocol::GetSerial(void* serial, void* param)
{
    if(serial == NULL)
        return -1;
    COnvifRcvProtocol *pThis = (COnvifRcvProtocol*)param;
    return pThis->m_pChnManager->m_pDecSet->GetDeviceSN((char*)serial, (void*)pThis->m_pChnManager->m_pDecSet);
}







