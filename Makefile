# Metadata
METAPATH=$(abspath ./metadata.make)
-include $(METAPATH)
APPDATE=$(shell date -d "$$(stat --printf "%y" $(METAPATH))" "+%d %B %Y")

# Paths
PREFIX=/usr/local
SRCDIR=$(abspath ./src)
INCDIR=$(abspath ./include)
DOCDIR=$(abspath ./docs)
DISTDIR=$(abspath ./dist)
BINPATH=$(DISTDIR)/bin/$(APPNAME)

# Source files
SRCS=$(shell find $(SRCDIR) -name '*.c')
HEADERS=$(shell find $(INCDIR) -name '*.h')

# Derived filenames
OBJS=$(SRCS:.c=.o)
PREREQUISITES=$(SRCS:.c=.d)

# Compiler options
CC=clang
WARNINGFLAGS=-Wall -Wshadow -Wwrite-strings -Wmissing-prototypes -Wimplicit-fallthrough -pedantic -fstack-protector-all -fno-strict-aliasing
DEFINEFLAGS=-DAPPNAME=\"$(APPNAME)\" -DAPPVERSION=\"$(APPVERSION)\"
INCLUDEFLAGS=$(shell pkg-config --cflags libfido2 libcbor libsodium) -iquote $(INCDIR)
LDLIBS=$(shell pkg-config --libs libfido2 libcbor libsodium)

# Derived compiler options
CFLAGS=$(INCLUDEFLAGS) $(DEFINEFLAGS) $(WARNINGFLAGS)
LDFLAGS=$(WARNINGFLAGS) $(DEFINEFLAGS)

# m4 preprocessor options
M4FLAGS=-Dm4_APPNAME="$(APPNAME)" -Dm4_APPVERSION="$(APPVERSION)" -Dm4_APPDATE="$(APPDATE)" --prefix-builtins

# Release build targets
.PHONY: all
all: release docs

.PHONY: release
release: CFLAGS+=-O3
release: LDFLAGS+=-O3 -s
release: $(BINPATH) docs bash-completion

.PHONY: installdirs
installdirs:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	mkdir -p $(DESTDIR)$(PREFIX)/share/bash-completion/completions

.PHONY: install
install: release docs installdirs
	install -g 0 -o 0 -p -m 4755 $(DISTDIR)/bin/$(APPNAME) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)
	install -g 0 -o 0 -p -m 0644 $(DISTDIR)/share/man/man1/$(APPNAME).1.gz $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME).1.gz
	install -g 0 -o 0 -p -m 0644 $(DISTDIR)/share/bash-completion/completions/$(APPNAME) $(DESTDIR)$(PREFIX)/share/bash-completion/completions/$(APPNAME)

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/share/bash-completion/completions/$(APPNAME)
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME).1.gz
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)

.PHONY: clean
clean: cleandep cleanobj cleandist

# Development build targets
.PHONY: debug
debug: CFLAGS+=-g -DDEBUG
debug: LDFLAGS+=-g -DDEBUG
debug: $(BINPATH)

.PHONY: lint
lint:
	clang-tidy --fix $(SRCS) -- $(INCLUDEFLAGS) $(DEFINEFLAGS)

.PHONY: format
format:
	clang-format -style=file -i $(SRCS) $(HEADERS)

# Invidiual source file targets
$(BINPATH): $(OBJS)
	mkdir -p $(DISTDIR)/bin
	$(CC) -o $(BINPATH) $(OBJS) $(LDFLAGS) $(LDLIBS)

-include $(PREREQUISITES)

$(INCDIR)/help.h: $(METAPATH)

%.d: %.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

# Ancillary files targets
.PHONY: docs
docs: manpage bash-completion

.PHONY: manpage
manpage: $(DISTDIR)/share/man/man1/$(APPNAME).1.gz

$(DISTDIR)/share/man/man1/$(APPNAME).1.gz: $(DISTDIR)/share/man/man1/$(APPNAME).1
	gzip -f $<

.INTERMEDIATE: $(DISTDIR)/share/man/man1/$(APPNAME).1
$(DISTDIR)/share/man/man1/$(APPNAME).1: $(DOCDIR)/manpage.m4 $(METAPATH)
	mkdir -p $(DISTDIR)/share/man/man1
	m4 $(M4FLAGS) $(DOCDIR)/manpage.m4 > $@

.PHONY: bash-completion
bash-completion: $(DISTDIR)/share/bash-completion/completions/$(APPNAME)

$(DISTDIR)/share/bash-completion/completions/$(APPNAME): $(DOCDIR)/bash-completion.m4 $(METAPATH)
	mkdir -p $(DISTDIR)/share/bash-completion/completions
	m4 $(M4FLAGS) $(DOCDIR)/bash-completion.m4 > $@

# Cleanup targets
.PHONY: cleandist
cleandist:
	$(RM) -rf $(DISTDIR)

.PHONY: cleandep
cleandep:
	$(RM) $(PREREQUISITES)

.PHONY: cleanobj
cleanobj:
	$(RM) $(OBJS)
