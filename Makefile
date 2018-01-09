CC = g++
OPT = -O3
OPT = -g
#WARN = -Wall
#ERR = -Werror

CFLAGS = $(OPT) $(WARN) $(ERR) $(INC) $(LIB)

SIM_SRC = main.cc cache.cc

SIM_OBJ = main.o cache.o

all: smp_cache
	@echo "Compilation Done ---> nothing else to make :) "

smp_cache: $(SIM_OBJ)
	$(CC) -o smp_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "----------------------------------------------------------"
	@echo "-----------FALL11-506 SMP SIMULATOR (SMP_CACHE)-----------"
	@echo "----------------------------------------------------------"
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

clean:
	rm -f *.o *.out smp_cache

zip:
	zip karthi2.zip Makefile main.cc cache.cc cache.h MSI*.txt MESI*.txt MOESI*.txt *.out
clobber:
	rm -f *.o


