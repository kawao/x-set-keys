-include ../make.inc

PROGRAM = x-set-keys
OBJS = main.o x-set-keys.o action.o config.o key-code-array.o \
 key-information.o device.o keyboard-device.o uinput-device.o window-system.o

CC = gcc
CFLAGS = -Wall -g -O2 `pkg-config --cflags glib-2.0`
LDFLAGS = -lX11 `pkg-config --libs glib-2.0`

.SUFFIXES: .c .o

.PHONY: all
all: depend $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS) depend.inc

.PHONY: depend
depend: $(OBJS:.o=.c)
	-@ $(RM) depend.inc
	-@ for i in $^; do cpp -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.c//g" >> depend.inc; done

-include depend.inc

.PHONY: install
install: all
	install -d $(DINSTALLBIN)
	install -m 755 $(PROGRAM) $(DINSTALLBIN)/

.PHONY: uninstall
uninstall:
	rm -f $(DINSTALLBIN)/$(PROGRAM)
