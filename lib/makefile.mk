# Library files
LIBS_TARGET = \
	stdlib

LIBS := $(addprefix lib/, $(LIBS_TARGET))
LIBS := $(addsuffix .o, $(LIBS))

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

libs: $(LIBS)

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -c -o $@ $<

lib-clean:
	rm -f $(LIBS)	
