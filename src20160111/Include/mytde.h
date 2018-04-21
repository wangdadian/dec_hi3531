#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#include "hifb.h"

#include "hi_comm_vo.h"
#include "mpi_sys.h"

#include "mpi_vo.h"
#include "sample_comm.h"
#include "PublicDefine.h"
#include "define.h"


#define TDE_PRINT printf

#define VoDev 0
#define VoChn 0


#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define BPP     2

#ifndef HI_FLOAT
typedef float HI_FLOAT;
#endif


long MyTest2();
long MyTest();



