target=	led_sim
simavr = ../lib/simavr

src = $(wildcard *.c)
obj = $(src:.c=.o)

OBJ 		:= obj-${shell $(CC) -dumpmachine}

libsimavr = ${simavr}/simavr/${OBJ}/
libsimavrparts = ${simavr}/examples/parts/${OBJ}/

CFLAGS+=-I../lib/simavr/simavr/sim/ -I ../lib/simavr/examples/parts/ -Wall -Werror -O2

debug: CFLAGS += -DDEBUG -ggdb3
debug: led_sim

#LDFLAGS = "-Wl,-z,origin" "-Wl,-rpath,\$$ORIGIN/${libsimavr}" "-Wl,-rpath,\$$ORIGIN/${libsimavrparts}" -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lglut -lutil -lGL -lelf
LDFLAGS = -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lutil -lelf

include ${simavr}/examples/Makefile.opengl

led_sim: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

clean: 
	rm ${target} *.o
