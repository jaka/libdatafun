CC	?= cc
STRIP	?= strip

OPTS	:= -O2
CFLAGS	+= -Isrc -Wall -Wextra -ffunction-sections -fdata-sections -fno-strict-aliasing
# -s -fdiagnostics-color=always
LDFLAGS	+= -Wl,--gc-sections
SFLAGS	:= -s -R .comment -R .gnu.version -R .note -R .note.ABI-tag

SOURCES	:= $(wildcard src/*.c)
OBJECTS	:= $(patsubst %.c,%.o,$(SOURCES))
TARGETS := upload

all: $(OBJECTS) $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(OPTS) -c -o $@ $<

$(TARGETS): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OPTS) -o $@ $@.c $^
#	$(STRIP) $(SFLAGS) $@

clean:
ifneq (,$(OBJECTS))
	rm -f $(OBJECTS)
endif
ifneq (,$(TARGETS))
	rm -f $(TARGETS)
endif
