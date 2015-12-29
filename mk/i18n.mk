# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for internationalisation. ===#


# Enables the rules:
#   locale          Build all translations.
#   update-po       Update all .po files for further translation.
#   install-locale  Install all locales.
# 
# "All locales" are those listed in LOCALES.
# If LOCALES is not defined, this file is ignored.
# 
# If WITHOUT_GETTEXT is defined, `locale` and
# `install-locale` will not do anything.
# 
# _SRC should list all sources files, excluding the src/
# at the beginning of the pathnames.


ifdef LOCALES


# WHEN TO BUILD, INSTALL, AND UNINSTALL:

all: locale
everything: locale
install: install-locale
install-everything: install-locale
uninstall: uninstall-locale


# BUILD RULES:

# Build all translations.
ifdef WITHOUT_GETTEXT
.PHONY: locale
locale:
endif
ifndef WITHOUT_GETTEXT
.PHONY: locale
locale: $(foreach L,$(LOCALES),bin/mo/$(L)/messages.mo)
endif

# Update all translation files for further translation.
.PHONY: update-po
update-po: $(foreach L,$(LOCALES),po/$(L).po)

# Generate template for translations.
aux/$(_PROJECT).pot: $(foreach S,$(_SRC),$(v)src/$(S))
	@$(PRINTF_INFO) '\e[00;01;31mPOT\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p aux
	$(Q)$(CPP) -DUSE_GETTEXT=1 $^ |  \
	$(XGETTEXT) -o "$@" -Lc --from-code utf-8 --package-name "$(_PROJECT_FULL)"  \
	--package-version $(_VERSION) --no-wrap --force-po  \
	--copyright-holder '$(_COPYRIGHT_HOLDER)' - #$Z
	@$(ECHO_EMPTY)

# Create or update a translation file.
po/%.po: aux/$(_PROJECT).pot
	@$(PRINTF_INFO) '\e[00;01;31mPO\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p po
	$(Q)if ! $(TEST) -e $@; then  \
	  $(MSGINIT) --no-translator --no-wrap -i aux/$(_PROJECT).pot -o $@ -l $*;  \
	else  \
	  $(MSGMERGE) --no-wrap -U $@ aux/$(_PROJECT).pot;  \
	fi #$Z
	@$(TOUCH) $@
	@$(ECHO_EMPTY)

# Compile a translation file.
bin/mo/%/messages.mo: $(v)po/%.po
	@$(PRINTF_INFO) '\e[00;01;31mMO\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p bin/mo/$*
	$(Q)cd bin/mo/$* && $(MSGFMT) $(__back3unless_v)$< #$Z
	@$(ECHO_EMPTY)


# INSTALL RULES:

# Install all locales.
ifdef WITHOUT_GETTEXT
.PHONY: install-locale
install-locale:
endif
ifndef WITHOUT_GETTEXT
.PHONY: install-locale
install-locale: $(foreach L,$(LOCALES),bin/mo/$(L)/messages.mo)
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL) -dm755 -- $(foreach L,$(LOCALES),"$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES")
	$(Q)$(foreach L,$(LOCALES),$(INSTALL_DATA) bin/mo/$(L)/messages.mo -- "$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES/$(PKGNAME).mo" &&) $(TRUE)
	@$(ECHO_EMPTY)
endif


# UNINSTALL RULES:

# Uninstall all locales.
.PHONY: uninstall-locale
uninstall-locale:
	-$(Q)$(RM) -- $(foreach L,$(LOCALES),"$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES/$(PKGNAME).mo")


endif

