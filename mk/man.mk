# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for man pages. ===#


# Enables the rules:
#   install-man               Install all man page
#   install-man-untranslated  Install untranslated man page
#   install-man-locale        Install translated man page
# 
# This file is ignored unless _MAN_PAGE_SECTIONS
# is defined. _MAN_PAGE_SECTIONS should list all
# used man page sections. For all used sections
# there should also be a variable named
# _MAN_$(SECTION) that lists the suffixless
# basename of all man pages in that section.
# 
# The pathname of a man page should look
# like this: doc/man/$(DOCUMENT).$(SECTION)
# Translations looks like this:
# doc/man/$(DOCUMENT).$(LANGUAGE).$(SECTION)
# 
# For each language and section, there should
# be a variable _MAN_$(LANGUAGE)_$(SECTION)
# that lists all translated documents in that
# section and for that lanuage. These should
# be suffixless basenames. The lanuage counts
# as a suffix.
# 
# The translations of the man pages to
# install should be specified, by language,
# in the variable MAN_LOCALES.


ifdef _MAN_PAGE_SECTIONS


# WHEN TO BUILD, INSTALL, AND UNINSTALL:

install: install-man
install-everything: install-man
install-doc: install-man
uninstall: uninstall-man


# HELP VARIABLES

# Customisable man page filename.
ifdef COMMAND
ifeq ($(shell $(PRINTF) '%s\n' $(COMMAND) | $(WC) -l),1)
ifeq ($(shell $(PRINTF) '%s\n' $(_MAN_PAGE_SECTIONS) | $(WC) -l),1)
ifeq ($(shell $(PRINTF) '%s\n' $(_MAN_$(_MAN_PAGE_SECTIONS)) | $(WC) -l),1)
__MAN_COMMAND = $(COMMAND)$(MAN$(_MAN_PAGE_SECTIONS)EXT)
endif
endif
endif
endif


# INSTALL RULES:

.PHONY: install-man
install-man: install-man-untranslated install-man-locale

.PHONY: install-man-untranslated
install-man-untranslated:
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- $(foreach S,$(_MAN_PAGE_SECTIONS),"$(DESTDIR)$(MANDIR)/$(MAN$(S))")
ifndef __MAN_COMMAND
	$(Q)$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(S)),$(INSTALL_DATA) $(v)doc/man/$(P).$(S) -- "$(DESTDIR)$(MANDIR)/$(MAN$(S))/$(P)$(MAN$(S)EXT)" &&)) $(TRUE)
endif
ifdef __MAN_COMMAND
	$(Q)$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(S)),$(INSTALL_DATA) $(v)doc/man/$(P).$(S) -- "$(DESTDIR)$(MANDIR)/$(MAN$(S))/$(__MAN_COMMAND)" &&)) $(TRUE)
endif
	@$(ECHO_EMPTY)

.PHONY: install-man-locale
install-man-locale:
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(foreach L,$(MAN_LOCALES),$(INSTALL_DIR) -- $(foreach S,$(_MAN_PAGE_SECTIONS),"$(DESTDIR)$(MANDIR)/$(L)/$(MAN$(S))") &&) $(TRUE)
ifndef __MAN_COMMAND
	$(Q)$(foreach L,$(MAN_LOCALES),$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(L)_$(S)),$(INSTALL_DATA) $(v)doc/man/$(P).$(L).$(S) -- "$(DESTDIR)$(MANDIR)/$(L)/$(MAN$(S))/$(P)$(MAN$(S)EXT)" &&))) $(TRUE)
endif
ifdef __MAN_COMMAND
	$(Q)$(foreach L,$(MAN_LOCALES),$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(L)_$(S)),$(INSTALL_DATA) $(v)doc/man/$(P).$(L).$(S) -- "$(DESTDIR)$(MANDIR)/$(L)/$(MAN$(S))/$(__MAN_COMMAND)" &&))) $(TRUE)
endif
	@$(ECHO_EMPTY)


# UNINSTALL RULES:

.PHONY: uninstall-man
uninstall-man:
ifndef __MAN_COMMAND
	-$(Q)$(RM) -- $(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(S)),"$(DESTDIR)$(MANDIR)/$(MAN$(S))/$(P).$(MAN$(S)EXT)"))
	-$(Q)$(RM) -- $(foreach L,$(MAN_LOCALES),$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(L)_$(S)),"$(DESTDIR)$(MANDIR)/$(L)/$(MAN$(S))/$(P)$(MAN$(S)EXT)")))
endif
ifdef __MAN_COMMAND
	-$(Q)$(RM) -- "$(DESTDIR)$(MANDIR)/$(MAN$(_MAN_PAGE_SECTIONS))/$(__MAN_COMMAND)"
	-$(Q)$(RM) -- $(foreach L,$(MAN_LOCALES),$(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(L)_$(S)),"$(DESTDIR)$(MANDIR)/$(L)/$(MAN$(S))/$(__MAN_COMMAND)")))
endif


endif

