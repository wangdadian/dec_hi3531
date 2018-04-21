#ifndef DEVUPDATE_PROTOCOL_DEFINE_H_
#define DEVUPDATE_PROTOCOL_DEFINE_H_

#pragma pack (push,1)

#define UP_MULTI_ADDR "224.20.15.210"
#define UP_MULTI_PORT 53541
#define UP_SERVER_PORT 20152
#define UP_VERSION 0x0201
#define UP_HEAD_FLAG "UPGRADE"

typedef struct _UpHead
{
    char flag[8]; // 标志字段UPGRADE
    unsigned int version; // 版本号0x0201
    unsigned int cmd; // 通讯指令
    unsigned int workid; // 消息流水号
    unsigned int length; // 消息体长度(不包含消息头)
    char reserved[40];// 预留，0x00
}struUpHead;

enum UpdateCmd
{
    // 组播发现设备
    UP_CMD_REQ_DETECT_DEVICES=0xf000,
    UP_CMD_RET_DETECT_DEVICES=0xf001,
    
    // 升级设备
    UP_MSG_REQ_UPGRADE_DEV=0xf103,
    UP_MSG_RET_UPGRADE_DEV=0xf104,
    // 重启设备
    UP_MSG_REQ_REBOOT_DEV=0xf105,
    UP_MSG_RET_REBOOT_DEV=0xf106,
    // 时间校准
    UP_MSG_REQ_SETTIME=0xf107,
    UP_MSG_RET_SETTIME=0xf108,
    
};

enum UP_ERROR_NO
{
    UP_E_SUCCESS = 0x00,
    UP_E_FAILED = 0x01,
    UP_E_INVALID_FILE = 0x02,

};

///////////////////////////////////////////////////////////////////
// 组播发现设备
typedef struct _UpCmdReqDetectDev
{
    struUpHead head;

}struUpCmdReqDetectDev;

typedef struct _UpDevInfo
{
    char szDevName[16]; // 设备名称
    char szSysVersion[32];// 设备版本
    char szBuildtime[32];// 设备创建日期
    char szDevIp[32]; // 设备IP *
    char szDevNetmask[32]; // 子网掩码
    char szDevGateway[32]; // 网关
    char szDevMac[32]; // MAC地址 *
    unsigned int iCtlFlag; // 是否被连接 1-是，0-否 *
    char szCtlIp[32]; // 连接方Ip
    char szReserved[40]; // 保留

}struUpDevInfo;

typedef struct _UpCmdRetDetectDev
{
    struUpHead head;
    struUpDevInfo body;
}struUpCmdRetDetectDev;

// 升级设备
typedef struct _UpReqUpgradeDev
{
    struUpHead head;
    char *szData; // 升级数据
}struUpReqUpgradeDev;

typedef struct _UpRetUpgradeDev
{
    struUpHead head;
    int iRet;
}struUpRetUpgradeDev;

// 设备重启
typedef struct _UpReqRebootDev
{
    struUpHead head;

}struUpReqRebootDev;

typedef struct _UpRetRebootDev
{
    struUpHead head;
    int iRet;
}struUpRetRebootDev;

// 时间校准
typedef struct _UpReqSettime
{
    struUpHead head;
    char szTime[40]; // 时间"2015-02-10 18:30:59"
}struUpReqSettime;

typedef struct _UpRetSettime
{
    struUpHead head;
    int iRet;
}struUpRetSettime;


#pragma pack (pop)

#endif //DEVUPDATE_PROTOCOL_DEFINE_H_






