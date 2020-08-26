CC ?= gcc
CFLAGS = -O -Wall -Wno-unused-result
LDFLAGS = -lpthread

OBJS = game.o tui.o
deps := $(OBJS:%.o=.%.o.d)

TARGET = ttt
all: $(TARGET)

# Control the build verbosity
# `make V=1` is equal to `make VERBOSE=1`
ifeq ("$(origin V)", "command line")
    VERBOSE = $(V)
endif
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

%.o: %.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF .$@.d $<

$(TARGET): $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(Q)$(RM) $(OBJS) $(deps) $(TARGET)

-include $(deps)
