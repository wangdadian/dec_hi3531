
#include "soapStub.h"
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <linux/hdreg.h>
#include <netinet/ip.h> 
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/ip.h> 
#include <netinet/ip_icmp.h> 
#include <linux/if_ether.h> 
#include  <linux/sockios.h>
#include  <linux/ethtool.h>
#include <time.h>
#include <sys/time.h>

const unsigned int RECEIVER_COUNT  = 4;
struct _trv__GetReceiversResponse * g_pGetReceiversResponse = NULL;
struct _tds__GetNetworkInterfacesResponse* g_pGetNetworkInterfacesResponse = NULL;
struct _tds__GetHostnameResponse *g_pGetHostnameResponse = NULL;
struct _tds__GetSystemDateAndTimeResponse * g_pGetSystemDateAndTimeResponse = NULL;
struct _tds__GetDeviceInformationResponse * g_pGetDeviceInformationResponse = NULL;
struct _tds__GetNTPResponse * g_pGetNTPResponse = NULL;
struct _tds__GetNetworkDefaultGatewayResponse* g_pGetNetworkDefaultGatewayResponse = NULL;

SetMediaUriCB_T g_fSetMediaUri = NULL;
SetNetIpCB_T g_fSetNetIp = NULL;

SetCB_T g_fSetNetGW = NULL;
GetCB_T g_fGetNetGW = NULL;

GetCB_T g_fGetHostname = NULL;
SetCB_T g_fSetHostname = NULL;

GetCB_T g_fGetNtp = NULL;
SetCB_T g_fSetNtp = NULL;

GetCB_T g_fGetSystime = NULL;
SetCB_T g_fSetSystime = NULL;

GetCB_T g_fGetBuildtime = NULL;
GetCB_T g_fGetModel = NULL;
GetCB_T g_fGetSerial = NULL;

void* g_pObjOnvif = NULL;

enum NetInfoType
{
	NetInfoType_MAC = 0x00,
	NetInfoType_IP = 0x01,
	NetInfoType_NM = 0x02, // netmask
	NetInfoType_BC = 0x03, //broadcast
	NetInfoType_MTU = 0x04, //mtu
	NetInfoType_FLAG = 0x05, // flags
	NetInfoType_ETHTOOL = 0x06,
};
#define MAXINTERFACES 8

int GetLocalNetInfo( struct ifreq *buf, enum NetInfoType iType )
{
	int fd, intrface, retn = 0;
	//struct ifreq buf[MAXINTERFACES];
	//struct arpreq arp;
	struct ifconf ifc;
    struct ifreq *buf_tmp;
    buf_tmp = malloc(sizeof(struct ifreq)*MAXINTERFACES);

    int i, j = 0;
	for ( i = 0; i < MAXINTERFACES; i++ )
	{
		memset( &buf_tmp[i], 0, sizeof( struct ifreq ) );
	}
    
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		ifc.ifc_len = sizeof(struct ifreq) * MAXINTERFACES;
		ifc.ifc_buf = (caddr_t) buf_tmp;
		
		if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
		{
			intrface = ifc.ifc_len / sizeof(struct ifreq);
            j = 0;
			//printf( "interface num is intrface=%d\n", intrface);
			while (j < intrface)
			{
			    if ( strcmp( buf_tmp[j].ifr_name, "lo" ) == 0 )
                {
                    j++;
                    continue;
                }
				//printf( "net device %s. ", buf_tmp[j].ifr_name);
				/*Jugde whether the net card status is promisc */
				if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf_tmp[j])))
				{
					if (buf_tmp[j].ifr_flags & IFF_PROMISC)
					{
						//printf( "the interface is PROMISC \n");
					}
				}
				else
				{
					char str[256];

					sprintf(str, "cpm: ioctl device %s.\n ", 
						buf_tmp[j].ifr_name);

					perror(str);
				}

				/*Jugde whether the net card status is up */
				if (buf_tmp[j].ifr_flags & IFF_UP)
				{
					//printf( "the interface status is UP. ");
				}
				else
				{
				    j++;
                    continue;
					//printf( "the interface status is DOWN. ");
				}

				switch( (int)iType )
				{
					case (int)NetInfoType_FLAG:
					{
						/*Get flags of the net card */
						if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf_tmp[j])))
						{

						}
						else
						{
							char str[256];
							sprintf(str, "cpm: ioctl device %s. ", buf_tmp[j].ifr_name);
							perror(str);
						}
						break;
					}
					case (int)NetInfoType_MAC:
					{
						/*Get HW ADDRESS of the net card */
						if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf_tmp[j])))
						{
							/*printf( "HW address is: %02x:%02x:%02x:%02x:%02x:%02x. \n",
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[0],
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[1],
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[2],
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[3],
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[4],
								(unsigned char) buf_tmp[j].ifr_hwaddr.sa_data[5]);
								*/
						}
						else
						{
							char str[256];
							sprintf(str, "cpm: ioctl device %s. ", buf_tmp[j].ifr_name);
							perror(str);
						}
						break;
					}
					case (int)NetInfoType_IP:
					{
						/*Get IP of the net card */
						if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf_tmp[j])))
						{
							//printf( "IP address is: %s.\n", inet_ntoa(((struct sockaddr_in *)(&buf_tmp[j].ifr_addr))-> sin_addr));
							
						}
						else
						{
							char str[256];

							sprintf(str, "cpm: ioctl device %s. ",
								buf_tmp[j].ifr_name);

							perror(str);
						}
						break;
					}
                    case (int)NetInfoType_BC:
					{
						/*Get broadcast of the net card */
						if (!(ioctl(fd, SIOCGIFBRDADDR, (char *) &buf_tmp[j])))
						{
							
							//printf( "broadcast is: %s.\n", inet_ntoa(((struct sockaddr_in *)(&buf_tmp[j].ifr_addr))-> sin_addr));
							
						}
						else
						{
							char str[256];

							sprintf(str, "cpm: ioctl device %s. ",
								buf_tmp[j].ifr_name);

							perror(str);
						}
						break;
					}
                    case (int)NetInfoType_NM:
					{
						/*Get netmask of the net card */
						if (!(ioctl(fd, SIOCGIFNETMASK, (char *) &buf_tmp[j])))
						{
							
							//printf( "netmask is: %s.\n", inet_ntoa(((struct sockaddr_in *)(&buf_tmp[j].ifr_addr))-> sin_addr));
							
						}
						else
						{
							char str[256];

							sprintf(str, "cpm: ioctl device %s. ",
								buf_tmp[j].ifr_name);

							perror(str);
						}
						break;
					}
                    case (int)NetInfoType_MTU:
					{
						/*Get mtu of the net card */
						if (!(ioctl(fd, SIOCGIFMTU, (char *) &buf_tmp[j])))
						{
							//printf( "mtu is: %d.\n", buf_tmp[j].ifr_mtu);
						}
						else
						{
							char str[256];

							sprintf(str, "cpm: ioctl device %s. ",
								buf_tmp[j].ifr_name);

							perror(str);
						}
						break;
					}
                    case (int)NetInfoType_ETHTOOL:
					{
						/*Get ethtool info of the net card */
                        // 调用者释放
                        struct ethtool_cmd *ep = malloc(sizeof(struct ethtool_cmd)); 
                        ep->cmd = ETHTOOL_GSET;
                        buf_tmp[j].ifr_data = (caddr_t)ep;
						if (!(ioctl(fd, SIOCETHTOOL, (char *) &buf_tmp[j])))
						{
							//printf( "mtu is: %d.\n", buf_tmp[j].ifr_mtu);
						}
						else
						{
							char str[256];

							sprintf(str, "cpm: ioctl device %s. ",
								buf_tmp[j].ifr_name);

							perror(str);
						}
						break;
					}

					default:
						break;
				}
                memcpy(&buf[retn], &buf_tmp[j], sizeof(struct ifreq));
                retn++;
				j++;
				//printf( "\n" );
			}
		}
		else
			perror( "cpm: ioctl ");
	}
	else
		perror( "cpm: socket ");

	close(fd);
    if(buf_tmp)
    {
        free(buf_tmp);
        buf_tmp = NULL;
    }
	return retn;
}

void get_ip(char *ip)	//读取系统实际IP
{
    struct ifreq *buf;
    buf = malloc(sizeof(struct ifreq)*MAXINTERFACES);
    int i = 0;
	for ( i = 0; i < MAXINTERFACES; i++ )
	{
		memset( &buf[i], 0, sizeof( struct ifreq ) );
	}

    int count = 0;
    count = GetLocalNetInfo( buf, NetInfoType_IP );
    
    for ( i = 0; i < count; i++ )
    {
        if ( strcmp( buf[i].ifr_name, "lo" ) == 0 )
        {
            continue;
        }
    
        if (! buf[i].ifr_flags & IFF_UP )
        {
            continue;
        }
        if(strcmp(buf[i].ifr_name, "eth0")==0)
        {
            strcpy( ip, inet_ntoa(((struct sockaddr_in *)(&buf[i].ifr_addr))-> sin_addr) );
            break;
        }
     }
    if ( buf != NULL )	
        free(buf);
}

void get_mac(unsigned char* mac)//获取MAC
{
    struct ifreq *buf;
    buf = malloc(sizeof(struct ifreq)*MAXINTERFACES);
    int i = 0;
	for ( i = 0; i < MAXINTERFACES; i++ )
	{
		memset( &buf[i], 0, sizeof( struct ifreq ) );
	}

    int count = 0;
    count = GetLocalNetInfo( buf, NetInfoType_MAC );
    
    for ( i = 0; i < count; i++ )
    {
        if ( strcmp( buf[i].ifr_name, "lo" ) == 0 )
        {
            continue;
        }
    
        if (! buf[i].ifr_flags & IFF_UP )
        {
            continue;
        }
        if(strcmp(buf[i].ifr_name, "eth0")==0)
        {
            mac[0] = (unsigned char) buf[i].ifr_hwaddr.sa_data[0];
            mac[1] = (unsigned char) buf[i].ifr_hwaddr.sa_data[1];
            mac[2] = (unsigned char) buf[i].ifr_hwaddr.sa_data[2];
            mac[3] = (unsigned char) buf[i].ifr_hwaddr.sa_data[3];
            mac[4] = (unsigned char) buf[i].ifr_hwaddr.sa_data[4];
            mac[5] = (unsigned char) buf[i].ifr_hwaddr.sa_data[5];
            break;
        }
    }
    if ( buf != NULL )	
        free(buf);

}
// 判断是否为IP地址，是返回0
int IsIP(const char* szIp)
{
    if ( NULL == szIp )
    {
        return -1;
    }
    struct in_addr addr;
    if ( inet_aton ( szIp, &addr ) == 0 )
    {
        return -1;
    }
    
    char* destIp = ( char* ) inet_ntoa ( addr );
    if ( 0 != strcmp ( szIp, destIp ) )
    {
        return -1;
    }
    
    return 0;
}
int netmask2prefix(char* netmask)
{
    int prefix = 0;
    unsigned int iNetmask = 0;
    struct in_addr in;
    inet_aton(netmask, &in);
    memcpy(&iNetmask, &in.s_addr, sizeof(unsigned int));
    iNetmask = ntohl(iNetmask);
    int i=0;
    int itmp = 0;
    for(i=0; i<sizeof(unsigned int)*8; i++)
    {
        itmp = (iNetmask<<i) & 0x80000000;
        if(!itmp)
        {
            break;
        }
        prefix++;
    }
    return prefix;
}
char* prefix2netmask(int prefix)
{
    if(prefix < 0 || prefix >32)
        return "0.0.0.0";
    
    unsigned int iNetmask = 0;
    struct in_addr in;
    int i=0;
    for(i=0; i<prefix/8; i++)
    {
        iNetmask += 0xff<<(i*8);
    }
    iNetmask += ((0xff<<(8-prefix%8) )&0xff)<<(prefix/8*8);
    
    in.s_addr = iNetmask;
    return inet_ntoa(in);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

int  __wsdd__Hello(struct soap* soap, struct wsdd__HelloType *wsdd__Hello)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __wsdd__Bye(struct soap* soap, struct wsdd__ByeType *wsdd__Bye)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __wsdd__Probe(struct soap* soap, struct wsdd__ProbeType *wsdd__Probe)
{
    printf("__wsdd__Probe\n");  
    int INFO_LENGTH = 10240;
    int SMALL_INFO_LENGTH=10240;
    char _IPAddr[INFO_LENGTH];  
    char _HwId[1024];
    char szIp[64]={0};
    unsigned char szMAC[6]={0};

    wsdd__ProbeMatchesType ProbeMatches;  
    ProbeMatches.ProbeMatch = (struct wsdd__ProbeMatchType *)soap_malloc(soap, sizeof(struct wsdd__ProbeMatchType));  
    ProbeMatches.ProbeMatch->XAddrs = (char *)soap_malloc(soap, sizeof(char) * INFO_LENGTH);  
    ProbeMatches.ProbeMatch->Types = (char *)soap_malloc(soap, sizeof(char) * INFO_LENGTH);  
    ProbeMatches.ProbeMatch->Scopes = (struct wsdd__ScopesType*)soap_malloc(soap,sizeof(struct wsdd__ScopesType));  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties = (struct wsa__ReferencePropertiesType*)soap_malloc(soap,sizeof(struct wsa__ReferencePropertiesType));  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters = (struct wsa__ReferenceParametersType*)soap_malloc(soap,sizeof(struct wsa__ReferenceParametersType));  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName = (struct wsa__ServiceNameType*)soap_malloc(soap,sizeof(struct wsa__ServiceNameType));  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType = (char **)soap_malloc(soap, sizeof(char *) * SMALL_INFO_LENGTH);  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.__any = (char **)soap_malloc(soap, sizeof(char*) * SMALL_INFO_LENGTH);  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.__anyAttribute = (char *)soap_malloc(soap, sizeof(char) * SMALL_INFO_LENGTH);  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.Address = (char *)soap_malloc(soap, sizeof(char) * INFO_LENGTH);  

    get_mac(szMAC);
    sprintf(_HwId,"urn:uuid:2419d68a-2dd2-21b2-a205-%02X%02X%02X%02X%02X%02X",szMAC[0], szMAC[1], szMAC[2], szMAC[3], szMAC[4], szMAC[5]);  

    get_ip(szIp);
    sprintf(_IPAddr, "http://%s:8080/onvif/device_service", szIp);  
    ProbeMatches.__sizeProbeMatch = 1;  
    ProbeMatches.ProbeMatch->Scopes->__item =(char *)soap_malloc(soap, 1024);  
    memset(ProbeMatches.ProbeMatch->Scopes->__item,0,sizeof(ProbeMatches.ProbeMatch->Scopes->__item));    
  
    //Scopes MUST BE  
    strcat(ProbeMatches.ProbeMatch->Scopes->__item, "onvif://www.onvif.org/type/NetworkVideoDisplay");
  
    ProbeMatches.ProbeMatch->Scopes->MatchBy = NULL;  
    strcpy(ProbeMatches.ProbeMatch->XAddrs, _IPAddr);  
    strcpy(ProbeMatches.ProbeMatch->Types, wsdd__Probe->Types);  
    printf("wsdd__Probe->Types=%s\n",wsdd__Probe->Types);  
    ProbeMatches.ProbeMatch->MetadataVersion = 1;  
    //ws-discovery规定 为可选项  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__size = 0;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceProperties->__any = NULL;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__size = 0;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ReferenceParameters->__any = NULL;  
      
    ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType[0] = (char *)soap_malloc(soap, sizeof(char) * SMALL_INFO_LENGTH);  
    //ws-discovery规定 为可选项  
    strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.PortType[0], "ttl");  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->__item = NULL;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->PortName = NULL;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.ServiceName->__anyAttribute = NULL;  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.__any[0] = (char *)soap_malloc(soap, sizeof(char) * SMALL_INFO_LENGTH);  
    strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.__any[0], "Any");  
    strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.__anyAttribute, "Attribute");  
    ProbeMatches.ProbeMatch->wsa__EndpointReference.__size = 0;  
    strcpy(ProbeMatches.ProbeMatch->wsa__EndpointReference.Address, _HwId);  
  
    /*注释的部分为可选，注释掉onvif test也能发现ws-d*/  
    //soap->header->wsa__To = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";  
    //soap->header->wsa__Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";  
    soap->header->wsa__RelatesTo = (struct wsa__Relationship*)soap_malloc(soap, sizeof(struct wsa__Relationship));  
    //it's here  
    soap->header->wsa__RelatesTo->__item = soap->header->wsa__MessageID;  
    soap->header->wsa__RelatesTo->RelationshipType = NULL;  
    soap->header->wsa__RelatesTo->__anyAttribute = NULL;  
  
    soap->header->wsa__MessageID =(char *)soap_malloc(soap, sizeof(char) * INFO_LENGTH);  
    strcpy(soap->header->wsa__MessageID,_HwId+4);  
  
    /* send over current socket as HTTP OK response: */  
    /*测试过，第二参数必须http，action随意*/  
    soap_send___wsdd__ProbeMatches(soap, "http://", NULL, &ProbeMatches);  
    return SOAP_OK;  

}

int  __wsdd__ProbeMatches(struct soap* soap, struct wsdd__ProbeMatchesType *wsdd__ProbeMatches)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __wsdd__Resolve(struct soap* soap, struct wsdd__ResolveType *wsdd__Resolve)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __wsdd__ResolveMatches(struct soap* soap, struct wsdd__ResolveMatchesType *wsdd__ResolveMatches)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __tdn__Hello(struct soap* soap, struct wsdd__HelloType tdn__Hello, struct wsdd__ResolveType *tdn__HelloResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __tdn__Bye(struct soap* soap, struct wsdd__ByeType tdn__Bye, struct wsdd__ResolveType *tdn__ByeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __tdn__Probe(struct soap* soap, struct wsdd__ProbeType tdn__Probe, struct wsdd__ProbeMatchesType *tdn__ProbeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __trv__GetServiceCapabilities(struct soap* soap, struct _trv__GetServiceCapabilities *trv__GetServiceCapabilities, struct _trv__GetServiceCapabilitiesResponse *trv__GetServiceCapabilitiesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __trv__GetReceivers(struct soap* soap, struct _trv__GetReceivers *trv__GetReceivers, struct _trv__GetReceiversResponse *trv__GetReceiversResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if(trv__GetReceiversResponse == NULL || g_pGetReceiversResponse==NULL)
    {
        return SOAP_ERR;
    }    

    trv__GetReceiversResponse->__sizeReceivers = g_pGetReceiversResponse->__sizeReceivers;
    trv__GetReceiversResponse->Receivers = g_pGetReceiversResponse->Receivers;
    
    return SOAP_OK;
}

int  __trv__GetReceiver(struct soap* soap, struct _trv__GetReceiver *trv__GetReceiver, struct _trv__GetReceiverResponse *trv__GetReceiverResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if(trv__GetReceiverResponse == NULL || g_pGetReceiversResponse==NULL)
    {
        return SOAP_ERR;
    }

    unsigned int i = 0;
    for( ; i<RECEIVER_COUNT; i++ )
    {
        if(g_pGetReceiversResponse->Receivers!=NULL)
        {
            if( strcmp(trv__GetReceiver->ReceiverToken, g_pGetReceiversResponse->Receivers[i].Token) == 0)
            {
                trv__GetReceiverResponse->Receiver = &g_pGetReceiversResponse->Receivers[i];
                return SOAP_OK;
            }
        }
    }

    return SOAP_ERR;
}

int  __trv__CreateReceiver(struct soap* soap, struct _trv__CreateReceiver *trv__CreateReceiver, struct _trv__CreateReceiverResponse *trv__CreateReceiverResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    
    return SOAP_OK;
}

int  __trv__DeleteReceiver(struct soap* soap, struct _trv__DeleteReceiver *trv__DeleteReceiver, struct _trv__DeleteReceiverResponse *trv__DeleteReceiverResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __trv__ConfigureReceiver(struct soap* soap, struct _trv__ConfigureReceiver *trv__ConfigureReceiver, struct _trv__ConfigureReceiverResponse *trv__ConfigureReceiverResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    char szMediaUri[1024] = {0};
    char szToken[200] = {0};
    int iChnno = 0;
    sprintf(szToken, "%s", trv__ConfigureReceiver->ReceiverToken);
    sprintf(szMediaUri, "%s", trv__ConfigureReceiver->Configuration->MediaUri);
    sscanf(szToken, "tokenReceiver%d", &iChnno);
    if(iChnno <= 0 || iChnno >RECEIVER_COUNT)
    {
        iChnno = 1;
        sprintf(szToken, "tokenReceiver%d", iChnno);
        printf("ConfigureReceiver: invalid receiver token [%s], and set to be [%s]", trv__ConfigureReceiver->ReceiverToken, szToken);
    }
    if(g_fSetMediaUri && g_pObjOnvif)
    {
        int iRet = g_fSetMediaUri(szMediaUri, iChnno, g_pObjOnvif);
        if(iRet !=0 )
        {
            printf("%s: set media uri failed!\n", __FILE__);
            return SOAP_ERR;
        }
    }
    printf("set receiver[%s] media uri [%s]\n", szToken, szMediaUri);

    // 保存
    if(g_pGetReceiversResponse != NULL)
    {
        unsigned int i = 0;
        for( ; i<RECEIVER_COUNT; i++ )
        {
            if(g_pGetReceiversResponse->Receivers!=NULL && \
                strcmp(szToken, g_pGetReceiversResponse->Receivers[i].Token) == 0)
            {
                if(g_pGetReceiversResponse->Receivers[i].Configuration->MediaUri == NULL)
                {
                    g_pGetReceiversResponse->Receivers[i].Configuration->MediaUri = malloc(sizeof(char)*1024);
                }
                sprintf(g_pGetReceiversResponse->Receivers[i].Configuration->MediaUri, "%s", szMediaUri);
                break;
            }
        }
    }
    return SOAP_OK;
}

int  __trv__SetReceiverMode(struct soap* soap, struct _trv__SetReceiverMode *trv__SetReceiverMode, struct _trv__SetReceiverModeResponse *trv__SetReceiverModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int  __trv__GetReceiverState(struct soap* soap, struct _trv__GetReceiverState *trv__GetReceiverState, struct _trv__GetReceiverStateResponse *trv__GetReceiverStateResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetServices(struct soap* soap, struct _tds__GetServices *tds__GetServices, struct _tds__GetServicesResponse *tds__GetServicesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetServiceCapabilities(struct soap* soap, struct _tds__GetServiceCapabilities *tds__GetServiceCapabilities, struct _tds__GetServiceCapabilitiesResponse *tds__GetServiceCapabilitiesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDeviceInformation(struct soap* soap, struct _tds__GetDeviceInformation *tds__GetDeviceInformation, struct _tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if(g_pGetDeviceInformationResponse == NULL)
    {
        g_pGetDeviceInformationResponse = malloc(sizeof(struct _tds__GetDeviceInformationResponse));
        
        g_pGetDeviceInformationResponse->Manufacturer = malloc(sizeof(char)*80);
        g_pGetDeviceInformationResponse->Model = malloc(sizeof(char)*80);
        g_pGetDeviceInformationResponse->FirmwareVersion = malloc(sizeof(char)*80);
        g_pGetDeviceInformationResponse->SerialNumber = malloc(sizeof(char)*80);
        g_pGetDeviceInformationResponse->HardwareId = malloc(sizeof(char)*80);
    }
    struct _tds__GetDeviceInformationResponse* ptr = g_pGetDeviceInformationResponse;
    memset(ptr->Manufacturer, 0, sizeof(char)*80);
    memset(ptr->Model, 0, sizeof(char)*80);
    memset(ptr->FirmwareVersion, 0, sizeof(char)*80);
    memset(ptr->SerialNumber, 0, sizeof(char)*80);
    memset(ptr->HardwareId, 0, sizeof(char)*80);
    
    
    if(g_fGetBuildtime == NULL || g_fGetModel == NULL || g_fGetSerial == NULL || g_pObjOnvif == NULL)
    {
        return SOAP_ERR;
    }
    if(
        g_fGetBuildtime((void*)ptr->FirmwareVersion, (void*)g_pObjOnvif) == 0 && 
        g_fGetModel((void*)ptr->Model, (void*)g_pObjOnvif) == 0 && 
        g_fGetSerial((void*)ptr->SerialNumber, (void*)g_pObjOnvif) == 0
    )
    {
        unsigned char mac[6]={0};
        get_mac(mac);
        sprintf(ptr->HardwareId, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        sprintf(ptr->Manufacturer, "%s", "");
    }
    else
    {
        return SOAP_ERR;
    }
    memcpy(tds__GetDeviceInformationResponse, ptr, sizeof(struct _tds__GetDeviceInformationResponse));
    return SOAP_OK;
}

int __tds__SetSystemDateAndTime(struct soap* soap, struct _tds__SetSystemDateAndTime *tds__SetSystemDateAndTime, struct _tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);

    // 手动设置时间
    if(tds__SetSystemDateAndTime->DateTimeType == tt__SetDateTimeType__Manual)
    {
        struct tm mytm;
        memset(&mytm, 0, sizeof(struct tm));
        mytm.tm_year = tds__SetSystemDateAndTime->UTCDateTime->Date->Year;
        mytm.tm_mon = tds__SetSystemDateAndTime->UTCDateTime->Date->Month;
        mytm.tm_mday = tds__SetSystemDateAndTime->UTCDateTime->Date->Day;
        mytm.tm_hour = tds__SetSystemDateAndTime->UTCDateTime->Time->Hour;
        mytm.tm_min = tds__SetSystemDateAndTime->UTCDateTime->Time->Minute;
        mytm.tm_sec = tds__SetSystemDateAndTime->UTCDateTime->Time->Second;

        char time[80] = {0};
        sprintf(time, "%d-%d-%d %d:%d:%d", mytm.tm_year, mytm.tm_mon, mytm.tm_mday, mytm.tm_hour, mytm.tm_min, mytm.tm_sec);
        if(g_fSetSystime == NULL || g_pObjOnvif == NULL)
        {
            return SOAP_ERR;
        }
        if(g_fSetSystime((void*)time, (void*)g_pObjOnvif) != 0)
        {
            return SOAP_ERR;
        }
        return SOAP_OK;
    }
    else if(tds__SetSystemDateAndTime->DateTimeType == tt__SetDateTimeType__NTP)
    {
        
    }
    else
    {
        return SOAP_ERR;
    }
    return SOAP_OK;
}

int __tds__GetSystemDateAndTime(struct soap* soap, struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if(g_pGetSystemDateAndTimeResponse == NULL)
    {
        g_pGetSystemDateAndTimeResponse = malloc(sizeof(struct _tds__GetSystemDateAndTimeResponse));
        memset(g_pGetSystemDateAndTimeResponse, 0, sizeof(struct _tds__GetSystemDateAndTimeResponse));
        
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime = malloc(sizeof(struct tt__SystemDateTime));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime, 0, sizeof(struct tt__SystemDateTime));
        /*
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone = malloc(sizeof(struct tt__TimeZone));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone, 0, sizeof(struct tt__TimeZone));
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ = malloc(sizeof(char)*80);
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ , 0, sizeof(char)*80);
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime= malloc(sizeof(struct tt__DateTime));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime, 0, sizeof(struct tt__DateTime));
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time = malloc(sizeof(struct tt__Time));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time, 0, sizeof(struct tt__Time));
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date = malloc(sizeof(struct tt__Date));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date, 0, sizeof(struct tt__Date));
        */
        
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime = malloc(sizeof(struct tt__DateTime));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime, 0, sizeof(struct tt__DateTime));
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time = malloc(sizeof(struct tt__Time));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time, 0, sizeof(struct tt__Time));
        g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date = malloc(sizeof(struct tt__Date));
        memset(g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date, 0, sizeof(struct tt__Date));
    }    
    
    struct tt__SystemDateTime *ptr = g_pGetSystemDateAndTimeResponse->SystemDateAndTime;
    ptr->DateTimeType = tt__SetDateTimeType__Manual;
    ptr->DaylightSavings = xsd__boolean__false_;
    
    if(g_fGetSystime == NULL || g_pObjOnvif == NULL)
    {
        return SOAP_ERR;
    }
    char time[80] = {0};
    int iRet = g_fGetSystime((void*)time, g_pObjOnvif);
    if(iRet != 0)
    {
        return SOAP_ERR;
    }
    struct tm mytm;
    memset(&mytm, 0, sizeof(struct tm));
    sscanf(time, "%d-%d-%d %d:%d:%d", &mytm.tm_year,&mytm.tm_mon,&mytm.tm_mday, \
        &mytm.tm_hour, &mytm.tm_min, &mytm.tm_sec);
    struct tt__Time* ptime = NULL;
    struct tt__Date* pdate = NULL;
    /*
        sprintf(ptr->TimeZone->TZ, "%s", "GTM +8");
        ptime = g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time;
        pdate = g_pGetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date;
        ptime->Hour = mytm.tm_hour;
        ptime->Minute = mytm.tm_min;
        ptime->Second = mytm.tm_sec;
        pdate->Year = mytm.tm_year;
        pdate->Month = mytm.tm_mon;
        pdate->Day = mytm.tm_mday;
    */

    ptime = g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time;
    pdate = g_pGetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date;
    ptime->Hour = mytm.tm_hour;
    ptime->Minute = mytm.tm_min;
    ptime->Second = mytm.tm_sec;
    pdate->Year = mytm.tm_year;
    pdate->Month = mytm.tm_mon;
    pdate->Day = mytm.tm_mday;

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime = g_pGetSystemDateAndTimeResponse->SystemDateAndTime;
    return SOAP_OK;
}

int __tds__SetSystemFactoryDefault(struct soap* soap, struct _tds__SetSystemFactoryDefault *tds__SetSystemFactoryDefault, struct _tds__SetSystemFactoryDefaultResponse *tds__SetSystemFactoryDefaultResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__UpgradeSystemFirmware(struct soap* soap, struct _tds__UpgradeSystemFirmware *tds__UpgradeSystemFirmware, struct _tds__UpgradeSystemFirmwareResponse *tds__UpgradeSystemFirmwareResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

void *reboot_thread_worker(void* argv)
{
    sleep(2);
    system("/sbin/reboot");
    return NULL;
}
int __tds__SystemReboot(struct soap* soap, struct _tds__SystemReboot *tds__SystemReboot, struct _tds__SystemRebootResponse *tds__SystemRebootResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    pthread_t  waitThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&waitThread, &attr, reboot_thread_worker, NULL);
    tds__SystemRebootResponse->Message = "reboot system! bye";
    return SOAP_OK;
}

int __tds__RestoreSystem(struct soap* soap, struct _tds__RestoreSystem *tds__RestoreSystem, struct _tds__RestoreSystemResponse *tds__RestoreSystemResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetSystemBackup(struct soap* soap, struct _tds__GetSystemBackup *tds__GetSystemBackup, struct _tds__GetSystemBackupResponse *tds__GetSystemBackupResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetSystemLog(struct soap* soap, struct _tds__GetSystemLog *tds__GetSystemLog, struct _tds__GetSystemLogResponse *tds__GetSystemLogResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetSystemSupportInformation(struct soap* soap, struct _tds__GetSystemSupportInformation *tds__GetSystemSupportInformation, struct _tds__GetSystemSupportInformationResponse *tds__GetSystemSupportInformationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetScopes(struct soap* soap, struct _tds__GetScopes *tds__GetScopes, struct _tds__GetScopesResponse *tds__GetScopesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetScopes(struct soap* soap, struct _tds__SetScopes *tds__SetScopes, struct _tds__SetScopesResponse *tds__SetScopesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__AddScopes(struct soap* soap, struct _tds__AddScopes *tds__AddScopes, struct _tds__AddScopesResponse *tds__AddScopesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__RemoveScopes(struct soap* soap, struct _tds__RemoveScopes *tds__RemoveScopes, struct _tds__RemoveScopesResponse *tds__RemoveScopesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDiscoveryMode(struct soap* soap, struct _tds__GetDiscoveryMode *tds__GetDiscoveryMode, struct _tds__GetDiscoveryModeResponse *tds__GetDiscoveryModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetDiscoveryMode(struct soap* soap, struct _tds__SetDiscoveryMode *tds__SetDiscoveryMode, struct _tds__SetDiscoveryModeResponse *tds__SetDiscoveryModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetRemoteDiscoveryMode(struct soap* soap, struct _tds__GetRemoteDiscoveryMode *tds__GetRemoteDiscoveryMode, struct _tds__GetRemoteDiscoveryModeResponse *tds__GetRemoteDiscoveryModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetRemoteDiscoveryMode(struct soap* soap, struct _tds__SetRemoteDiscoveryMode *tds__SetRemoteDiscoveryMode, struct _tds__SetRemoteDiscoveryModeResponse *tds__SetRemoteDiscoveryModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDPAddresses(struct soap* soap, struct _tds__GetDPAddresses *tds__GetDPAddresses, struct _tds__GetDPAddressesResponse *tds__GetDPAddressesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetEndpointReference(struct soap* soap, struct _tds__GetEndpointReference *tds__GetEndpointReference, struct _tds__GetEndpointReferenceResponse *tds__GetEndpointReferenceResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetRemoteUser(struct soap* soap, struct _tds__GetRemoteUser *tds__GetRemoteUser, struct _tds__GetRemoteUserResponse *tds__GetRemoteUserResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetRemoteUser(struct soap* soap, struct _tds__SetRemoteUser *tds__SetRemoteUser, struct _tds__SetRemoteUserResponse *tds__SetRemoteUserResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetUsers(struct soap* soap, struct _tds__GetUsers *tds__GetUsers, struct _tds__GetUsersResponse *tds__GetUsersResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__CreateUsers(struct soap* soap, struct _tds__CreateUsers *tds__CreateUsers, struct _tds__CreateUsersResponse *tds__CreateUsersResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__DeleteUsers(struct soap* soap, struct _tds__DeleteUsers *tds__DeleteUsers, struct _tds__DeleteUsersResponse *tds__DeleteUsersResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetUser(struct soap* soap, struct _tds__SetUser *tds__SetUser, struct _tds__SetUserResponse *tds__SetUserResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetWsdlUrl(struct soap* soap, struct _tds__GetWsdlUrl *tds__GetWsdlUrl, struct _tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetCapabilities(struct soap* soap, struct _tds__GetCapabilities *tds__GetCapabilities, struct _tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetDPAddresses(struct soap* soap, struct _tds__SetDPAddresses *tds__SetDPAddresses, struct _tds__SetDPAddressesResponse *tds__SetDPAddressesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetHostname(struct soap* soap, struct _tds__GetHostname *tds__GetHostname, struct _tds__GetHostnameResponse *tds__GetHostnameResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if( g_pGetHostnameResponse == NULL)
    {
        g_pGetHostnameResponse = malloc(sizeof(struct _tds__GetHostnameResponse));
        g_pGetHostnameResponse->HostnameInformation = malloc(sizeof(struct tt__HostnameInformation));
        memset(g_pGetHostnameResponse->HostnameInformation, 0, sizeof(struct tt__HostnameInformation));
        g_pGetHostnameResponse->HostnameInformation->Name = malloc(sizeof(char)*80);
        memset(g_pGetHostnameResponse->HostnameInformation->Name, 0, sizeof(char)*80);
    }
    g_pGetHostnameResponse->HostnameInformation->FromDHCP = xsd__boolean__false_;
    if(g_fGetHostname && g_pObjOnvif)
    {
        if(g_fGetHostname((void*)g_pGetHostnameResponse->HostnameInformation->Name, g_pObjOnvif) != 0)
        {
            return SOAP_ERR;
        }
    }
    tds__GetHostnameResponse->HostnameInformation = g_pGetHostnameResponse->HostnameInformation;
    return SOAP_OK;
}

int __tds__SetHostname(struct soap* soap, struct _tds__SetHostname *tds__SetHostname, struct _tds__SetHostnameResponse *tds__SetHostnameResponse)
{
    printf("%s : %s\n", __FILE__, __func__);

    if(g_fSetHostname && g_pObjOnvif)
    {
        if (g_fSetHostname((void*)tds__SetHostname->Name, g_pObjOnvif) != 0)
        {
            return SOAP_ERR;
        }
    }

    return SOAP_OK;
}

int __tds__SetHostnameFromDHCP(struct soap* soap, struct _tds__SetHostnameFromDHCP *tds__SetHostnameFromDHCP, struct _tds__SetHostnameFromDHCPResponse *tds__SetHostnameFromDHCPResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDNS(struct soap* soap, struct _tds__GetDNS *tds__GetDNS, struct _tds__GetDNSResponse *tds__GetDNSResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetDNS(struct soap* soap, struct _tds__SetDNS *tds__SetDNS, struct _tds__SetDNSResponse *tds__SetDNSResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetNTP(struct soap* soap, struct _tds__GetNTP *tds__GetNTP, struct _tds__GetNTPResponse *tds__GetNTPResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    struct MyNtpInfo
    {
        char ip[32];//NTP服务器IP地址
        int iEnable;//是否生效，0-否，1是
        int iInterval;//校时间隔，分钟
    }ntp;

    if(g_pGetNTPResponse == NULL)
    {
        g_pGetNTPResponse = malloc(sizeof(struct _tds__GetNTPResponse));
        g_pGetNTPResponse->NTPInformation = malloc(sizeof(struct tt__NTPInformation));
        memset(g_pGetNTPResponse->NTPInformation, 0, sizeof(struct tt__NTPInformation));
        g_pGetNTPResponse->NTPInformation->FromDHCP = xsd__boolean__false_;
        
        g_pGetNTPResponse->NTPInformation->__sizeNTPManual = 1;
        g_pGetNTPResponse->NTPInformation->NTPManual = malloc(sizeof(struct tt__NetworkHost));
        memset(g_pGetNTPResponse->NTPInformation->NTPManual, 0, sizeof(struct tt__NetworkHost));
        g_pGetNTPResponse->NTPInformation->NTPManual->Type = tt__NetworkHostType__IPv4;
        g_pGetNTPResponse->NTPInformation->NTPManual->IPv4Address = malloc(sizeof(char)*32);
        memset(g_pGetNTPResponse->NTPInformation->NTPManual->IPv4Address, 0,  sizeof(char)*32);
    }
    int iRet = g_fGetNtp((void*)&ntp, g_pObjOnvif);
    if(iRet !=0 )
    {
        return SOAP_ERR;
    }
    if(ntp.iEnable ==0)
    {
        strcpy(g_pGetNTPResponse->NTPInformation->NTPManual->IPv4Address, "0.0.0.0");
    }
    else
    {
        strcpy(g_pGetNTPResponse->NTPInformation->NTPManual->IPv4Address, ntp.ip);
    }
    tds__GetNTPResponse->NTPInformation = g_pGetNTPResponse->NTPInformation;
    return SOAP_OK;
}

int __tds__SetNTP(struct soap* soap, struct _tds__SetNTP *tds__SetNTP, struct _tds__SetNTPResponse *tds__SetNTPResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    int iEnable = 0;
    char szIp[32] = {0};
    int iInterval = 5;
    struct _tds__SetNTP
    {
        enum xsd__boolean FromDHCP; /* required element of type xsd:boolean */
        int __sizeNTPManual;    /* sequence of elements <NTPManual> */
        struct tt__NetworkHost *NTPManual;  /* optional element of type tt:NetworkHost */
    };
    if(tds__SetNTP->FromDHCP == xsd__boolean__true_)
    {
        iEnable = 0;
        szIp[0] = ' ';
    }
    else if(tds__SetNTP->__sizeNTPManual >0 )
    {
        if(tds__SetNTP->NTPManual != NULL)
        {
            strcpy(szIp, tds__SetNTP->NTPManual[0].IPv4Address);
            if(IsIP(szIp)==-1 || strcmp(szIp, "0.0.0.0")==0)
            {
                iEnable = 0;
                sprintf(szIp, "%s", " ");
            }
            else
            {
                iEnable = 1;
            }
        }
    }
    else
    {
        return SOAP_ERR;
    }
    char ntpStr[200]={0};
    sprintf(ntpStr, "%d;%s;%d;", iEnable, szIp, iInterval);
    int iRet = g_fSetNtp((void*)ntpStr, g_pObjOnvif);
    if(iRet != 0)
    {
        return SOAP_ERR;
    }
    return SOAP_OK;
}

int __tds__GetDynamicDNS(struct soap* soap, struct _tds__GetDynamicDNS *tds__GetDynamicDNS, struct _tds__GetDynamicDNSResponse *tds__GetDynamicDNSResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    
    return SOAP_OK;
}

int __tds__SetDynamicDNS(struct soap* soap, struct _tds__SetDynamicDNS *tds__SetDynamicDNS, struct _tds__SetDynamicDNSResponse *tds__SetDynamicDNSResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetNetworkInterfaces(struct soap* soap, struct _tds__GetNetworkInterfaces *tds__GetNetworkInterfaces, struct _tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    int iRet = GetNetworkInterfacesResponseInfo();
    if(iRet != 0) 
        return SOAP_ERR;
    GetNetworkInterfacesResponseInfo();
    tds__GetNetworkInterfacesResponse->__sizeNetworkInterfaces = g_pGetNetworkInterfacesResponse->__sizeNetworkInterfaces;
    tds__GetNetworkInterfacesResponse->NetworkInterfaces = g_pGetNetworkInterfacesResponse->NetworkInterfaces;
    return SOAP_OK;
}

int __tds__SetNetworkInterfaces(struct soap* soap, struct _tds__SetNetworkInterfaces *tds__SetNetworkInterfaces, struct _tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    char ip[32]={0};//IP地址
    char netmask[32]={0};//掩码
    
    struct _tds__SetNetworkInterfaces * pData = tds__SetNetworkInterfaces;
    //if(strcmp(pData->InterfaceToken, "eth0") != 0)
        //return SOAP_ERR;
    sprintf(ip, "%s", pData->NetworkInterface->IPv4->Manual->Address);
    sprintf(netmask, "%s", prefix2netmask(pData->NetworkInterface->IPv4->Manual->PrefixLength));
    if(g_fSetNetIp && g_pObjOnvif)
    {
        if( g_fSetNetIp(ip, netmask, (void*)g_pObjOnvif) != 0)
            return SOAP_ERR;
    }
    return SOAP_OK;
}

int __tds__GetNetworkProtocols(struct soap* soap, struct _tds__GetNetworkProtocols *tds__GetNetworkProtocols, struct _tds__GetNetworkProtocolsResponse *tds__GetNetworkProtocolsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetNetworkProtocols(struct soap* soap, struct _tds__SetNetworkProtocols *tds__SetNetworkProtocols, struct _tds__SetNetworkProtocolsResponse *tds__SetNetworkProtocolsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetNetworkDefaultGateway(struct soap* soap, struct _tds__GetNetworkDefaultGateway *tds__GetNetworkDefaultGateway, struct _tds__GetNetworkDefaultGatewayResponse *tds__GetNetworkDefaultGatewayResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    if(g_pGetNetworkDefaultGatewayResponse == NULL)
    {
        g_pGetNetworkDefaultGatewayResponse = malloc(sizeof(struct _tds__GetNetworkDefaultGatewayResponse));
        struct _tds__GetNetworkDefaultGatewayResponse *ptr = g_pGetNetworkDefaultGatewayResponse;
        ptr->NetworkGateway = malloc(sizeof(struct tt__NetworkGateway));
        memset(ptr->NetworkGateway, 0, sizeof(struct tt__NetworkGateway));
        ptr->NetworkGateway->__sizeIPv4Address = 1;
        ptr->NetworkGateway->IPv4Address = (char**)malloc(sizeof(char*)*1);
        ptr->NetworkGateway->IPv4Address[0] = (char*)malloc(sizeof(char)*32);
    }
    struct _tds__GetNetworkDefaultGatewayResponse *ptr = g_pGetNetworkDefaultGatewayResponse;
    if(g_fGetNetGW && g_pObjOnvif)
    {
        if( g_fGetNetGW((void*)ptr->NetworkGateway->IPv4Address[0], g_pObjOnvif) != 0)
        {
            return SOAP_ERR;
        }
    }
    tds__GetNetworkDefaultGatewayResponse->NetworkGateway = ptr->NetworkGateway;
    return SOAP_OK;
}

int __tds__SetNetworkDefaultGateway(struct soap* soap, struct _tds__SetNetworkDefaultGateway *tds__SetNetworkDefaultGateway, struct _tds__SetNetworkDefaultGatewayResponse *tds__SetNetworkDefaultGatewayResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    char* gw = NULL;
    if(tds__SetNetworkDefaultGateway->__sizeIPv4Address>0)
    {
        gw = tds__SetNetworkDefaultGateway->IPv4Address[0];
        if( g_fSetNetGW && g_pObjOnvif)
        {
            if( g_fSetNetGW((void*)gw, (void*)g_pObjOnvif) != 0 )
            {
                return SOAP_ERR;
            }
        }
    }
    else
    {
        return SOAP_ERR;
    }
    return SOAP_OK;
}

int __tds__GetZeroConfiguration(struct soap* soap, struct _tds__GetZeroConfiguration *tds__GetZeroConfiguration, struct _tds__GetZeroConfigurationResponse *tds__GetZeroConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetZeroConfiguration(struct soap* soap, struct _tds__SetZeroConfiguration *tds__SetZeroConfiguration, struct _tds__SetZeroConfigurationResponse *tds__SetZeroConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetIPAddressFilter(struct soap* soap, struct _tds__GetIPAddressFilter *tds__GetIPAddressFilter, struct _tds__GetIPAddressFilterResponse *tds__GetIPAddressFilterResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetIPAddressFilter(struct soap* soap, struct _tds__SetIPAddressFilter *tds__SetIPAddressFilter, struct _tds__SetIPAddressFilterResponse *tds__SetIPAddressFilterResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__AddIPAddressFilter(struct soap* soap, struct _tds__AddIPAddressFilter *tds__AddIPAddressFilter, struct _tds__AddIPAddressFilterResponse *tds__AddIPAddressFilterResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__RemoveIPAddressFilter(struct soap* soap, struct _tds__RemoveIPAddressFilter *tds__RemoveIPAddressFilter, struct _tds__RemoveIPAddressFilterResponse *tds__RemoveIPAddressFilterResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetAccessPolicy(struct soap* soap, struct _tds__GetAccessPolicy *tds__GetAccessPolicy, struct _tds__GetAccessPolicyResponse *tds__GetAccessPolicyResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}
int __tds__SetAccessPolicy(struct soap* soap, struct _tds__SetAccessPolicy *tds__SetAccessPolicy, struct _tds__SetAccessPolicyResponse *tds__SetAccessPolicyResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__CreateCertificate(struct soap* soap, struct _tds__CreateCertificate *tds__CreateCertificate, struct _tds__CreateCertificateResponse *tds__CreateCertificateResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetCertificates(struct soap* soap, struct _tds__GetCertificates *tds__GetCertificates, struct _tds__GetCertificatesResponse *tds__GetCertificatesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetCertificatesStatus(struct soap* soap, struct _tds__GetCertificatesStatus *tds__GetCertificatesStatus, struct _tds__GetCertificatesStatusResponse *tds__GetCertificatesStatusResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetCertificatesStatus(struct soap* soap, struct _tds__SetCertificatesStatus *tds__SetCertificatesStatus, struct _tds__SetCertificatesStatusResponse *tds__SetCertificatesStatusResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__DeleteCertificates(struct soap* soap, struct _tds__DeleteCertificates *tds__DeleteCertificates, struct _tds__DeleteCertificatesResponse *tds__DeleteCertificatesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetPkcs10Request(struct soap* soap, struct _tds__GetPkcs10Request *tds__GetPkcs10Request, struct _tds__GetPkcs10RequestResponse *tds__GetPkcs10RequestResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__LoadCertificates(struct soap* soap, struct _tds__LoadCertificates *tds__LoadCertificates, struct _tds__LoadCertificatesResponse *tds__LoadCertificatesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetClientCertificateMode(struct soap* soap, struct _tds__GetClientCertificateMode *tds__GetClientCertificateMode, struct _tds__GetClientCertificateModeResponse *tds__GetClientCertificateModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetClientCertificateMode(struct soap* soap, struct _tds__SetClientCertificateMode *tds__SetClientCertificateMode, struct _tds__SetClientCertificateModeResponse *tds__SetClientCertificateModeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetRelayOutputs(struct soap* soap, struct _tds__GetRelayOutputs *tds__GetRelayOutputs, struct _tds__GetRelayOutputsResponse *tds__GetRelayOutputsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetRelayOutputSettings(struct soap* soap, struct _tds__SetRelayOutputSettings *tds__SetRelayOutputSettings, struct _tds__SetRelayOutputSettingsResponse *tds__SetRelayOutputSettingsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetRelayOutputState(struct soap* soap, struct _tds__SetRelayOutputState *tds__SetRelayOutputState, struct _tds__SetRelayOutputStateResponse *tds__SetRelayOutputStateResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SendAuxiliaryCommand(struct soap* soap, struct _tds__SendAuxiliaryCommand *tds__SendAuxiliaryCommand, struct _tds__SendAuxiliaryCommandResponse *tds__SendAuxiliaryCommandResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetCACertificates(struct soap* soap, struct _tds__GetCACertificates *tds__GetCACertificates, struct _tds__GetCACertificatesResponse *tds__GetCACertificatesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__LoadCertificateWithPrivateKey(struct soap* soap, struct _tds__LoadCertificateWithPrivateKey *tds__LoadCertificateWithPrivateKey, struct _tds__LoadCertificateWithPrivateKeyResponse *tds__LoadCertificateWithPrivateKeyResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetCertificateInformation(struct soap* soap, struct _tds__GetCertificateInformation *tds__GetCertificateInformation, struct _tds__GetCertificateInformationResponse *tds__GetCertificateInformationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__LoadCACertificates(struct soap* soap, struct _tds__LoadCACertificates *tds__LoadCACertificates, struct _tds__LoadCACertificatesResponse *tds__LoadCACertificatesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__CreateDot1XConfiguration(struct soap* soap, struct _tds__CreateDot1XConfiguration *tds__CreateDot1XConfiguration, struct _tds__CreateDot1XConfigurationResponse *tds__CreateDot1XConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetDot1XConfiguration(struct soap* soap, struct _tds__SetDot1XConfiguration *tds__SetDot1XConfiguration, struct _tds__SetDot1XConfigurationResponse *tds__SetDot1XConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDot1XConfiguration(struct soap* soap, struct _tds__GetDot1XConfiguration *tds__GetDot1XConfiguration, struct _tds__GetDot1XConfigurationResponse *tds__GetDot1XConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDot1XConfigurations(struct soap* soap, struct _tds__GetDot1XConfigurations *tds__GetDot1XConfigurations, struct _tds__GetDot1XConfigurationsResponse *tds__GetDot1XConfigurationsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__DeleteDot1XConfiguration(struct soap* soap, struct _tds__DeleteDot1XConfiguration *tds__DeleteDot1XConfiguration, struct _tds__DeleteDot1XConfigurationResponse *tds__DeleteDot1XConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDot11Capabilities(struct soap* soap, struct _tds__GetDot11Capabilities *tds__GetDot11Capabilities, struct _tds__GetDot11CapabilitiesResponse *tds__GetDot11CapabilitiesResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetDot11Status(struct soap* soap, struct _tds__GetDot11Status *tds__GetDot11Status, struct _tds__GetDot11StatusResponse *tds__GetDot11StatusResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__ScanAvailableDot11Networks(struct soap* soap, struct _tds__ScanAvailableDot11Networks *tds__ScanAvailableDot11Networks, struct _tds__ScanAvailableDot11NetworksResponse *tds__ScanAvailableDot11NetworksResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetSystemUris(struct soap* soap, struct _tds__GetSystemUris *tds__GetSystemUris, struct _tds__GetSystemUrisResponse *tds__GetSystemUrisResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__StartFirmwareUpgrade(struct soap* soap, struct _tds__StartFirmwareUpgrade *tds__StartFirmwareUpgrade, struct _tds__StartFirmwareUpgradeResponse *tds__StartFirmwareUpgradeResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__StartSystemRestore(struct soap* soap, struct _tds__StartSystemRestore *tds__StartSystemRestore, struct _tds__StartSystemRestoreResponse *tds__StartSystemRestoreResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetStorageConfigurations(struct soap* soap, struct _tds__GetStorageConfigurations *tds__GetStorageConfigurations, struct _tds__GetStorageConfigurationsResponse *tds__GetStorageConfigurationsResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__CreateStorageConfiguration(struct soap* soap, struct _tds__CreateStorageConfiguration *tds__CreateStorageConfiguration, struct _tds__CreateStorageConfigurationResponse *tds__CreateStorageConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__GetStorageConfiguration(struct soap* soap, struct _tds__GetStorageConfiguration *tds__GetStorageConfiguration, struct _tds__GetStorageConfigurationResponse *tds__GetStorageConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__SetStorageConfiguration(struct soap* soap, struct _tds__SetStorageConfiguration *tds__SetStorageConfiguration, struct _tds__SetStorageConfigurationResponse *tds__SetStorageConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

int __tds__DeleteStorageConfiguration(struct soap* soap, struct _tds__DeleteStorageConfiguration *tds__DeleteStorageConfiguration, struct _tds__DeleteStorageConfigurationResponse *tds__DeleteStorageConfigurationResponse)
{
    printf("%s : %s\n", __FILE__, __func__);
    return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// 自定义
////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetCB_SetMediaUri(SetMediaUriCB_T cbFun, void* pCBobj)
{
    g_fSetMediaUri = cbFun;
    g_pObjOnvif = pCBobj;
}
void SetCB_SetNetIp(SetNetIpCB_T cbFun, void* pCBobj)
{
    g_fSetNetIp = cbFun;
    g_pObjOnvif = pCBobj;
}
void SetCB_GetNetGW(GetCB_T cbFun, void* pCBobj)
{
    g_fGetNetGW = cbFun;
    g_pObjOnvif = pCBobj;
}

void SetCB_SetNetGW(SetCB_T cbFun, void* pCBobj)
{
    g_fSetNetGW = cbFun;
    g_pObjOnvif = pCBobj;
}

void SetCB_GetHostname(GetCB_T cbFun, void* pCBobj)
{
    g_fGetHostname = cbFun;
    g_pObjOnvif = pCBobj;
}
void SetCB_SetHostname(SetCB_T cbFun, void* pCBobj)
{
    g_fSetHostname = cbFun;
    g_pObjOnvif = pCBobj;
}
void SetCB_GetSystime(GetCB_T cbFun, void* pCBobj)
{
    g_fGetSystime = cbFun;
    g_pObjOnvif = pCBobj;

}

void SetCB_SetSystime(SetCB_T cbFun, void* pCBobj)
{
    g_fSetSystime = cbFun;
    g_pObjOnvif = pCBobj;

}

void SetCB_GetNtp(GetCB_T cbFun, void* pCBobj)
{
    g_fGetNtp = cbFun;
    g_pObjOnvif = pCBobj;

}
void SetCB_SetNtp(SetCB_T cbFun, void* pCBobj)
{
    g_fSetNtp = cbFun;
    g_pObjOnvif = pCBobj;
}

void SetCB_GetModel(GetCB_T cbFun, void* pCBobj)
{
    g_fGetModel = cbFun;
    g_pObjOnvif = pCBobj;

}
void SetCB_GetSerial(SetCB_T cbFun, void* pCBobj)
{
    g_fGetSerial = cbFun;
    g_pObjOnvif = pCBobj;

}

void SetCB_GetBuildtime(SetCB_T cbFun, void* pCBobj)
{
    g_fGetBuildtime = cbFun;
    g_pObjOnvif = pCBobj;

}

int InitReceiver()
{
    g_fSetMediaUri = NULL;
    g_pObjOnvif = NULL;
    unsigned int i = 0;

    if(g_pGetReceiversResponse == NULL)
    {
        g_pGetReceiversResponse = malloc(sizeof(struct _trv__GetReceiversResponse));
        memset(g_pGetReceiversResponse, 0, sizeof(struct _trv__GetReceiversResponse));
        g_pGetReceiversResponse->__sizeReceivers = RECEIVER_COUNT;
        g_pGetReceiversResponse->Receivers = malloc(sizeof(struct tt__Receiver) * RECEIVER_COUNT);
        memset(g_pGetReceiversResponse->Receivers, 0, sizeof(struct tt__Receiver) * RECEIVER_COUNT);
        
        for( i=0; i<RECEIVER_COUNT; i++)
        {
            g_pGetReceiversResponse->Receivers[i].Token = malloc(sizeof(char)*32);
            memset(g_pGetReceiversResponse->Receivers[i].Token, 0, 32*sizeof(char));
            sprintf(g_pGetReceiversResponse->Receivers[i].Token, "tokenReceiver%d", i+1);
            g_pGetReceiversResponse->Receivers[i].Configuration = malloc(sizeof(struct tt__ReceiverConfiguration));
            memset(g_pGetReceiversResponse->Receivers[i].Configuration, 0, sizeof(struct tt__ReceiverConfiguration));
            g_pGetReceiversResponse->Receivers[i].Configuration->Mode = 0;
        }

    }
    return 0;
}

int GetNetworkInterfacesResponseInfo()
{
    int iRet = 0;
    struct ifreq *buf;
    buf = malloc(sizeof(struct ifreq)*MAXINTERFACES);
    int i = 0;
	for ( i = 0; i < MAXINTERFACES; i++ )
	{
		memset( &buf[i], 0, sizeof( struct ifreq ) );
	}
    
    //获取网卡数量
    int iInterfaceCount = GetLocalNetInfo( buf, NetInfoType_IP );
    // 分配内存空间并初始化
    struct tt__NetworkInterface* pNet = NULL;
    if(g_pGetNetworkInterfacesResponse == NULL)
    {
        g_pGetNetworkInterfacesResponse = malloc(sizeof(struct _tds__GetNetworkInterfacesResponse));
        
        g_pGetNetworkInterfacesResponse->__sizeNetworkInterfaces = iInterfaceCount;        
        g_pGetNetworkInterfacesResponse->NetworkInterfaces = malloc(sizeof(struct tt__NetworkInterface) * iInterfaceCount);
        pNet = g_pGetNetworkInterfacesResponse->NetworkInterfaces;
        memset(pNet, 0, sizeof(struct tt__NetworkInterface) * iInterfaceCount);
        for(i=0; i<iInterfaceCount; i++)
        {
            pNet[i].token = malloc(sizeof(char)*32);
            
            pNet[i].Info = malloc(sizeof(struct tt__NetworkInterfaceInfo));
            memset(pNet[i].Info, 0, sizeof(struct tt__NetworkInterfaceInfo));
            pNet[i].Info->Name = malloc(sizeof(char)*32);
            pNet[i].Info->HwAddress = malloc(sizeof(char)*32);
            pNet[i].Info->MTU = malloc(sizeof(int));

            pNet[i].Link = malloc(sizeof(struct tt__NetworkInterfaceLink));
            memset(pNet[i].Link, 0, sizeof(struct tt__NetworkInterfaceLink));

            pNet[i].Link->AdminSettings = malloc(sizeof(struct tt__NetworkInterfaceConnectionSetting));

            pNet[i].Link->OperSettings = malloc(sizeof(struct tt__NetworkInterfaceConnectionSetting));

            pNet[i].IPv4 = malloc(sizeof(struct tt__IPv4NetworkInterface));
            memset(pNet[i].IPv4, 0, sizeof(struct tt__IPv4NetworkInterface));

            pNet[i].IPv4->Config = malloc(sizeof(struct tt__IPv4Configuration));
            memset(pNet[i].IPv4->Config, 0, sizeof(struct tt__IPv4Configuration));
            pNet[i].IPv4->Config->Manual = malloc(sizeof(struct tt__PrefixedIPv4Address) * pNet[i].IPv4->Config->__sizeManual);
            memset(pNet[i].IPv4->Config->Manual, 0, sizeof(struct tt__PrefixedIPv4Address));
            pNet[i].IPv4->Config->Manual->Address = malloc(sizeof(char)*32);
        }

    }
    pNet=g_pGetNetworkInterfacesResponse->NetworkInterfaces;    
    
    for(i=0; i<iInterfaceCount; i++)
    {
        memset(pNet[i].token, 0, sizeof(char)*32);        
        
        memset(pNet[i].Info->Name, 0, sizeof(char)*32);
        memset(pNet[i].Info->HwAddress, 0, sizeof(char)*32);
        memset(pNet[i].Info->MTU, 0, sizeof(int));
    
        memset(pNet[i].Link->AdminSettings, 0, sizeof(struct tt__NetworkInterfaceConnectionSetting));
        memset(pNet[i].Link->OperSettings, 0, sizeof(struct tt__NetworkInterfaceConnectionSetting));

        pNet[i].IPv4->Enabled = xsd__boolean__true_;
        pNet[i].IPv4->Config->__sizeManual = 1;

        memset(pNet[i].IPv4->Config->Manual->Address, 0, sizeof(char)*32);
        pNet[i].IPv4->Config->DHCP = xsd__boolean__false_;
    }
    
    // token, enabled
    for(i=0; i<iInterfaceCount; i++)
    {
        sprintf(pNet[i].token, "%s", buf[i].ifr_name);
        pNet[i].Enabled = xsd__boolean__true_; // GetLocalNetInfo接口取到的信息均为已启动的网口信息
    }
    // MAC
    GetLocalNetInfo( buf, NetInfoType_MAC );
    for ( i = 0; i < iInterfaceCount; i++ )
    {
        sprintf(pNet[i].Info->HwAddress, "%02x:%02x:%02x:%02x:%02x:%02x", 
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[0],
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[1],
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[2],
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[3],
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[4],
                    (unsigned char) buf[i].ifr_hwaddr.sa_data[5]
                );
    }
    //MTU
    memset(buf, 0, sizeof(struct ifreq)*MAXINTERFACES);
    GetLocalNetInfo( buf, NetInfoType_MTU );
    for ( i = 0; i < iInterfaceCount; i++ )
    {
        memcpy(pNet[i].Info->MTU, &buf[i].ifr_mtu, sizeof(int));
    }

    // IP
    memset(buf, 0, sizeof(struct ifreq)*MAXINTERFACES);
    GetLocalNetInfo( buf, NetInfoType_IP );
    for ( i = 0; i < iInterfaceCount; i++ )
    {
        strcpy( pNet[i].IPv4->Config->Manual->Address, inet_ntoa( ( (struct sockaddr_in *)(&buf[i].ifr_addr) )->sin_addr ) );
    }

    // netmask
    memset(buf, 0, sizeof(struct ifreq)*MAXINTERFACES);
    GetLocalNetInfo( buf, NetInfoType_NM );
    char netmask[32] = {0};
    for ( i = 0; i < iInterfaceCount; i++ )
    {
        strcpy( netmask, inet_ntoa( ( (struct sockaddr_in *)(&buf[i].ifr_addr) )->sin_addr ) );
        pNet[i].IPv4->Config->Manual->PrefixLength = netmask2prefix(netmask);
    }
    // ethtool 信息
    memset(buf, 0, sizeof(struct ifreq)*MAXINTERFACES);
    GetLocalNetInfo( buf, NetInfoType_ETHTOOL);
    struct ethtool_cmd *ep;
    for ( i = 0; i < iInterfaceCount; i++ )
    {
        ep = (struct ethtool_cmd*)buf[i].ifr_data;
        pNet[i].Link->AdminSettings->Speed = ep->speed;
        pNet[i].Link->AdminSettings->AutoNegotiation = (ep->autoneg==0 ? xsd__boolean__false_ : xsd__boolean__true_);
        pNet[i].Link->AdminSettings->Duplex = (ep->autoneg>0 ? tt__Duplex__Full : tt__Duplex__Half);
        memcpy(pNet[i].Link->OperSettings, pNet[i].Link->AdminSettings, sizeof(struct tt__NetworkInterfaceConnectionSetting));
        if ( buf[i].ifr_data != NULL )  
            free(buf[i].ifr_data);
    }

    if ( buf != NULL )	
        free(buf);

    return iRet;
}







