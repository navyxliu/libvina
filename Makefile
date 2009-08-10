#This file is not supposed to be distributed.

SYSTEM := $(shell uname -s)
PWD    := $(shell pwd)

#compiler conf.
CXX = g++
CFLAGS = -g -I $(BOOST_PATH) -std=c++0x $(TEST_INFO) $(OPT) #-fopenmp
MTSUPPORT = -l$(THREAD_LIB) #-lgomp
GLSUPPORT = -lGL -lGLU -lglut
LDFLAGS = -lm $(MTSUPPORT) -lpng
OPT= -O2 -msse

#test parameters
TEST_INFO = -DMM_TEST_TYPE=float -DMM_TEST_GRANULARITY=256 -DMM_TEST_SIZE_N=1024 -DMM_TEST_K=4 -DVEC_TEST_TYPE=int -DVEC_TEST_GRANULARITY=2500000 -DVEC_TEST_SIZE_N=10000000 -DVEC_TEST_K=2 -DIMG_TEST_SIZE_M=256 -DIMG_TEST_SIZE_N=256 -DIMG_TEST_GRANULARITY=128 -D__NDEBUG #-DFORK_AT_CALLSITE -DVIZ_CALLSITE

ifeq ($(SYSTEM), Linux)
ISSUE= $(shell cat /etc/issue)
ifeq ($(word 1, $(ISSUE)), Red)
THREAD_LIB = boost_thread-gcc44-mt#for jw system
MTSUPPORT+= -L /root/Desktop/boost_1_39_0/stage/lib
BOOST_PATH=/usr/local/include/boost-1_39/
else#fedora7, default
THREAD_LIB=boost_thread	
BOOST_PATH=/usr/local/include
endif
#general linux features
CFLAGS += -DPMC_SUPPORT -DLINUX -D__USEPOOL
LDFLAGS += -lrt
else ifeq ($(SYSTEM), Darwin)
BOOST_PATH = /opt/local/include
THREAD_LIB = boost_thread-mt
LDFLAGS += -L/opt/local/lib
else
#other system
endif

AUX_OBJS = profiler.o toolkits.o mtsupport.o imgsupport.o
OBJS = mat_mul.o test_pipe.o dot_prod.o saxpy.o conv2d.o
OBJS += $(AUX_OBJS)


all: mat_mul lang_pipe dot_prod conv2d saxpy

mat_mul: mat_mul.o $(AUX_OBJS) 
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

lang_pipe: test_pipe.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

dot_prod: dot_prod.o $(AUX_OBJS)
	
saxpy: saxpy.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)
conv2d: conv2d.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)
$(OBJS):%.o:%.cc frame.hpp Makefile
	$(CXX) -o $@ -c $< $(CFLAGS)



###################################################
#                   TEST                          #
###################################################
TEST_SET = test_profiler test_toolkits test_trait test_img
TEST_OBJS = $(addsuffix .o, $(TEST_SET))

test: $(TEST_SET)
	@echo "target for $(SYSTEM)"
	for i in $(TEST_SET); do\
		echo "testing $$i"; \
		./$$i;\
	done

$(TEST_SET):%:%.cc $(AUX_OBJS)
	$(CXX) -o $@  $(CFLAGS) $(LDFLAGS) $(AUX_OBJS)  $<
tpbench.o : libSPMD/tpbench.cc
	g++ -O2 -c -o tpbench.o libSPMD/tpbench.cc -std=c++0x -I /usr/local/include/boost-1_39/
tpbench: tpbench.o profiler.o toolkits.o mtsupport.o imgsupport.o
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS) -I /usr/local/include/boost-1_39/


###################################################
#               Miscellaneous                     #
###################################################
LAST=`date +%y_%m_`$$((`date +%d`-1))
.PHONY: clean dist distclean lines all test_threadlib

clean: 
	-rm -f *.o mat_mul lang_pipe saxpy dot_prod conv2d $(TEST_SET) test_threadlib tpbench vina.loops_per_ms
distclean:
	-rm -f *~ ._*
dist: 
	@make clean && make distclean
	@-rm -fr /tmp/libvina
	cd .. && tar -cjf libvina.`date +%y_%m_%d`.tar.bz2 libvina/
	tar xjf $(dir $(PWD))libvina.$(LAST).tar.bz2 -C /tmp/
	diff -Nur /tmp/libvina $(dir $(PWD))libvina/ > $(dir $(PWD))new.patch
lines: 
	find | grep ".\(c\|h\|hpp\|cc\)$$" | xargs wc -l
test_threadlib: tpbench
	cp test_threadlib.sh test_threadlib
	chmod 777 test_threadlib

