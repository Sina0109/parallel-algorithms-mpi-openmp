CC = mpicc
CFLAGS = -O2 -g -Wall -Wno-unused-result -std=c99 -I /usr/X11R6/include
LDLIBS = -L /usr/X11R6/lib -lX11
RM = rm -f

ALL = chemin_lab gen_lab gen_lab-parallel

all: $(ALL)

$(ALL): graph.o

clean:
	$(RM) $(ALL) *.o *.lab core vgcore.[0-9]*
