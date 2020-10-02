# khefin Makefile
#
# Comments beginning with #: and immediately preceding a target are printed by the `help` target.


################################################################################
# DEFINITIONS                                                                  #
################################################################################

# Metadata
METAPATH=$(abspath ./metadata.make)
-include $(METAPATH)
APPDATE=$(shell date -d "$$(stat --printf "%y" $(METAPATH))" "+%d %B %Y")
LONGEST_VALID_PASSPHRASE=1024
WARN_ON_MEMORY_LOCK_ERRORS=1
SETCAP_BINARY=1

# Paths
ifeq ($(origin PREFIX),undefined)
PREFIX=/usr/local
endif
SRCDIR=$(abspath ./src)
INCDIR=$(abspath ./include)
MANDIR=$(abspath ./man)
SCRIPTDIR=$(abspath ./scripts)
DISTDIR=$(abspath ./dist)
BINPATH=$(DISTDIR)/bin/$(APPNAME)
M4VARSPATH=$(abspath ./variables.m4)

# Source files
SRCS=$(shell find $(SRCDIR) -name '*.c')
HEADERS=$(shell find $(INCDIR) -name '*.h')

# Derived filenames
OBJS=$(SRCS:.c=.o)
PREREQUISITES=$(SRCS:.c=.d)

# Compiler options
ifeq ($(origin CC),default)
CC=clang
endif
WARNINGFLAGS=-Wall \
    -Wshadow \
	-Wwrite-strings \
	-Wmissing-prototypes \
	-Wimplicit-fallthrough \
	-pedantic \
	-fstack-protector-all \
	-fno-strict-aliasing
DEFINEFLAGS=-DAPPNAME=\"$(APPNAME)\" \
    -DAPPVERSION=\"$(APPVERSION)\" \
	-DLONGEST_VALID_PASSPHRASE=$(LONGEST_VALID_PASSPHRASE) \
	-DWARN_ON_MEMORY_LOCK_ERRORS=$(WARN_ON_MEMORY_LOCK_ERRORS)
INCLUDEFLAGS=$(shell pkg-config --cflags libfido2 libcbor libsodium) -iquote $(INCDIR)
LDLIBS=$(shell pkg-config --libs libfido2 libcbor libsodium)

# Derived compiler options
CFLAGS:=$(INCLUDEFLAGS) $(DEFINEFLAGS) $(WARNINGFLAGS) $(CFLAGS)
LDFLAGS:=$(WARNINGFLAGS) $(DEFINEFLAGS) $(LDFLAGS)

# m4 preprocessor options
M4FLAGS=-Dm4_APPNAME="$(APPNAME)" \
    -Dm4_APPVERSION="$(APPVERSION)" \
	-Dm4_APPDATE="$(APPDATE)" \
	-Dm4_LONGEST_VALID_PASSPHRASE=$(LONGEST_VALID_PASSPHRASE) \
	-Dm4_WARN_ON_MEMORY_LOCK_ERRORS=$(WARN_ON_MEMORY_LOCK_ERRORS) \
	-Dm4_INSTALL_PREFIX=$(PREFIX) \
	--prefix-builtins \
	$(M4VARSPATH)


################################################################################
# COMMON TARGETS                                                               #
################################################################################

.PHONY: help
#: Print this list of targets and their descriptions
help:
	@grep -B1 -E "^[a-zA-Z0-9_-]+\:([^\=]|$$)" Makefile \
	 | grep -v -- -- \
	 | sed 'N;s/\n/###/' \
	 | sed -n 's/^#: \(.*\)###\([^:]*\):.*/\2###\1/p' \
	 | column -t  -s '###'


check_dep_pkgconfig = @printf "%-20s %-10s " "$(1)" "$(2)"; if pkg-config $(1) > /dev/null 2>&1; then printf "OK\n"; else printf "not found!\n"; fi
check_dep_command = @printf "%-20s %-10s " "$(1)" "$(2)"; if command -v $(3) > /dev/null; then printf "OK\n"; else printf "not found!\n"; fi

.PHONY: check
#: Check for the presence of dependencies
check:
	$(call check_dep_pkgconfig,libfido2,required)
	$(call check_dep_pkgconfig,libcbor,required)
	$(call check_dep_pkgconfig,libsodium,required)
	$(call check_dep_command,bash,optional,bash)
	$(call check_dep_command,ssh-agent,optional,ssh-agent)
	$(call check_dep_command,mkinitcpio,optional,mkinitcpio)
	$(call check_dep_command,initramfs-tools,optional,mkinitramfs)

# Release build targets
.PHONY: all
#: Alias for release manpages bash-completion ssh-askpass
all:         release manpages bash-completion ssh-askpass

.PHONY: release
#: Build an optimized and stripped binary
release: CFLAGS:=-O3 $(CFLAGS)
release: LDFLAGS:=-O3 -s $(LDFLAGS)
release: $(BINPATH)

.PHONY: install
#: Install built files to $DESTDIR
install: release manpages
	install -g 0 -o 0 -p -m 0755 -D $(DISTDIR)/bin/$(APPNAME) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)
	if [ "$(SETCAP_BINARY)" -ne 0 ]; then setcap cap_ipc_lock+ep $(DESTDIR)$(PREFIX)/bin/$(APPNAME); fi
	install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/share/man/man1/$(APPNAME).1.gz $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME).1.gz
	if [ -f $(DISTDIR)/share/bash-completion/completions/$(APPNAME) ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/share/bash-completion/completions/$(APPNAME) $(DESTDIR)$(PREFIX)/share/bash-completion/completions/$(APPNAME); fi
	if [ -f $(DISTDIR)/bin/$(APPNAME)-add-luks-key ]; then install -g 0 -o 0 -p -m 0755 -D $(DISTDIR)/bin/$(APPNAME)-add-luks-key $(DESTDIR)$(PREFIX)/bin/$(APPNAME)-add-luks-key; fi
	if [ -f $(DISTDIR)/bin/$(APPNAME)-add-luks-key ] && [ -f $(DISTDIR)/share/man/man8/$(APPNAME)-add-luks-key.8.gz ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/share/man/man8/$(APPNAME)-add-luks-key.8.gz $(DESTDIR)$(PREFIX)/share/man/man8/$(APPNAME)-add-luks-key.8.gz; fi
	if [ -f $(DISTDIR)/bin/$(APPNAME)-ssh-askpass ]; then install -g 0 -o 0 -p -m 0755 -D $(DISTDIR)/bin/$(APPNAME)-ssh-askpass $(DESTDIR)$(PREFIX)/bin/$(APPNAME)-ssh-askpass; fi
	if [ -f $(DISTDIR)/bin/$(APPNAME)-ssh-askpass ] && [ -f $(DISTDIR)/share/man/man1/$(APPNAME)-ssh-askpass.1.gz ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/share/man/man1/$(APPNAME)-ssh-askpass.1.gz $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME)-ssh-askpass.1.gz; fi
	if [ -f $(DISTDIR)/lib/initcpio/install/$(APPNAME) ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/lib/initcpio/install/$(APPNAME) $(DESTDIR)$(PREFIX)/lib/initcpio/install/$(APPNAME); fi
	if [ -f $(DISTDIR)/lib/initcpio/hooks/$(APPNAME) ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/lib/initcpio/hooks/$(APPNAME) $(DESTDIR)$(PREFIX)/lib/initcpio/hooks/$(APPNAME); fi
	if [ -f $(DISTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME) ]; then install -g 0 -o 0 -p -m 0755 -D $(DISTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME) $(DESTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME); fi
	if [ -f $(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript ]; then install -g 0 -o 0 -p -m 0755 -D $(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript $(DESTDIR)$(PREFIX)/lib/$(APPNAME)/cryptsetup-keyscript; fi
	if [ -f $(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript ] && [ -f $(DISTDIR)/share/man/man8/$(APPNAME)-cryptsetup-keyscript.8.gz ]; then install -g 0 -o 0 -p -m 0644 -D $(DISTDIR)/share/man/man8/$(APPNAME)-cryptsetup-keyscript.8.gz $(DESTDIR)$(PREFIX)/share/man/man8/$(APPNAME)-cryptsetup-keyscript.8.gz; fi

.PHONY: uninstall
#: Remove files from $DESTDIR
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man8/$(APPNAME)-cryptsetup-keyscript.8.gz
	$(RM) $(DESTDIR)$(PREFIX)/lib/$(APPNAME)/cryptsetup-keyscript
	$(RM) $(DESTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME)
	$(RM) $(DESTDIR)$(PREFIX)/lib/initcpio/hooks/$(APPNAME)
	$(RM) $(DESTDIR)$(PREFIX)/lib/initcpio/install/$(APPNAME)
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME)-ssh-askpass.1.gz
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)-ssh-askpass
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man8/$(APPNAME)-add-luks-key.8.gz
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)-add-luks-key
	$(RM) $(DESTDIR)$(PREFIX)/share/bash-completion/completions/$(APPNAME)
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/$(APPNAME).1.gz
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(APPNAME)

.PHONY: clean
#: Delete built files
clean: cleandep cleanobj cleandist

.PHONY: debug
#: Build an unoptimized binary with debug symbols
debug: CFLAGS:=-fsanitize=address -fno-omit-frame-pointer -g -DDEBUG $(CFLAGS)
debug: LDFLAGS:=-fsanitize=address -fno-omit-frame-pointer -g -DDEBUG $(LDFLAGS)
debug: $(BINPATH)


################################################################################
# MAN PAGES                                                                    #
################################################################################

.PHONY: manpages
#: Build man pages
manpages: $(DISTDIR)/share/man/man1/$(APPNAME).1.gz $(DISTDIR)/share/man/man1/$(APPNAME)-ssh-askpass.1.gz $(DISTDIR)/share/man/man8/$(APPNAME)-add-luks-key.8.gz $(DISTDIR)/share/man/man8/$(APPNAME)-cryptsetup-keyscript.8.gz

$(DISTDIR)/share/man/man1/%.1.gz: $(DISTDIR)/share/man/man1/%.1
	gzip -f $<

$(DISTDIR)/share/man/man8/%.8.gz: $(DISTDIR)/share/man/man8/%.8
	gzip -f $<

.INTERMEDIATE: $(DISTDIR)/share/man/man1/%.1
$(DISTDIR)/share/man/man1/%.1: $(MANDIR)/1/%.m4 $(METAPATH) $(M4VARSPATH)
	mkdir -p $(DISTDIR)/share/man/man1
	m4 $(M4FLAGS) $< > $@

.INTERMEDIATE: $(DISTDIR)/share/man/man8/%.8
$(DISTDIR)/share/man/man8/%.8: $(MANDIR)/8/%.m4 $(METAPATH) $(M4VARSPATH)
	mkdir -p $(DISTDIR)/share/man/man8
	m4 $(M4FLAGS) $< > $@


################################################################################
# COMPLETION SCRIPTS                                                           #
################################################################################

.PHONY: bash-completion
#: Build bash completion scripts
bash-completion: $(DISTDIR)/share/bash-completion/completions/$(APPNAME)

shellcheck: $(DISTDIR)/share/bash-completion/completions/$(APPNAME)

$(DISTDIR)/share/bash-completion/completions/$(APPNAME): $(SCRIPTDIR)/bash-completion.m4 $(METAPATH) $(M4VARSPATH)
	mkdir -p $(DISTDIR)/share/bash-completion/completions
	m4 $(M4FLAGS) $(SCRIPTDIR)/bash-completion.m4 > $@


################################################################################
# DISK ENCRYPTION                                                              #
################################################################################

.PHONY: add-luks-key
add-luks-key: $(DISTDIR)/bin/$(APPNAME)-add-luks-key

shellcheck: $(DISTDIR)/bin/$(APPNAME)-add-luks-key
$(DISTDIR)/bin/$(APPNAME)-add-luks-key: $(SCRIPTDIR)/add-luks-key.m4 $(M4VARSPATH)
	mkdir -p $(DISTDIR)/bin
	m4 $(M4FLAGS) $< > $@


.PHONY: mkinitcpio
#: Build disk encryption scripts for use with mkinitcpio
mkinitcpio: add-luks-key $(DISTDIR)/lib/initcpio/install/$(APPNAME) $(DISTDIR)/lib/initcpio/hooks/$(APPNAME)

shellcheck: $(DISTDIR)/lib/initcpio/install/$(APPNAME) $(DISTDIR)/lib/initcpio/hooks/$(APPNAME)

$(DISTDIR)/lib/initcpio/install/$(APPNAME): $(SCRIPTDIR)/mkinitcpio/install.m4 $(M4VARSPATH)
	mkdir -p $(DISTDIR)/lib/initcpio/install
	m4 $(M4FLAGS) $< > $@

$(DISTDIR)/lib/initcpio/hooks/$(APPNAME): $(SCRIPTDIR)/mkinitcpio/run.m4 $(M4VARSPATH)
	mkdir -p $(DISTDIR)/lib/initcpio/hooks
	m4 $(M4FLAGS) $< > $@


.PHONY: initramfs-tools
#: Build disk encryption scripts for use with initramfs-tools
initramfs-tools: add-luks-key $(DISTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME) $(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript

shellcheck: $(DISTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME) $(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript

$(DISTDIR)/etc/initramfs-tools/hooks/crypt$(APPNAME): $(SCRIPTDIR)/initramfs-tools/hook.m4 $(M4VARSPATH)
	mkdir -p $(DISTDIR)/etc/initramfs-tools/hooks/
	m4 $(M4FLAGS) $< > $@

$(DISTDIR)/lib/$(APPNAME)/cryptsetup-keyscript: $(SCRIPTDIR)/initramfs-tools/keyscript.m4 $(M4VARSPATH)
	mkdir -p $(DISTDIR)/lib/$(APPNAME)
	m4 $(M4FLAGS) $< > $@



################################################################################
# SSH-ASKPASS                                                                  #
################################################################################

.PHONY: ssh-askpass
#: Build SSH askpass script
ssh-askpass: $(DISTDIR)/bin/$(APPNAME)-ssh-askpass

shellcheck: $(DISTDIR)/bin/$(APPNAME)-ssh-askpass

$(DISTDIR)/bin/$(APPNAME)-ssh-askpass: $(SCRIPTDIR)/ssh-askpass.m4 $(METAPATH) $(M4VARSPATH)
	mkdir -p $(DISTDIR)/bin
	m4 $(M4FLAGS) $(SCRIPTDIR)/ssh-askpass.m4 > $@


################################################################################
# SOURCE CODE CHECKS                                                           #
################################################################################

.PHONY: lint
#: Lint and fix source code with clang-tidy
lint:
	clang-tidy --fix $(SRCS) -- $(INCLUDEFLAGS) $(DEFINEFLAGS)

.PHONY: check-lint
check-lint:
	clang-tidy $(SRCS) -- $(INCLUDEFLAGS) $(DEFINEFLAGS)

.PHONY: shellcheck
#: Check scripts with shellcheck
shellcheck:
	shellcheck $^

.PHONY: format
#: Format source code with clang-format
format:
	clang-format -style=file -i $(SRCS) $(HEADERS)

.PHONY: check-format
check-format:
	clang-format -style=file -Werror --dry-run $(SRCS) $(HEADERS)


################################################################################
# INDIVIDUAL SOURCE FILES                                                      #
################################################################################

$(BINPATH): $(OBJS)
	mkdir -p $(DISTDIR)/bin
	$(CC) -o $(BINPATH) $(OBJS) $(LDFLAGS) $(LDLIBS)

-include $(PREREQUISITES)

$(INCDIR)/help.h: $(METAPATH)

%.d: %.c
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@


################################################################################
# CLEANUP                                                                      #
################################################################################

.PHONY: cleandist
cleandist:
	$(RM) -rf $(DISTDIR)

.PHONY: cleandep
cleandep:
	$(RM) $(PREREQUISITES)

.PHONY: cleanobj
cleanobj:
	$(RM) $(OBJS)
