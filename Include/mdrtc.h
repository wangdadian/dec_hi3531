#ifndef MDRTC_H
#define MDRTC_H

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "define.h"
#include "Facility.h"
#include "IniFile.h"
#include "dwstype.h"
#include "PublicDefine.h"

typedef struct linux_rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#define RTC_SET_TIME  _IOW('p', 0x0a, struct linux_rtc_time) /* Set RTC time    */
#define RTC_RD_TIME   _IOR('p', 0x09, struct linux_rtc_time) /* Read RTC time   */


class CRtc
{
public:
    CRtc();
    ~CRtc();
    
    static void write_rtc(time_t t, int utc=0);
    static time_t read_rtc(int utc=0);

};


#endif

