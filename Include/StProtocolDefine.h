
#ifndef ST_PROTOCOL_DEFINE_H_
#define ST_PROTOCOL_DEFINE_H_

#pragma pack (push,1)

#define DECODER_TCP_PORT 9000
#define DECODER_UDP_PORT 9000
#define ENCODER_TCP_PORT 9000
#define ENCODER_UDP_PORT 9000
#define KEEPALIVE_INTERVAL 3
#define CMD_VERSION 0x20

typedef struct _StHead
{
    char version; // 版本号 二进制00100000 (0x20)
    int length; // 消息包长度
    int workid; // 消息流水号
    char flag; // 标志位，0x00-媒体数据 其他-控制命令
    int cmd; //消息指令编号(前三个字节为0x00)
    unsigned char src[4];//消息发送方编码，IP地址
    unsigned char dst[4];//消息接收方编码，IP地址
    char reserved[18];//预留，0
}struStHead;

/************************************
 *
 * 接口4 控制器<-->解码器
 *
 ************************************/
///////////////////////
// 指令编号
#define REQ_VC_LOGIN_DECODER 0x0401 //视频控制器登录解码器请求
#define RET_VC_LOGIN_DECODER 0x0402 //视频控制器登录解码器结果
#define REQ_VC_LOGOUT_DECODER 0x0403 //视频控制器登出解码器请求
#define RET_VC_LOGOUT_DECODER 0x0404 //视频控制器登出解码器结果
#define REQ_VC_SET_DECODER_PROFILE 0x0405 //解码器参数设置
#define RET_VC_SET_DECODER_PROFILE 0x0406 //解码器参数设置结果
#define REQ_VC_GET_DECODER_PROFILE 0x0407 //解码器参数查询
#define RET_VC_GET_DECODER_PROFILE 0x0408 //解码器参数查询结果
#define REQ_VC_GET_DECODER_STATE 0x0409 //解码器工作状态查询：未启动/接收单播/接收组播
#define RET_VC_GET_DECODER_STATE 0x040a //解码器工作状态查询结果
#define REQ_VC_SET_UNICAST_ENCODER 0x040b //设置解码器向编码器的单播连接请求
#define RET_VC_SET_UNICAST_ENCODER 0x040c //解码器向编码器的单播连接请求结果
#define REQ_VC_GET_UNICAST_ENCODER 0x040d //查询解码器连接的单播编码器
#define RET_VC_GET_UNICAST_ENCODER 0x040e //查询解码器连接的单播编码器结果
#define REQ_VC_LOCK_DECODER_VIDEO 0x040f //设置解码器图像锁定
#define RET_VC_LOCK_DECODER_VIDEO 0x0410 //设置解码器图像锁定结果
#define REQ_VC_UNLOCK_DECODER_VIDEO 0x0411 //设置解码器图像解锁
#define RET_VC_UNLOCK_DECODER_VIDEO 0x0412 //设置解码器图像解锁结果
#define REQ_VC_SET_MULTICAST_GROUP 0x0413 //设置解码器加入的组播组
#define RET_VC_SET_MULTICAST_GROUP 0x0414 //设置解码器加入的组播组的返回结果
#define REQ_VC_SET_RELAY 0x0415 //设置解码器转发的IP地址
#define RET_VC_SET_RELAY 0x0416 //设置解码器转发的IP地址的返回结果

/////////////////////////////////
//消息结构体
/////////////////////////////////

//控制器想解码器登录请求及返回
typedef struct _ReqVcLoginDec
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
    unsigned char byteLevel;// 0x00, 操作员优先级
    char szKey[64]; //登录密钥，全0x00
}ReqVcLoginDec;
typedef struct _RetVcLoginDec
{
    unsigned char byteRet;//登陆结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x01:无效设备 0x02:无效密钥 0x03:已登录
}RetVcLoginDec;

// 控制器向解码器退出登录请求
typedef struct _ReqVcLogoutDec
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcLogoutDec;
typedef struct _RetVcLogoutDec
{
    unsigned char byteRet;//登陆结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetVcLogoutDec;

//控制器设置解码器工作参数
typedef struct _ReqVcSetDecProfile
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecId[4];//解码器设备编号，全0x00
    unsigned char szDecIp[4];//解码器新的IP
    unsigned char szDecNetmask[4];//解码器子网掩码
    unsigned char szDecGateway[4];//解码器网关
    unsigned char szDecNetManagerIp[4];//解码器网管设备地址
    unsigned char szDecTcpPort[2];//解码器工作端口TCP
    unsigned char szDecUdpPort[2];//解码器工作端口UDP
    unsigned char szDecId2[4];// 解码器设备编号，全0x00
}ReqVcSetDecProfile;
typedef struct _RetVcSetDecProfile
{
    unsigned char byteRet;//登陆结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x01不支持的属性，0x02权限冲突
}RetVcSetDecProfile;

// 控制器获取解码器工作参数
typedef struct _ReqVcGetDecProfile
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcGetDecProfile;
typedef struct _RetVcGetDecProfile
{
    unsigned char szDecIp[4];//解码器新的IP
    unsigned char szDecNetmask[4];//解码器子网掩码
    unsigned char szDecGateway[4];//解码器网关
    unsigned char szDecNetManagerIp[4];//解码器网管设备地址
    unsigned char szDecMulticastIp[4];//解码器组播地址
    unsigned char szDecTcpPort[2];//解码器工作端口TCP
    unsigned char szDecUdpPort[2];//解码器工作端口UDP
    unsigned char szDecId[4];// 解码器设备编号，全0x00
}RetVcGetDecProfile;

// 控制器查询解码器工作状态
typedef struct _ReqVcGetDecState
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcGetDecState;
typedef struct _RetVcGetDecState
{
    unsigned char byteState;// 0x00未连接编码器 0x01接收单播流 0x02接收组播流
}RetVcGetDecState;

//控制器设置解码器相编码器的单播请求
typedef struct _ReqVcSetUnicastEnc
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncId[4];//编码器编号，全0x00
    unsigned char szEncIp[4];//编码器IP
    unsigned short usTcpPort;//TCP端口
    unsigned short usUdpPort;//udp端口
    char szKey[64];//编码器登陆密钥，全0x00
    unsigned char byteAction;//连接后动作，0x00登陆后不动作 0x01登陆后启动编码器
}ReqVcSetUnicastEnc;
typedef struct _RetVcSetUnicastEnc
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetVcSetUnicastEnc;

//控制器查询解码器连接的单播编码器
typedef struct _ReqVcGetUnicastEnc
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcGetUnicastEnc;
typedef struct _RetVcGetUnicastEnc
{
    unsigned char szEncId[4];//编码器ID，全0x00
    unsigned char szEncIp[4];//编码器IP
    unsigned char szTcpPort[2];//TCP端口
    unsigned char szUdpPort[2];//UDP端口
    char szConnectedTime[4];//0x00
}RetVcGetUnicastEnc;

// 控制器设置解码器图像锁定
typedef struct _ReqVcLockDecVideo
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcLockDecVideo;
typedef struct _RetVcLockDecVideo
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x01权限不足
}RetVcLockDecVideo;

// 控制器设置解码器图像解除锁定
typedef struct _ReqVcUnlockDecVideo
{
    unsigned char szVcIp[4];//视频控制器IP
    unsigned char szDecIp[4];//解码器IP
}ReqVcUnlockDecVideo;
typedef struct _RetVcUnlockDecVideo
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x01权限不足
}RetVcUnlockDecVideo;

//解码器申请加入制定的组播地址
typedef struct _ReqVcSetMulticastGroup
{
    unsigned char szMulticastIp[4];//组播地址，解码器接受的组播地址
}ReqVcSetMulticastGroup;
typedef struct _RetVcSetMulticastGroup
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetVcSetMulticastGroup;

// 设置解码器转发的 IP地址
typedef struct _ReqVcSetRelay
{
    unsigned char byteType;//转发类型 0x00单播，0x01组播
    unsigned char szIp[4];//转发IP地址
    unsigned char byteStart;//起始/停止:0x00停止转发，0x01开始转发
}ReqVcSetRelay;
typedef struct _RetVcSetRelay
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetVcSetRelay;

/************************************
 *
 * 接口6 解码器<-->编码器
 *
 ************************************/
/////////////////////////
//指令编号
#define REQ_DECODER_LOGIN_ENCODER 0x0601 //登录编码器
#define RET_DECODER_LOGIN_ENCODER 0x0602 //登录编码器返回
#define REQ_DECODER_LOGOUT_ENCODER 0x0603 //登出编码器
#define RET_DECODER_LOGOUT_ENCODER 0x0604 //登录编码器返回
#define REQ_DECODER_CONNECT_ENCODER 0x0605 //建立编码器->解码器输出
#define RET_DECODER_CONNECT_ENCODER 0x0606 //建立编码器->解码器输出返回
#define REQ_DECODER_DISCONNECT_ENCODER 0x0607 //拆除编码器->解码器输出
#define RET_DECODER_DISCONNECT_ENCODER 0x0608 //拆除编码器->解码器输出返回
#define MSG_ENCODER_KEEPALIVE_DECODER 0x0609 //解码器心跳信号

/////////////////////////////////
//消息结构体
/////////////////////////////////
// 解码器发送登陆编码器请求
typedef struct _ReqDecLoginEnc
{
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncIp[4];//编码器IP
    unsigned char byteLevel;//操作员优先级，0x00
    char szKey[64];//登陆密钥，全0x00
}ReqDecLoginEnc;
typedef struct _RetDecLoginEnc
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x01无效设备，0x02无效密钥，0x03已登录
}RetDecLoginEnc;

// 解码器发送登出编码器请求
typedef struct _ReqDecLogoutEnc
{
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncIp[4];//编码器IP
}ReqDecLogoutEnc;
typedef struct _RetDecLogoutEnc
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetDecLogoutEnc;

//解码器向编码器发送建立编码器->解码器输出连接的请求
typedef struct _ReqDecConnectEnc
{
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncId[4];//编码器编号,0x00
    unsigned char szEncIp[4];//编码器IP
    unsigned char szUdpPort[2];//UDP端口    
}ReqDecConnectEnc;
typedef struct _RetDecConnectEnc
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetDecConnectEnc;

//解码器向编码器发送拆除编码器->解码器输出连接的请求
typedef struct _ReqDecDisconnectEnc
{
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncIp[4];//编码器IP
    unsigned char szVcIp[4];//视频控制器IP
}ReqDecDisconnectEnc;
typedef struct _RetDecDisconnectEnc
{
    unsigned char byteRet;//结果 0x00失败，0x01成功
    unsigned char byteErr;//说明:0x00
}RetDecDisconnectEnc;

// 解码器向编码器发送心跳信号
typedef struct _MsgEncKeepaliveDec
{
    unsigned char szDecIp[4];//解码器IP
    unsigned char szEncIp[4];//编码器IP
}MsgEncKeepaliveDec;


#pragma pack (pop)

#endif //ST_PROTOCOL_DEFINE_H_


