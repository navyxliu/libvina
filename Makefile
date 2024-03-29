#This file is not supposed to be distributed.
#History:
#July. 2, 2009, 
#Sep. 29, 2009, massively re-organize this file.

PWD    := $(shell pwd)
SYSTEM := $(shell uname -s)

#compiler conf.
CXX=g++
CFLAGS=-g -std=c++0x $(TEST_INFO) $(OPT) -I.
OPT= -O3 -msse2
#Linux system support additional features, such as
# 1.PMC counter
# 2.high-resolution timer
ifeq ($(SYSTEM), Linux)
CFLAGS+=-DLINUX #-DPMC_SUPPORT 
LDFLAGS+=-lrt -lm
endif

#include TEST parameters 
include PARAMS 

#intel Math kernel library
MKLLIB=-L$(MKLPATH)/lib/em64t  \
	-Wl,--start-group $(MKLPATH)/lib/em64t/libmkl_intel_lp64.a\
	$(MKLPATH)/lib/em64t/libmkl_sequential.a \
	$(MKLPATH)/lib/em64t/libmkl_core.a -Wl,--end-group \
	-lpthread
#MKL library, link the parallel version
MKLLIB_P=-L$(MKLPATH)/lib/em64t  \
	-Wl,--start-group $(MKLPATH)/lib/em64t/libmkl_intel_lp64.a\
	$(MKLPATH)/lib/em64t/libmkl_intel_thread.a \
	$(MKLPATH)/lib/em64t/libmkl_core.a -Wl,--end-group \
	-liomp5 -lpthread

#libSPMD lib
SPMDPATH=./libSPMD
SPMDLIB=-L$(SPMDPATH) -lSPMD

#boost::thread lib
ifeq ($(SYSTEM), Linux)
ISSUE=$(shell cat /etc/issue)
#comment out
#ifeq ($(word 1, $(ISSUE)), Ubuntu) #Ubuntu, nhm, jw's 
#MKLPATH=/opt/intel/mkl/10.2.2.025
#BOOSTPATH=/root/source/boost_1_40_0
#BOOSTLIB=-L$(BOOSTPATH)/stage/lib -lboost_thread
#BOOSTINC=/usr/local/include/boost
ifeq ($(word 1, $(ISSUE)), Ubuntu) #ubuntu, epcc, xliu's
MKLPATH=/opt/intel/mkl/10.2.2.025
#BOOSTPATH=/
BOOSTLIB=-L/usr/lib/ -lboost_thread
BOOSTINC=/usr/include/boost
else                            #fedora, xliu's
MKLPATH=/opt/intel/mkl/10.2.1.017
BOOSTLIB=-L/usr/lib64 -lboost_thread-mt
BOOSTINC=/usr/include
endif
else ifeq ($(SYSTEM), Darwin)  #for mac osx 
BOOSTINC=/opt/local/include
BOOSTLIB=-L/opt/local/lib -lboost_thread-mt
else
#other system
endif

#ifneq (, $(findstring LIBSPMD, $(TESTINFO)))
#MTSUPPORT+=-I$(SPMDPATH) $(SPMDLIB)
#else
MTSUPPORT+=-I$(BOOSTINC) $(BOOSTLIB)
#endif

LDFLAGS+=$(MTSUPPORT)

#ifneq (,$(findstring MKL,$(TESTINFO)))
#CFLAGS+=-I$(MKLPATH)/include
#LDFLAGS+=$(MKLLIB)
#endif

AUX_OBJS = profiler.o toolkits.o mtsupport.o
OBJS = mat_mul.o lang_pipe.o dot_prod.o saxpy.o conv2d.o mm_omp.o tpbench.o test_cl.o sgemm.o
OBJS += $(AUX_OBJS)

all: mat_mul lang_pipe dot_prod conv2d saxpy tpbench

mat_mul: mat_mul.o $(AUX_OBJS) 
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)
sgemm: sgemm.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)
#matrix muliplication with openmp mlk
mm_omp: mm_omp.o
	$(CXX) -o $@ $< $(AUX_OBJS) $(MKLLIB_P) -lrt $(BOOSTLIB)

lang_pipe: lang_pipe.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

dot_prod: dot_prod.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)	

saxpy: saxpy.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

conv2d: conv2d.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS) 

tpbench: tpbench.o $(AUX_OBJS) ./libSPMD/libSPMD.a
	$(CXX) -o $@ $^ $(LDFLAGS) ./libSPMD/libSPMD.a 

#test opencl, only works on macosx
test_cl: test_cl.o
	$(CXX) -o $@ $^ $(LDFLAGS) toolkits.o -framework OpenCL -I./inc

$(OBJS):%.o:%.cc Makefile PARAMS
	$(CXX) -o $@ -c $< $(CFLAGS)



###################################################
#                   TEST                          #
###################################################
#TEST_SET = test_profiler test_toolkits test_trait test_img test_seq
TEST_SET = test_seq
TEST_OBJS = $(addsuffix .o, $(TEST_SET))

test: $(TEST_SET)
	@echo "target for $(SYSTEM)"
	for i in $(TEST_SET); do\
		echo "testing $$i"; \
		./$$i;\
	done

$(TEST_SET):%:%.cc #$(AUX_OBJS)
	$(CXX) -o $@  $(CFLAGS) $(LDFLAGS) $< #$(AUX_OBJS)


###################################################
#               Miscellaneous                     #
###################################################
LAST=`date +%y_%m_`$$((`date +%d`-1))
.PHONY: clean dist distclean lines all test_threadlib vina_tmp

vina_tmp:
	mkdir /tmp/tmp
clean: 
	-rm -f *.o timelog* mat_mul lang_pipe saxpy dot_prod conv2d $(TEST_SET)
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

