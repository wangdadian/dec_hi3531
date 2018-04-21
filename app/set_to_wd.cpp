#include <stdlib.h>
#include <stdio.h>
#include  <sys/ioctl.h>
#include  <sys/stat.h>
#include  <arpa/inet.h>
#include  <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#define    WATCHDOG_IOCTL_BASE    'W'
#define    WDIOC_SETTIMEOUT     _IOWR(WATCHDOG_IOCTL_BASE, 6, int)

int  foo(int time)
{
	int wd_fd = open("/dev/watchdog", O_RDWR, 0);
	if(wd_fd == -1)
	{
		printf("Cannot open %s (%s)\n", "/dev/watchdog", strerror(errno));
		return -1;
	}
    
    ioctl(wd_fd, WDIOC_SETTIMEOUT, &time);  

	close(wd_fd);
	wd_fd = -1;
    return 0;
}

int main(int argc, char **argv)
{
    int time = 1200;
    if(argc>=2)
    {
        time = atoi(argv[1]);
    }
    
    printf("set timeout seconds to [%d]\n", time);
    return foo(time);
}



