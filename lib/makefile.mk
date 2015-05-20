# Library files
LIBS_TARGET = \
	stdlib

LIBS_TEST = \
	string \
	malloc

LIBS_TEST := $(addsuffix -test.o, $(LIBS_TEST))
LIBS_TEST := $(addprefix lib/test/, $(LIBS_TEST))
LIBS := $(addprefix lib/, $(LIBS_TARGET))
LIBS := $(addsuffix .o, $(LIBS))

# Are you debugging?
DEBUG = 1

# Include Linux proxy functions (remove during real build)
LIB_CFLAGS += -I linux

# Include files
LIB_CFLAGS += -I include
# Disable Position Independant Code
LIB_CFLAGS += -fno-pic
# Disable built in functions
LIB_CFLAGS += -fno-builtin
# Disable optimizations that deal with aliases.
LIB_CFLAGS += -fno-strict-aliasing
# Disable stack smashing protection
LIB_CFLAGS += -fno-stack-protector

libs: li_proxy $(LIBS) $(LIBS_TEST)

lib/test/%.o: lib/test/%.c
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -o $@ $< $(LIBS) linux/li_proxy.o

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -c -o $@ $<

lib-clean:
	rm -f $(LIBS) $(LIBS_TEST)
