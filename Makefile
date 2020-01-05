# Metadata
METAPATH=$(abspath ./metadata.make)
-include $(METAPATH)
APPDATE=$(shell date -d "$$(stat --printf "%y" $(METAPATH))" "+%d %B %Y")
LONGEST_VALID_PASSPHRASE=1024
WARN_ON_MEMORY_LOCK_ERRORS=1
SETCAP_BINARY=1

# Paths
PREFIX=/usr/local
SRCDIR=$(abspath ./src)
INCDIR=$(abspath ./include)
DOCDIR=$(abspath ./docs)
SCRIPTDIR=$(abspath ./scripts)
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
DEFINEFLAGS=-DAPPNAME=\"$(APPNAME)\" -DAPPVERSION=\"$(APPVERSION)\" -DLONGEST_VALID_PASSPHRASE=$(LONGEST_VALID_PASSPHRASE) -DWARN_ON_MEMORY_LOCK_ERRORS=$(WARN_ON_MEMORY_LOCK_ERRORS)
INCLUDEFLAGS=$(shell pkg-config --cflags libfido2 libcbor libsodium) -iquote $(INCDIR)
LDLIBS=$(shell pkg-config --libs libfido2 libcbor libsodium)

# Derived compiler options
CFLAGS=$(INCLUDEFLAGS) $(DEFINEFLAGS) $(WARNINGFLAGS)
LDFLAGS=$(WARNINGFLAGS) $(DEFINEFLAGS)

# m4 preprocessor options
M4FLAGS=-Dm4_APPNAME="$(APPNAME)" -Dm4_APPVERSION="$(APPVERSION)" -Dm4_APPDATE="$(APPDATE)" -Dm4_LONGEST_VALID_PASSPHRASE=$(LONGEST_VALID_PASSPHRASE) -Dm4_WARN_ON_MEMORY_LOCK_ERRORS=$(WARN_ON_MEMORY_LOCK_ERRORS) --prefix-builtins

# Release build targets
.PHONY: release
release: CFLAGS+=-O3
release: LDFLAGS+=-O3 -s
release: $(BINPATH) manpage

.PHONY: installdirs
installdirs:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	[ -f $(DISTDIR)/share/bash-completion/completions/$(APPNAME) ] && mkdir -p $(DESTDIR)$(PREFIX)/share/bash-completion/completions || true
	[ -f $(DISTDIR)/etc/initcpio/install/$(APPNAME) ] && mkdir -p $(DESTDIR)/etc/initcpio/install/ || true
	[ -f $(DISTDIR)/etc/initcpio/hooks/$(APPNAME) ] && mkdir -p $(DESTDIR)/etc/initcpio/hooks/ || true

.PHONY: install
install: release installdirs
	install -g 0 -o 0 -p -m 0755 $(DISTDIR)/bin/$(APPNAME) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)
	[ "$(SETCAP_BINARY)" -eq 0 ] || setcap cap_ipc_lock+ep $(DESTDIR)$(PREFIX)/bin/$(APPNAME)
	install -g 0 -o 0 -p -m 0644 $(DISTDIR)/share/man/man1/$(APPNAME).1.gz $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME).1.gz
	[ -f $(DISTDIR)/share/bash-completion/completions/$(APPNAME) ] && install -g 0 -o 0 -p -m 0644 $(DISTDIR)/share/bash-completion/completions/$(APPNAME) $(DESTDIR)$(PREFIX)/share/bash-completion/completions/$(APPNAME) || true
	[ -f $(DISTDIR)/etc/initcpio/install/$(APPNAME) ] && install -g 0 -o 0 -p -m 0644 $(DISTDIR)/etc/initcpio/install/$(APPNAME) $(DESTDIR)/etc/initcpio/install/$(APPNAME) || true
	[ -f $(DISTDIR)/etc/initcpio/hooks/$(APPNAME) ] && install -g 0 -o 0 -p -m 0644 $(DISTDIR)/etc/initcpio/hooks/$(APPNAME) $(DESTDIR)/etc/initcpio/hooks/$(APPNAME) || true

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)/etc/initcpio/hooks/$(APPNAME)
	$(RM) $(DESTDIR)/etc/initcpio/install/$(APPNAME)
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

$(DISTDIR)/share/bash-completion/completions/$(APPNAME): $(SCRIPTDIR)/bash-completion.m4 $(METAPATH)
	mkdir -p $(DISTDIR)/share/bash-completion/completions
	m4 $(M4FLAGS) $(SCRIPTDIR)/bash-completion.m4 > $@


.PHONY: initcpio
initcpio: $(DISTDIR)/etc/initcpio/install/$(APPNAME) $(DISTDIR)/etc/initcpio/hooks/$(APPNAME)

INITCPIO_M4FLAGS=-Dm4_DEFAULT_MAX_PASSPHRASE_ATTEMPTS=3 -Dm4_DEFAULT_ENCRYPTED_KEYFILE_DIR="/keyfiles" -Dm4_DEFAULT_KEYFILES_SOURCE_DIR="/boot/keyfiles"

$(DISTDIR)/etc/initcpio/install/$(APPNAME): $(SCRIPTDIR)/initcpio-install.m4
	mkdir -p $(DISTDIR)/etc/initcpio/install
	m4 $(M4FLAGS) $(INITCPIO_M4FLAGS) $(SCRIPTDIR)/initcpio-install.m4 > $@

$(DISTDIR)/etc/initcpio/hooks/$(APPNAME): $(SCRIPTDIR)/initcpio-run.m4
	mkdir -p $(DISTDIR)/etc/initcpio/hooks
	m4 $(M4FLAGS) $(INITCPIO_M4FLAGS) $(SCRIPTDIR)/initcpio-run.m4 > $@

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
