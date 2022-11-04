# See LICENSE file for copyright and license details
.POSIX:

include config.mk

all: $(NAME)
.PHONY: test debug clean dist install all softclean

SEDREP := sed -e 's/^\(.*\)\.o:/\1.d \1.o:/'

SRC = src/util.c src/protocol.c src/canopen.c src/drunkcan.c src/workqueue.c
OBJ = ${SRC:.c=.o}

include test.mk
ifeq ($(COVERAGE),1)
CFLAGS+=-DUNIT_TESTING -fprofile-arcs -ftest-coverage
LDLIBS+=--coverage
endif

$(TEST): CFLAGS+=-g -DUNIT_TESTING
$(TEST): % : %.o
$(TEST): LDLIBS+=-lcmocka


include $(OBJ:.o=.d)
include src/main.d
include $(TESTOBJ:.o=.d)
$(NAME): src/main.o $(OBJ)

# Make the dependencies and do not print the recipe, only the result
%.d: %.c
	@$(CC) -MM -MG $(CFLAGS) $*.c | sed -e "s/^$(@F)/$(@D)\//" | $(SEDREP) > $@
	@awk NF=NF RS= OFS=' ' $@ | awk -F '\\' '{print $$1, $$2}' | sed 's/  */ /g'

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean: softclean
	rm -rf */*.d */*.gcda */*.gcno

softclean:
	rm -rf */*.o $(NAME) $(NAME)-debug $(TEST)

$(NAME) $(TEST):
	$(LD) -o $@ $^ $(LDFLAGS) $(LDLIBS)

debug: softclean
	@DEBUG=1 $(MAKE)

test: $(TEST)
	-$(patsubst %,./%;,$^)

testsuite: $(TEST)
	-$(patsubst %,$(CHECKTOOL) $(CHECKFLAGS) ./%;,$^)

coverage: clean
	@COVERAGE=1 $(MAKE) test

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f $(NAME) "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/$(NAME)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f $(NAME).1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$(NAME).1"
