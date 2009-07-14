all: tpbench

tpbench: tpbench.o 
	g++ -o $@ ../profiler.o ../toolkits.o ../mtsupport.o $< -lboost_thread
tpbench.o:%.o:%.cc 
	g++ -o $@ -c $< -std=c++0x
