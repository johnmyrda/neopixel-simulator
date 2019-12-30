target=	led_sim

SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

simavr = lib/simavr
sds = lib/sds/
SIMAVR_OBJ := obj-${shell $(CC) -dumpmachine}
libsimavr = ${simavr}/simavr/${SIMAVR_OBJ}/
libsimavrparts = ${simavr}/examples/parts/${SIMAVR_OBJ}/

CFLAGS+=-I${simavr}/simavr/sim/ -I${simavr}/examples/parts/ -I$(sds) -Isrc -Wall -Werror -O2
#LDFLAGS = "-Wl,-z,origin" "-Wl,-rpath,\$$ORIGIN/${libsimavr}" "-Wl,-rpath,\$$ORIGIN/${libsimavrparts}" -L${libsimavr} -lsimavr -L${libsimavrparts} -lsimavrparts -lm -lpthread -lglut -lutil -lGL -lelf
LDFLAGS += -L${libsimavr} -L${libsimavrparts}
LDLIBS += -lsimavr -lsimavrparts -lm -lpthread -lutil -lelf

.PHONY: all debug clean

debug: CFLAGS += -DDEBUG -ggdb3
debug: | simavr $(target) # force order of target execution with |

# this target generates a dummy file so it doesn't get run on every build
simavr:
	$(MAKE) -C $(simavr)
	echo "simavr dummy file" > simavr

# build sds
$(target): $(OBJ) $(OBJ_DIR)/sds.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# build sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

clean-all: clean
	$(RM) simavr
	$(MAKE) -C $(simavr) clean
