#This file is not supposed to be distributed.

SYSTEM := $(shell uname -s)

#compiler conf.
CXX = g++
CFLAGS = -g -I $(BOOST_PATH) -std=c++0x $(TEST_INFO) $(OPT) -fopenmp
MTSUPPORT = -l$(THREAD_LIB)
LDFLAGS = -lm $(MTSUPPORT) -lgomp
OPT= #-O3 -msse2

include test_parameter

ifeq ($(SYSTEM), Linux)
ISSUE= $(shell cat /etc/issue)
ifeq ($(word 1, $(ISSUE)), Ubuntu)
THREAD_LIB = boost_thread-gcc44-mt#for jw system
MTSUPPORT+= -L /root/Desktop/boost_1_39_0/stage/lib
BOOST_PATH=/usr/local/include/boost-1_39/
else
THREAD_LIB=boost_thread	
BOOST_PATH=/usr/local/include
endif
CFLAGS += -DPMC_SUPPORT
else ifeq ($(SYSTEM), Darwin)
BOOST_PATH = /opt/local/include
THREAD_LIB = boost_thread-mt
LDFLAGS += -L/opt/local/lib
else
#other system
endif

AUX_OBJS = profiler.o toolkits.o mtsupport.o
OBJS = mat_mul.o test_vector.o test_pipe.o dot_prod.o saxpy.o
OBJS += $(AUX_OBJS)


all: vec_add mat_mul lang_pipe dot_prod

mat_mul: mat_mul.o $(AUX_OBJS) 
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

vec_add: test_vector.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

lang_pipe: test_pipe.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

dot_prod: dot_prod.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS)

saxpy: saxpy.o $(AUX_OBJS)
	$(CXX) -o $@ $< $(AUX_OBJS) $(LDFLAGS) 

$(OBJS):%.o:%.cc frame.hpp Makefile
	$(CXX) -o $@ -c $< $(CFLAGS)

###################################################
#                   TEST                          #
###################################################
TEST_SET = test_profiler test_toolkits test_trait
TEST_OBJS = $(addsuffix .o, $(TEST_SET))

test: $(TEST_SET)
	@echo "target for $(SYSTEM)"
	for i in $(TEST_SET); do\
		echo "testing $$i"; \
		./$$i;\
	done

$(TEST_SET):%:%.cc $(AUX_OBJS)
	$(CXX) -o $@  $(CFLAGS) $(LDFLAGS) $(AUX_OBJS)  $<


###################################################
#               Miscellaneous                     #
###################################################

.PHONY: clean dist distclean lines all

clean: 
	-rm -f *.o vec_add mat_mul lang_pipe dot_prod saxpy $(TEST_SET)
distclean:
	-rm -f *~ ._*
dist: 
	cd .. && tar -cjf libvina.`date +%y_%m_%d`.tar.bz2 libvina/

lines: 
	find | grep ".\(c\|h\|hpp\|cc\)$$" | xargs wc -l
