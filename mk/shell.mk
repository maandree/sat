# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for shell tab-completion using auto-auto-complete. ===#


# Enables the rules:
#   shell          Build tab-completion for all supported shells
#   bash           Build GNU Bash tab-completion
#   fish           Build fish tab-completion
#   zhs            Build Z shell tab-completion
#   install-shell  Install tab-completion for all supported shells
#   install-bash   Install GNU Bash tab-completion
#   install-fish   Install fish tab-completion
#   install-zsh    Install Z shell tab-completion
# 
# This file is ignored unless
# _AUTO_COMPLETE is defined.
# 
# _AUTO_COMPLETE shall list all commands that
# have an auto-auto-complete. These should be
# named src/$(COMMAND).auto-completion, where
# $(COMMAND) is the command with the script.
# If all auto-auto-complete scripts translations
# named src/$(COMMAND).$(LOCALE).auto-completion,
# SHELL_LOCALE can be set to install exactly
# on translation in place of the non-translated
# versions.
# 
# Although not used by this file, you should
# define _SHELL_LOCALES that lists all available
# translations. (it is used by dist.mk.)
# 
# You should also define _WITH_SHELL if you
# want shell tab-completion unless the user
# specifies otherwise. If you want it for
# just some shells, define _WITH_$(SHELL)
# for those shells instead of _WITH_SHELL.


ifdef _AUTO_COMPLETE


# HELP VARIABLES:

# Include all that were not explicitly excluded?
ifdef _WITH_SHELL
_WITH_BASH = 1
_WITH_FISH = 1
_WITH_ZSH = 1
endif

# Include for GNU Bash?
ifdef WITH_BASH
__WITH_BASH = 1
endif
ifndef WITH_BASH
ifndef WITHOUT_BASH
ifdef _WITH_BASH
__WITH_BASH = 1
endif
endif
endif

# Include for fish?
ifdef WITH_FISH
__WITH_FISH = 1
endif
ifndef WITH_FISH
ifndef WITHOUT_FISH
ifdef _WITH_FISH
__WITH_FISH = 1
endif
endif
endif

# Include for Z Shell?
ifdef WITH_ZSH
__WITH_ZSH = 1
endif
ifndef WITH_ZSH
ifndef WITHOUT_ZSH
ifdef _WITH_ZSH
__WITH_ZSH = 1
endif
endif
endif

# WHEN TO BUILD, INSTALL, AND UNINSTALL:

all: shell
everything: shell
install: install-shell
install-doc: install-info install-dvi install-pdf install-ps install-html
uninstall: uninstall-shell

shell:
install-shell:

ifdef __WITH_BASH
shell: bash
install-shell: install-bash
endif
ifdef __WITH_FISH
shell: fish
install-shell: install-fish
endif
ifdef __WITH_ZSH
shell: zsh
install-shell: install-zsh
endif


# HELP VARIABLES:

# Affixes on the source files.
ifdef SHELL_LOCALE
__AAC_L = .$(SHELL_LOCALE)
endif
__AAC = $(__AAC_L).auto-completion

# Customised command name.
ifdef COMMAND
ifeq ($(shell $(PRINTF) '%s\n' $(COMMAND) | $(WC) -l),1)
ifeq ($(shell $(PRINTF) '%s\n' $(_AUTO_COMPLETE) | $(WC) -l),1)
__SHELL_COMMAND = "command=$(COMMAND)"
endif
endif
endif


# BUILD RULES:

# Built tab-completion scripts for GNU Bash.
.PHONY: bash
bash: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).bash-completion)

# Built tab-completion scripts for fish.
.PHONY: fish
fish: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).fish-completion)

# Built tab-completion scripts for Z shell.
.PHONY: zsh
zsh: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).zsh-completion)

# Built a tab-completion script for GNU Bash.
bin/%.bash-completion: $(v)src/%$(__AAC)
	@$(PRINTF_INFO) '\e[00;01;31mAUTO-AUTO-COMPLETE\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p bin
	$(Q)$(AUTO_AUTO_COMPLETE) bash -o $@ -s $< $(__SHELL_COMMAND) #$Z
	@$(ECHO_EMPTY)

# Built a tab-completion script for fish.
bin/%.fish-completion: $(v)src/%$(__AAC)
	@$(PRINTF_INFO) '\e[00;01;31mAUTO-AUTO-COMPLETE\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p bin
	$(Q)$(AUTO_AUTO_COMPLETE) fish -o $@ -s $< $(__SHELL_COMMAND) #$Z
	@$(ECHO_EMPTY)

# Built a tab-completion script for Z shell.
bin/%.zsh-completion: $(v)src/%$(__AAC)
	@$(PRINTF_INFO) '\e[00;01;31mAUTO-AUTO-COMPLETE\e[34m %s\e[00m$A\n' "$@"
	@$(MKDIR) -p bin
	$(Q)$(AUTO_AUTO_COMPLETE) zsh -o $@ -s $< $(__SHELL_COMMAND) #$Z
	@$(ECHO_EMPTY)


# INSTALL RULES:

# Install tab-completion scripts for GNU Bash.
.PHONY: install-bash
install-bash: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).bash-completion)
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
ifndef __SHELL_COMMAND
	$(Q)$(foreach F,$(_AUTO_COMPLETE),$(INSTALL_DATA) bin/$(F).bash-completion -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/$(F)" &&) $(TRUE)
endif
ifdef __SHELL_COMMAND
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/$(COMMAND)"
endif
	@$(ECHO_EMPTY)

# Install tab-completion scripts for fish.
.PHONY: install-fish
install-fish: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).fish-completion)
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(DATADIR)/fish/completions"
ifndef __SHELL_COMMAND
	$(Q)$(foreach F,$(_AUTO_COMPLETE),$(INSTALL_DATA) bin/$(F).fish-completion -- "$(DESTDIR)$(DATADIR)/fish/completions/$(F).fish" &&) $(TRUE)
endif
ifdef __SHELL_COMMAND
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(DATADIR)/fish/completions/$(COMMAND).fish"
endif
	@$(ECHO_EMPTY)

# Install tab-completion scripts for Z shell.
.PHONY: install-zsh
install-zsh: $(foreach F,$(_AUTO_COMPLETE),bin/$(F).zsh-completion)
	@$(PRINTF_INFO) '\e[00;01;31mINSTALL\e[34m %s\e[00m\n' "$@"
	$(Q)$(INSTALL_DIR) -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
ifndef __SHELL_COMMAND
	$(Q)$(foreach F,$(_AUTO_COMPLETE),$(INSTALL_DATA) bin/$(F).zsh-completion -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(F)" &&) $(TRUE)
endif
ifdef __SHELL_COMMAND
	$(Q)$(INSTALL_DATA) $^ -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(COMMAND)"
endif
	@$(ECHO_EMPTY)


# UNINSTALL RULES:

# Uninstall tab-completion.
.PHONY: uninstall-shell
uninstall-shell:
ifndef __SHELL_COMMAND
	-$(Q)$(RM) -- $(foreach F,$(_AUTO_COMPLETE),"$(DESTDIR)$(DATADIR)/bash-completion/completions/$(F)")
	-$(Q)$(RM) -- $(foreach F,$(_AUTO_COMPLETE),"$(DESTDIR)$(DATADIR)/fish/completions/$(F).fish")
	-$(Q)$(RM) -- $(foreach F,$(_AUTO_COMPLETE),"$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(F)")
endif
ifdef __SHELL_COMMAND
	-$(Q)$(RM) -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/$(COMMAND)"
	-$(Q)$(RM) -- "$(DESTDIR)$(DATADIR)/fish/completions/$(COMMAND).fish"
	-$(Q)$(RM) -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(COMMAND)"
endif


endif

