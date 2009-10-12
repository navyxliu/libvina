CC:=gcc
CXX:=g++
AR:=ar

CFLAGS= -g -Wall -D__NDEBUG -DSMP_SUPPORT 
CFLAGS+=-DSPMD_SEM_KEY="\"/tmp/evan2\"" #-D__TIMELOG 
CFLAGS+=-DLINUX

#define SYNC_SIGNAL will send signal for all task.
#otherwise, use a array of semaphore.
CFLAGS+=#-DSYNC_SIGNAL

TARGETS=tiny hackbench libSPMD.a

all: hackbench tiny libSPMD.a
tiny: tiny.o libSPMD.a
	$(CC) -o $@ $^ -lpthread -lrt
hackbench: hackbench.c 
	$(CC) -o $@ $< -lpthread $(CFLAGS)
libSPMD.a: warp.o aux.o
	$(AR) cr libSPMD.a warp.o aux.o
%.o: %.c Makefile
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: all clean
clean:
	-rm *.o *~ $(TARGETS)
