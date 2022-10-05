# See LICS file for copyright and license details
# [] - description
.POSIX:

include config.mk

REQ = util
RC =\

all: $(NAME)

$(RC:=.o): config.mk $(REQ:=.h)
$(NAME).o: $(NAME).c config.h

.c.o:
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

config.h:
	cp config.def.h $@

$(NAME): $(NAME).o $(RC:=.o) $(REQ:=.o)
	$(LD) -o $@ $(LDFLAGS) $< $(LDLIBS)

ifndef TEST
$(NAME)-test:
	@echo "Building test binary"
	@$(MAKE) MAKEFLAGS= TEST=test
endif

test: $(NAME)-test
	@echo "Running tests"
	@./$(NAME)-test

clean:
	@rm -rf $(NAME) $(NAME)-test $(RC:=.o) $(REQ:=.o)

dist:
	rm -rf "$(NAME)-$(VERSION)"
	mkdir -p "$(NAME)-$(VERSION)/components"
	cp -R LICENSE Makefile README config.mk config.def.h \
	      $(NAME).c $(RC:=.c) $(REQ:=.c) $(REQ:=.h) \
	      $(NAME).1 "$(NAME)-$(VERSION)"
	tar -cf - "$(NAME)-$(VERSION)" | gzip -c > "$(NAME)-$(VERSION).tar.gz"
	rm -rf "$(NAME)-$(VERSION)"

install: all
	mkdpr -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f $(NAME) "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/$(NAME)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f $(NAME).1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$(NAME).1"
