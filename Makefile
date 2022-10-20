# See LICENSE file for copyright and license details
.POSIX:

include config.mk

all: $(NAME)

SRC = util.c btree.c protocol.c canopen.c
ifeq ($(TEST),)
	SRC += drunkcan.c
else
	SRC += test/test.c
endif
OBJ = ${SRC:.c=.o}

%.o: %.c
	$(CC) -c $(CFLAGS) $<

test/%.o: test/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(NAME): $(OBJ)
	$(LD) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

debug:
	DEBUG=1 $(MAKE)

ifeq ($(TEST),)
test:
	TEST=1 $(MAKE)
else
test: $(NAME)
	./$(NAME)
endif

ifeq ($(TEST)$(DEBUG),)
.PHONY: test debug clean dist install
clean:
	@rm -rf $(NAME) $(NAME)-test $(OBJ)

dist:
	rm -rf "$(NAME)-$(VERSION)"
	mkdir -p "$(NAME)-$(VERSION)/components"
	cp -R LICENSE Makefile README config.mk config.def.h \
	      $(NAME).c $(RC:=.c) $(REQ:=.c) $(REQ:=.h) \
	      $(NAME).1 "$(NAME)-$(VERSION)"
	tar -cf - "$(NAME)-$(VERSION)" | gzip -c > "$(NAME)-$(VERSION).tar.gz"
	rm -rf "$(NAME)-$(VERSION)"

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f $(NAME) "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/$(NAME)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f $(NAME).1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$(NAME).1"
endif
