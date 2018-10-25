target=	led_sim
simavr = ../lib/simavr

src = $(wildcard *.c)
obj = $(src:.c=.o)

libsimavr = ${simavr}/simavr/obj-x86_64-linux-gnu/
libsimavrparts = ${simavr}/examples/parts/obj-x86_64-linux-gnu/

CFLAGS+=-I../lib/simavr/simavr/sim/ -I ../lib/simavr/examples/parts/ -Wall -Werror -O2

debug: CFLAGS += -DDEBUG -ggdb3
debug: led_sim

LDFLAGS = "-Wl,-z,origin" "-Wl,-rpath,\$$ORIGIN/${libsimavr}" "-Wl,-rpath,\$$ORIGIN/${libsimavrparts}" -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lglut -lutil -lGL -lelf

led_sim: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

clean: 
	rm ${target} *.o
