# See LICENSE file for copyright and license details
.POSIX:

include config.mk

all: $(NAME)
.PHONY: test debug clean dist install all

SEDREP := sed -e 's/^\(.*\)\.o:/\1.d \1.o:/'

SRC = src/util.c src/btree.c src/protocol.c src/canopen.c
ifeq ($(TEST),)
	SRC += src/drunkcan.c
else
	SRC += test/test.c
endif
OBJ = ${SRC:.c=.o}

include $(OBJ:.o=.d)

# Make the dependencies and do not print the recipe, only the result
%.d: %.c
	@$(CC) -MM -MG $(CFLAGS) $*.c | sed -e "s/^$(@F)/$(@D)\//" | $(SEDREP) > $@
	@awk NF=NF RS= OFS=' ' $@ | awk -F '\\' '{print $$1, $$2}' | sed 's/  */ /g'

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf */*.d */*.o $(NAME) $(NAME)-debug $(NAME)-test

$(NAME): $(OBJ)
	$(LD) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

debug: clean
	@DEBUG=1 $(MAKE)

test: clean
	@TEST=1 $(MAKE)

testsuite: test
	./$(NAME)-test

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f $(NAME) "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/$(NAME)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f $(NAME).1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$(NAME).1"
