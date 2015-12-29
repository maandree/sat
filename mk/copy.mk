# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for legal files. ===#


# Enables the rules:
#   install-copyright  Install all files in _COPYING and _LICENSE
#   install-copying    Install all files in _COPYING
#   install-license    Install all files in _LICENSE


# WHEN TO BUILD, INSTALL, AND UNINSTALL:

install-base: install-copyright
uninstall: uninstall-copyright


# INSTALL RULES:

.PHONY: install-copyright
install-copyright:

ifdef _COPYING
.PHONY: install-copyright
install-copyright: install-copying

.PHONY: install-copying
install-copying: $(foreach F,$(_COPYING),$(v)$(F))
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	@$(ECHO_EMPTY)
endif

ifdef _LICENSE
.PHONY: install-copyright
install-copyright: install-license

.PHONY: install-license
install-license: $(foreach F,$(_LICENSE),$(v)$(F))
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	@$(ECHO_EMPTY)
endif


# UNINSTALL RULES:

.PHONY: uninstall-copyright
uninstall-copyright:

ifdef _COPYING
.PHONY: uninstall-copyright
uninstall-copyright: uninstall-copying

.PHONY: uninstall-copying
uninstall-copying:
	-$(Q)$(RM) -- $(foreach F,$(_COPYING),"$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$(F)")
	-$(Q)$(RMDIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
endif

ifdef _LICENSE
.PHONY: uninstall-copyright
uninstall-copyright: uninstall-license

.PHONY: uninstall-license
uninstall-license:
	-$(Q)$(RM) -- $(foreach F,$(_LICENSE),"$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/$(F)")
	-$(Q)$(RMDIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
endif

