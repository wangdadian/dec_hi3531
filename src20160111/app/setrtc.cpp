#include <stdio.h>
#include <sys/time.h>
#include "mdrtc.h"
#include "define.h"
#include "Facility.h"
#include "IniFile.h"
#include "dwstype.h"
#include "PublicDefine.h"

int main()
{
    // 获取RTC时间
    char szTime[100] = {0};
    time_t time = CRtc::read_rtc();
	tm sTime;
	localtime_r(&time, &sTime);
	sprintf(szTime, "%02i-%02i-%02i %02i:%02i:%02i", sTime.tm_year + 1900, \
                        sTime.tm_mon + 1, sTime.tm_mday, \
                        sTime.tm_hour, sTime.tm_min, sTime.tm_sec);
    
    _DEBUG_(">>>>>>Set Time: %s<<<<<<", szTime);
    // 更新系统时间
    char szCmd[100] = {0};
    sprintf(szCmd, "date -s \"%s\"", szTime);
    system(szCmd);    

    return 0;
}




