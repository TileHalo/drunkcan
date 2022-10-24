# See LICENSE file for copyright and license details
.POSIX:

include config.mk
include test.mk

all: $(NAME)
.PHONY: test debug clean dist install all softclean

SEDREP := sed -e 's/^\(.*\)\.o:/\1.d \1.o:/'

SRC = src/util.c src/btree.c src/protocol.c src/canopen.c src/drunkcan.c \
      src/intarray.c
OBJ = ${SRC:.c=.o}
TEST = test/btree test/intarray

$(TEST): CFLAGS+=-g
$(TEST): % : %.o
$(TEST): LDLIBS+=-lcmocka
$(TEST): $(OBJ)
test/btree: LDFLAGS+=-Wl,--wrap=remove

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
	rm -rf */*.d

softclean:
	rm -rf */*.o $(NAME) $(NAME)-debug $(TEST)

$(NAME) $(TEST):
	$(LD) -o $@ $^ $(LDFLAGS) $(LDLIBS)

debug: softclean
	@DEBUG=1 $(MAKE)

test: $(TEST)
	$(patsubst %,./%;,$^)

testsuite: $(TEST)
	$(patsubst %,$(CHECKTOOL) $(CHECKFLAGS) ./%;,$^)

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f $(NAME) "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/$(NAME)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f $(NAME).1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$(NAME).1"
