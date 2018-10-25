CFLAGS=-O2 -Werror -Wall

all: simulator

simulator: simavr
	$(MAKE) -C src

simavr:
	$(MAKE) -C lib/simavr
