all: tpbench hackbench
CFLAGS=-O2 -g -Wall
tpbench: tpbench.o 
	g++ -o $@ ../profiler.o ../toolkits.o ../mtsupport.o $< -lboost_thread $(CFLAGS)
tpbench.o:%.o:%.cc 
	g++ -o $@ -c $< -std=c++0x $(CFLAGS)
hackbench: hackbench.c 
	cc  -o $@ $< -lpthread $(CFLAGS)

.PHONY: all clean
clean:
	-rm tpbench hackbench *.o *~