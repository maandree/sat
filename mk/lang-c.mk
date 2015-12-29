# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file includes rules for C programs. ===#


# This file is ignored unless _C_STD is defined.
# _C_STD should be set the the version of C that
# is used.
# 
# If you want to compile with -pedantic, define
# the variable _PEDANTIC.
# 
# Define _CPPFLAGS with any additional CPP
# flags to use, _CFLAGS with any additional CC
# flags to use, and _LDFLAGS with any additional
# LD flags to use.
# 
# Defined in path.mk, you can change _ALL_DIRS
# if you do not want CPP definitions for all
# directories.
# 
# Define _HEADER_DIRLEVELS to specify the directory
# nesting level in src. It is assumed that all
# directories contain header files. Set to '0' if
# there are no header files.
# 
# _BIN shall list all commands to build. These
# should be the basenames. For each command
# you should be the variable _OBJ_$(COMMAND)
# that lists all objects files (without the
# suffix and without the aux/ prefix) that
# are required to build the command.
# 
# Binary you want installed to /sbin rather than
# /bin shall be listed in _SBIN rather than in
# _BIN. _SBIN and _BIN may not list binaries
# with identical names. Analogically, you may
# add _LIBEXEC for binary to be installed to
# /libexec.
# 
# If you only have one binary, you may select
# to define _BINDIR to name the variable with
# the pathname for the directory the binary
# shall be installed. For example, if you want
# the binary to be installed to /usr/sbin,
# define _BINDIR = SBINDIR.


ifdef _C_STD



# WHEN TO BUILD, INSTALL, AND UNINSTALL:

cmd: cmd-c
install-cmd: install-cmd-c
uninstall: uninstall-cmd-c


# HELP VARIABLES:

# Figure out whether the GCC is being used.
ifeq ($(shell $(PRINTF) '%s\n' ${CC} | $(HEAD) -n 1),gcc)
__USING_GCC = 1
endif


# BUILD VARIABLES:

# Optimisation settings for C code compilation.
ifndef OPTIMISE
ifndef DEBUG
OPTIMISE = -O2 -g
endif
ifdef DEBUG
ifdef __USING_GCC
OPTIMISE = -Og -g
endif
ifndef __USING_GCC
OPTIMISE = -g
endif
endif
endif

# Warning settings for C code compilation.
ifdef _PEDANTIC
_PEDANTIC = -pedantic
endif
ifndef WARN
ifndef DEBUG
WARN = -Wall
endif
ifdef DEBUG
ifdef __USING_GCC
WARN = -Wall -Wextra $(_PEDANTIC) -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs   \
       -Wtrampolines -Wmissing-prototypes -Wmissing-declarations -Wnested-externs                    \
       -Wno-variadic-macros -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align                     \
       -Wdeclaration-after-statement -Wundef -Wbad-function-cast -Wwrite-strings -Wlogical-op        \
       -Wstrict-prototypes -Wold-style-definition -Wpacked -Wvector-operation-performance            \
       -Wunsuffixed-float-constants -Wsuggest-attribute=const -Wsuggest-attribute=noreturn           \
       -Wsuggest-attribute=format -Wnormalized=nfkc -fstrict-aliasing -fipa-pure-const -ftree-vrp    \
       -fstack-usage -funsafe-loop-optimizations -Wshadow -Wredundant-decls -Winline -Wcast-qual     \
       -Wsign-conversion -Wstrict-overflow=5 -Wconversion -Wsuggest-attribute=pure -Wswitch-default  \
       -Wstrict-aliasing=1 -fstrict-overflow -Wfloat-equal -Wpadded -Waggregate-return               \
       -Wtraditional-conversion
endif
ifndef __USING_GCC
WARN = -Wall -Wextra $(_PEDANTIC)
endif
endif
endif

# Support for internationalisation?
ifndef WITHOUT_GETTEXT
_CPPFLAGS += -D'USE_GETTEXT=1'
endif

# Add CPP definitions for all directories.
_CPPFLAGS += $(foreach D,$(_ALL_DIRS),-D'$(D)="$($(D))"')

# Compilation and linking flags, and command.
ifndef CPPFLAGS
CPPFLAGS =
endif
ifndef CFLAGS
CFLAGS = $(OPTIMISE) $(WARN)
endif
ifndef LDFLAGS
LDFLAGS = $(OPTIMISE) $(WARN)
endif
__CC = $(CC) -std=$(_C_STD) -c $(_CPPFLAGS) $(_CFLAGS)
__LD = $(CC) -std=$(_C_STD) $(_LDFLAGS)
__CC_POST = $(CPPFLAGS) $(CFLAGS) $(EXTRA_CPPFLAGS) $(EXTRA_CFLAGS)
__LD_POST = $(LDFLAGS) $(EXTRA_LDFLAGS)

# Header files.
__H =
ifdef _HEADER_DIRLEVELS
ifeq ($(_HEADER_DIRLEVELS),1)
__H += src/*.h
endif
ifneq ($(_HEADER_DIRLEVELS),1)
ifeq ($(_HEADER_DIRLEVELS),2)
__H += src/*.h
__H += src/*/*.h
endif
ifneq ($(_HEADER_DIRLEVELS),2)
ifneq ($(_HEADER_DIRLEVELS),0)
__H += $(foreach W,$(shell $(SEQ) $(_HEADER_DIRLEVELS) | while read n; do $(ECHO) $$($(SEQ) $$n)" " | $(SED) 's/[^ ]* /\/\*/g'; done | $(XARGS) $(ECHO)),src$(W).h)
endif
endif
endif
endif

# Directory for the binary if there is only one binary.
ifndef _BINDIR
_BINDIR = BINDIR
endif


# BUILD RULES:

.PHONY: cmd-c
cmd-c: $(foreach B,$(_BIN) $(_SBIN) $(_LIBEXEC),bin/$(B))

# Compile a C file into an object file.
aux/%.o: $(v)src/%.c $(foreach H,$(__H),$(v)$(H))
	@$(PRINTF_INFO) '\e[00;01;31mCC\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p $(shell $(DIRNAME) $@)
	$(Q)$(__CC) -o $@ $< $(__CC_POST) #$Z
	@$(ECHO_EMPTY)

# Link object files into a command.
# Dependencies are declared below.
bin/%:
	@$(PRINTF_INFO) '\e[00;01;31mLD\e[34m %s\e[00;32m$A\n' "$@"
	@$(MKDIR) -p bin
	$(Q)$(__LD) -o $@ $^ $(__LD_POST) #$Z
	@$(ECHO_EMPTY)

# Dependencies for the rule above.
include aux/lang-c.mk
aux/lang-c.mk: Makefile
	@$(MKDIR) -p aux
	@$(ECHO) > aux/lang-c.mk
	@$(foreach B,$(_BIN) $(_SBIN) $(_LIBEXEC),$(ECHO) bin/$(B): $(foreach O,$(_OBJ_$(B)),aux/$(O).o) >> aux/lang-c.mk &&) $(TRUE)


# INSTALL RULES:

.PHONY: install-cmd-c
install-cmd-c: $(foreach B,$(_BIN) $(_SBIN) $(_LIBEXEC),bin/$(B))
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
ifndef COMMAND
ifdef _BIN
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(BINDIR)"
	$(Q)$(INSTALL_PROGRAM) $(__STRIP) $(foreach B,$(_BIN),bin/$(B)) -- "$(DESTDIR)$(BINDIR)"
endif
ifdef _SBIN
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(SBINDIR)"
	$(Q)$(INSTALL_PROGRAM) $(__STRIP) $(foreach B,$(_SBIN),bin/$(B)) -- "$(DESTDIR)$(SBINDIR)"
endif
ifdef _LIBEXEC
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(LIBEXECDIR)/$(PKGNAME)"
	$(Q)$(INSTALL_PROGRAM) $(__STRIP) $(foreach B,$(_LIBEXEC),bin/$(B)) -- "$(DESTDIR)$(LIBEXECDIR)/$(PKGNAME)"
endif
endif
ifdef COMMAND
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$($(_BINDIR))"
	$(Q)$(INSTALL_PROGRAM) $(__STRIP) $^ -- "$(DESTDIR)$($(_BINDIR))/$(COMMAND)"
endif
	@$(ECHO_EMPTY)


# UNINSTALL RULES:

.PHONY: uninstall-cmd-c
uninstall-cmd-c:
ifdef COMMAND
	-$(Q)$(RM) -- "$(DESTDIR)$($(_BINDIR))/$(COMMAND)"
endif
ifndef COMMAND
ifdef _BIN
	-$(Q)$(RM) -- $(foreach B,$(_BIN),"$(DESTDIR)$(BINDIR)/$(B)")
endif
ifdef _SBIN
	-$(Q)$(RM) -- $(foreach B,$(_SBIN),"$(DESTDIR)$(SBINDIR)/$(B)")
endif
ifdef _LIBEXEC
	-$(Q)$(RM) -- $(foreach B,$(_LIBEXEC),"$(DESTDIR)$(LIBEXECDIR)/$(PKGNAME)/$(B)")
	-$(Q)$(RMDIR) -- "$(DESTDIR)$(LIBEXECDIR)/$(PKGNAME)"
endif
endif


endif

