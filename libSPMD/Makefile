all: tpbench hackbench libSPMD.a
CFLAGS=-O2 -g -Wall -D__NDEBUG
tpbench: tpbench.o libSPMD.a
	g++ -o $@ ../profiler.o ../toolkits.o ../mtsupport.o $< -L/usr/lib64 -lboost_thread-mt $(CFLAGS) libSPMD.a
tpbench.o:%.o:%.cc 
	g++ -o $@ -c $< -std=c++0x $(CFLAGS)
hackbench: hackbench.c 
	cc  -o $@ $< -lpthread $(CFLAGS)
libSPMD.a: warp.o aux.o
	ar cr libSPMD.a warp.o aux.o
#	cc -otest_warp warp.o aux.o -lpthread
warp.o: warp.c
	cc -c warp.c $(CFLAGS)
aux.o: aux.c
	cc -c aux.c $(CFLAGS)

.PHONY: all clean
clean:
	-rm tpbench hackbench test_warp *.o *.a *~
