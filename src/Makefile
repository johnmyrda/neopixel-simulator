target=	led_sim
simavr = ../lib/simavr
sds = ../lib/sds/

src = $(wildcard *.c)
obj = $(src:.c=.o)

OBJ := obj-${shell $(CC) -dumpmachine}

libsimavr = ${simavr}/simavr/${OBJ}/
libsimavrparts = ${simavr}/examples/parts/${OBJ}/

CFLAGS+=-I${simavr}/simavr/sim/ -I${simavr}/examples/parts/ -I$(sds) -Wall -Werror -O2

debug: CFLAGS += -DDEBUG -ggdb3
debug: led_sim

#LDFLAGS = "-Wl,-z,origin" "-Wl,-rpath,\$$ORIGIN/${libsimavr}" "-Wl,-rpath,\$$ORIGIN/${libsimavrparts}" -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lglut -lutil -lGL -lelf
LDFLAGS = -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lutil -lelf

include ${simavr}/examples/Makefile.opengl

sds.o:
	$(CC) $(sds)/sds.c -c

led_sim: $(obj) sds.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean: 
	rm -f ${target} *.o
