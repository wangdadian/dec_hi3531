ifndef ROM
ROM=16
endif

#解码器内存大小，16或32
ifeq ($(ROM),32)
ROM32_DEF="ROM32"
endif

ifeq ($(ROM),16)
ROM16_DEF="ROM16"
endif

ifndef (CXX)
CXX=g++
LINKCXX=g++	
endif

ifndef (MIDDLEWAREINC)
MIDDLEWAREINC=-I/opt/include/Middleware -I/opt/include/common/ 
endif

ARM=1
ARMVER=100nptl



ifndef DEBUG
DEBUG=1
endif

VERSION="\"1.0.0.4\""

BUILDDATE= "\"`date +%Y-%m-%d\ %k:%M:%S`\""

MODULENAME="\"decoder\""

ifeq ($(ROM),16)
	BUILDINFO=-DVERSION=$(VERSION) -DBUILDDATE=$(BUILDDATE) -DMODULENAME=$(MODULENAME) -DROM16_DEF=$(ROM16_DEF) -D_FILE_OFFSET_BITS=64 -DLINUX
else
	BUILDINFO=-DVERSION=$(VERSION) -DBUILDDATE=$(BUILDDATE) -DMODULENAME=$(MODULENAME) -DROM32_DEF=$(ROM32_DEF) -D_FILE_OFFSET_BITS=64 -DLINUX
endif


#
#goahead webserver
# User Management switch
UMSW	= -DUSER_MANAGEMENT_SUPPORT
# Digest Access switch
DASW	= -DDIGEST_ACCESS_SUPPORT
CFLAGS1	= -DWEBS -DUEMF -DOS="LINUX" -DLINUX $(UMSW) $(DASW) $(SSLSW) $(IFMODSW)

ifeq ($(DEBUG),1)
CXXFLAGS=-g  $(COMPILEMAIN) -fPIC -rdynamic -Wall  $(INCLUDEDIR) -Wno-deprecated   $(BUILDINFO) -finput-charset=UTF-8  $(CFLAGS1)
else
CXXFLAGS=-O2  $(COMPILEMAIN) -fPIC -Wall $(INCLUDEDIR) -Wno-deprecated   $(BUILDINFO) -finput-charset=UTF-8  $(CFLAGS1)
endif


ifeq ($(ENC),1)
	CXXFLAGS += -DENCODER=1
endif

ifeq ($(X64),1)
	M64=64 
	DEST=/opt/MVP64
	COMPILEMAIN=-m$(M64)
else
	ifeq ($(X32),1)
	M64=32 
	DEST=/opt/MVP
	else

	CPU_ARCH=$(shell uname -m)
	ifeq (64,$(findstring 64,$(CPU_ARCH)))
		M64=64 
		DEST=/opt/MVP64
	else
		M64=32
		DEST=/opt/MVP
	endif

	endif
	COMPILEMAIN=-m$(M64)

endif


ifeq (64,$(findstring 64,$(COMPILEMAIN)))
	COMPILETARGET=64
else
	COMPILETARGET=
endif



ifeq ($(ARM),1)
		#CXX="arm-hisiv100nptl-linux-g++"
		#LINKCXX=arm-hisiv100nptl-linux-g++	
		#STRIP="arm-hisiv100nptl-linux-strip"
		CXX="arm-hisiv$(ARMVER)-linux-g++"
		LINKCXX=arm-hisiv$(ARMVER)-linux-g++	
		STRIP="arm-hisiv$(ARMVER)-linux-strip"
		COMPILEMAIN=
		DEST=/opt/libarm$(ARMVER)
		
		COMPILETARGET=arm$(ARMVER)
endif 



SDK_PATH=../../lib
EXTDRV=extdrv

INCLUDEDIR=$(MIDDLEWAREINC) \
-I/usr/local/include \
-I../Include \
-I/opt/include \
-I/opt/include/ES2PS \
-I/opt/include/ONVIF \
-I/opt/include/osip2 \
-I/opt/include/eXosip2 \
-I/opt/include/28181 \
-I/opt/include/tjgs \
-I../../../../MVP/MainBranche/server/src/Lib/OnvifSDK/OnvifProtocol \
-I../../../../MVP/MainBranche/server/src/Lib/RTSPStreamLib \
-I/opt/include/LiveMedia/liveMedia/ \
-I/opt/include/LiveMedia/groupsock \
-I/opt/include/LiveMedia/BasicUsageEnvironment \
-I/opt/include/LiveMedia/UsageEnvironment \
-I/opt/include/ES2PS \
-I$(SDK_PATH)/mpp/sample/common/ \
-I$(SDK_PATH)/mpp/component/hifb/include \
-I$(SDK_PATH)/mpp/include/ \
-I$(SDK_PATH)/mpp/$(EXTDRV)/tw2865 \
-I$(SDK_PATH)/mpp/$(EXTDRV)/tw2960 \
-I$(SDK_PATH)/mpp/$(EXTDRV)/tlv320aic31 \
-I$(SDK_PATH)/dws/ \
-I$(SDK_PATH)/dws/include

ifeq ($(ROM),16)
		MPPLIB=$(SDK_PATH)/mpp/lib/libmpi.a \
		$(SDK_PATH)/mpp/lib/libjpeg.a \
		$(SDK_PATH)/mpp/lib/libhdmi.a \
		$(SDK_PATH)/mpp/lib/libaec.a \
		$(SDK_PATH)/mpp/lib/libanr.a \
		$(SDK_PATH)/mpp/lib/libpciv.a \
		$(SDK_PATH)/mpp/lib/libtde.a \
		$(SDK_PATH)/mpp/lib/libVoiceEngine.a \
		$(SDK_PATH)/mpp/lib/libresampler.a  \
		$(SDK_PATH)/dws/lib/libdws.a \
		/opt/libarm$(ARMVER)/libwebs.a
else
		MPPLIB=$(SDK_PATH)/mpp/lib/libmpi.a \
		$(SDK_PATH)/mpp/lib/libjpeg.a \
		$(SDK_PATH)/mpp/lib/libhdmi.a \
		$(SDK_PATH)/mpp/lib/libaec.a \
		$(SDK_PATH)/mpp/lib/libanr.a \
		$(SDK_PATH)/mpp/lib/libpciv.a \
		$(SDK_PATH)/mpp/lib/libtde.a \
		$(SDK_PATH)/mpp/lib/libVoiceEngine.a \
		$(SDK_PATH)/mpp/lib/libresampler.a  \
		$(SDK_PATH)/dws/lib/libdws.a \
		/opt/libarm$(ARMVER)/libwebs.a \
		./libreceiver.a
endif

STREAMLIB=/opt/libarm$(ARMVER)/ES2PS/libES2PS.a \
/opt/libarm$(ARMVER)/LiveMedia/libBasicUsageEnvironment.a  \
/opt/libarm$(ARMVER)/LiveMedia/libgroupsock.a \
/opt/libarm$(ARMVER)/LiveMedia/libliveMedia.a \
/opt/libarm$(ARMVER)/LiveMedia/libUsageEnvironment.a



#/opt/libarm/sip/libeXosip2.a /opt/libarm/sip/libosip2.a  /opt/libarm/sip/libosipparser2.a 

#/opt/libarm/LiveMedia/libRtsp.a 
#/opt/libarm/ONVIF/libonvif.a

LIBDIR=/opt/lib$(COMPILETARGET)/libcommon.a  $(STREAMLIB) $(MPPLIB) -lpthread  -L/opt/libarm$(ARMVER)  -L/opt/libarm$(ARMVER)/sip  -L/opt/libarm -lRtsp -ltjgs -leXosip2 -losip2 -losipparser2 -lssl -lcrypto
#-lcrypto   -lssl -lcrypto

ifeq ($(ROM),16)
		OBJS=Hi3531Global.o NetSource.o  main.o  Decoder.o Source.o ChannelManager.o  MDProtocol.o Protocol.o ChannelConfig.o Vmux6671Protocol.o \
		AT200Protocol.o Forwardport.o FlashRW.o 2301DecoderProtocol.o DecoderSetting.o ConfigMtdFile.o UdpSource.o   encoder.o mdrtc.o  StreamTransform.o  \
		Text2Bmp.o PtzCtrl.o AlarmCtrl.o ZyProtocol.o StProtocol.o TJGSProtocol.o UpProtocol.o EncoderSetting.o TextOnYUV.o DecoderOsd.o \
		#RTSPSource.o 
		#28181Protocol.o  
else
		OBJS=Hi3531Global.o NetSource.o  main.o  Decoder.o Source.o ChannelManager.o  MDProtocol.o Protocol.o ChannelConfig.o Vmux6671Protocol.o \
		AT200Protocol.o Forwardport.o FlashRW.o 2301DecoderProtocol.o DecoderSetting.o ConfigMtdFile.o UdpSource.o   encoder.o mdrtc.o  StreamTransform.o  \
		Text2Bmp.o PtzCtrl.o AlarmCtrl.o RTSPSource.o ZyProtocol.o StProtocol.o TJGSProtocol.o UpProtocol.o EncoderSetting.o TextOnYUV.o DecoderOsd.o \
		OnvifRcvProtocol.o
		#28181Protocol.o 
endif

COMM_SRC := $(wildcard $(SDK_PATH)/mpp/sample/common/*.c)
COMM_OBJ := $(COMM_SRC:%.c=%.o)


LDFLAGS=-shared -Wl,-soname,$@ 

DLLS=

ifeq ($(ENC),1)
	APP=encoder
else
	APP=decoder
endif 

ALL_EXE=$(APP) upgrade dumpconfig dumpconfigEx importconfig setrtc set_to_wd
all: $(ALL_EXE)

#ifeq($(ENC),1)
$(APP):$(OBJS)  
	$(LINKCXX) $(CXXFLAGS)   -o $@  $(OBJS)  $(COMM_OBJ) $(LIBDIR)
#else
$(APP):$(OBJS)  
	$(LINKCXX) $(CXXFLAGS)   -o $@ $(OBJS)  $(COMM_OBJ) $(LIBDIR)
#endif

upgrade:upgrade.o FlashRW.o
	$(LINKCXX) $(CXXFLAGS)   -o $@ upgrade.o FlashRW.o  
set_to_wd:set_to_wd.o
	$(LINKCXX) $(CXXFLAGS)   -o $@ set_to_wd.o
dumpconfig:DumpConfig.o FlashRW.o
	$(LINKCXX) $(CXXFLAGS)   -o $@ DumpConfig.o FlashRW.o 	

dumpconfigEx:DumpConfigEx.o FlashRW.o ConfigMtdFile.o
	$(LINKCXX) $(CXXFLAGS)	 -o $@ DumpConfigEx.o FlashRW.o ConfigMtdFile.o

importconfig:ImportConfig.o FlashRW.o
	$(LINKCXX) $(CXXFLAGS)   -o $@ ImportConfig.o FlashRW.o 	

setrtc:setrtc.o mdrtc.o
	$(LINKCXX) $(CXXFLAGS)   -o $@ setrtc.o mdrtc.o

%.o: %.cpp %.h %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< 

install:
	@mkdir -p /opt/dws
	cp $(APP) set_to_wd upgrade /opt/libarm$(ARMVER)/libRtsp.so /opt/dws 
clean:
	@rm -rf $(OBJS) $(ALL_EXE)
	
