CC:=gcc
CXX:=g++
SYSTEM := $(shell uname -s)
CFLAGS= -g -Wall -D__NDEBUG -DSMP_SUPPORT 
CFLAGS+=-DSPMD_SEM_KEY="\"/tmp/evan2\"" #-D__TIMELOG 
CFLAGS+=-DLINUX

ifeq ($(SYSTEM), Linux)
ISSUE= $(shell cat /etc/issue)
ifeq ($(word 1, $(ISSUE)), Red)
THREAD_LIB = boost_thread-gcc44-mt#for jw system
MTSUPPORT+= -L /root/Desktop/boost_1_39_0/stage/lib
BOOST_PATH=/usr/local/include/boost-1_39/
else#fedora7, default
THREAD_LIB=boost_thread-mt
MTSUPPORT+= -L /usr/lib64
BOOST_PATH=/usr/include
endif
#general linux features
CFLAGS += -DLINUX -D__USEPOOL #-DPMC_SUPPORT
LDFLAGS += -lrt
else ifeq ($(SYSTEM), Darwin)
BOOST_PATH = /opt/local/include
THREAD_LIB = boost_thread-mt
LDFLAGS += -L/opt/local/lib
else
#other system
endif

all: tpbench hackbench tiny libSPMD.a
tpbench: tpbench.o libSPMD.a
	$(CXX) -o $@ ../profiler.o ../toolkits.o ../mtsupport.o $^ $(CFLAGS) -l$(THREAD_LIB) $(MTSUPPORT)
tiny: tiny.o libSPMD.a
	$(CC) -o $@ $^ -lpthread -lrt
tpbench.o:%.o:%.cc 
	$(CXX) -o $@ -c $< -std=c++0x $(CFLAGS) -I$(BOOST_PATH) 
hackbench: hackbench.c 
	$(CC) -o $@ $< -lpthread $(CFLAGS)
libSPMD.a: warp.o aux.o
	ar cr libSPMD.a warp.o aux.o
warp.o: warp.c
	$(CC) -c warp.c $(CFLAGS)
aux.o: aux.c
	$(CC) -c aux.c $(CFLAGS)
.PHONY: all clean
clean:
	-rm tpbench hackbench *.a *.o *~
