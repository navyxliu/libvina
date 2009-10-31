all: lib
lib: oclHelper.c 
	gcc -c $< -I../inc
	ar -ru ../lib/liboclUtil.a oclHelper.o
