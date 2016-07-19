PLATFORM 	= linux64
INC 		= $(GUROBI_HOME)/include/
CC       	= gcc

CARGS    	= -m64 -g -std=c99
CLIB     	= -L$(GUROBI_HOME)/lib/ -lgurobi65
OTHERLIB	= -lm -ljansson

SOURCES		= main.c \
			  json_read.c \
		  solver.c


OBJECTS		= $(SOURCES:.c=.o)
EXECUTABLES	= tp3s_grb

all: $(SOURCES) $(EXECUTABLES)


$(EXECUTABLES): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(CLIB) $(OTHERLIB)

.c.o:
	$(CC) $(CARGS) -c $< -o $@ -I$(INC)

clean:
	rm -f *.o
