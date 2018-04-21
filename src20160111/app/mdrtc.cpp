#include "mdrtc.h"

void CRtc::write_rtc(time_t t, int utc/*=0*/)
{
	int rtc;
	struct tm tm;
	
#ifdef HI3520D
	const char *dev_name = "/dev/hi_rtc";
	rtc = open(dev_name, O_RDWR);
	if(rtc < 0)
	{
		printf("open %s failed\n", dev_name);
		return;
	}
#else
	if (( rtc = open( "/dev/rtc", O_WRONLY )) < 0 ) {
		if (( rtc = open ( "/dev/misc/rtc", O_WRONLY )) < 0 )
			printf ( "Could not access RTC" );
	}
#endif
	
	//csp modify
	//tm = *( utc ? gmtime ( &t ) : localtime ( &t ));//根据时区获得相应时间
	if(utc)
	{
		gmtime_r(&t, &tm);
	}
	else
	{
		localtime_r(&t, &tm);
	}
	
	tm.tm_isdst = 0;
	
#ifdef HI3520D
	rtc_time_t hitm;
	hitm.year   = tm.tm_year + 1900;
	hitm.month  = tm.tm_mon + 1;
	hitm.date   = tm.tm_mday;
	hitm.hour   = tm.tm_hour;
	hitm.minute = tm.tm_min;
	hitm.second = tm.tm_sec;
	hitm.weekday = 0;
	int ret = ioctl(rtc, HI_RTC_SET_TIME, &hitm);
	if(ret < 0)
	{
		printf("ioctl: HI_RTC_SET_TIME failed\n");
	}
	//printf("HI_RTC_SET_TIME:\n");
	//printf("year:%d\n", hitm.year);
	//printf("month:%d\n",hitm.month);
	//printf("date:%d\n", hitm.date);
	//printf("hour:%d\n", hitm.hour);
	//printf("minute:%d\n", hitm.minute);
	//printf("second:%d\n", hitm.second);
#else
	if ( ioctl ( rtc, RTC_SET_TIME, &tm ) < 0 )
		printf ( "Could not set the RTC time" );
#endif
	
	close( rtc );
	
	//tl_venc_update_basetime();
	//nvr_sync_time();
	
	//printf("write_rtc:rz_venc_update_basetime......\n");
	
	//read_rtc(utc);
}

time_t CRtc::read_rtc(int utc/*=0*/)
{
	int rtc;
	struct tm tm_now;
	
#ifndef HI3520D
	char *oldtz = 0;
#endif
	
	time_t t = 0;
	
#ifdef HI3520D
	const char *dev_name = "/dev/hi_rtc";
	rtc = open(dev_name, O_RDWR);
	if(rtc < 0)
	{
		printf("open %s failed\n", dev_name);
		return 0;
	}
	
	rtc_time_t hitm;
	memset ( &hitm, 0, sizeof(hitm) );
	memset ( &tm_now, 0, sizeof( struct tm ) );
	int ret = ioctl(rtc, HI_RTC_RD_TIME, &hitm);
	if(ret < 0)
	{
		printf("ioctl: HI_RTC_RD_TIME failed\n");
	}
	
	tm_now.tm_year = hitm.year - 1900;
	tm_now.tm_mon = hitm.month - 1;
	tm_now.tm_mday = hitm.date;
	tm_now.tm_hour = hitm.hour;
	tm_now.tm_min = hitm.minute;
	tm_now.tm_sec = hitm.second;
	tm_now.tm_wday = hitm.weekday;
	//printf("HI_RTC_RD_TIME:\n");
	//printf("year:%d\n", hitm.year);
	//printf("month:%d\n",hitm.month);
	//printf("date:%d\n", hitm.date);
	//printf("hour:%d\n", hitm.hour);
	//printf("minute:%d\n", hitm.minute);
	//printf("second:%d\n", hitm.second);
#else
	if (( rtc = open ( "/dev/rtc", O_RDONLY )) < 0 ) {
		if (( rtc = open ( "/dev/misc/rtc", O_RDONLY )) < 0 )
			printf ( "Could not access RTC" );
	}
	
	memset ( &tm_now, 0, sizeof( struct tm ));
	if ( ioctl ( rtc, RTC_RD_TIME, &tm_now ) < 0 )
		printf ( "Could not read time from RTC" );
#endif
	
	tm_now.tm_isdst = -1; // not known
	
	close ( rtc );
	
#ifndef HI3520D
	if ( utc ) {
		oldtz = getenv ( "TZ" );
		setenv ( "TZ", "UTC 0", 1 );
		tzset ( );
	}
#endif
	
	t = mktime ( &tm_now );
	
#ifndef HI3520D
	if ( utc ) {
		if ( oldtz )
			setenv ( "TZ", oldtz, 1 );
		else
			unsetenv ( "TZ" );
		tzset ( );
	}
#endif
	
	return t;
}