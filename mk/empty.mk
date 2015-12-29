# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file includes empty rules that are filled by other files. ===#


.PHONY: all
all:

.PHONY: everything
everything:

.PHONY: base
base:

.PHONY: cmd
cmd:

.PHONY: doc
doc:


.PHONY: check
check:


.PHONY: install
install:

.PHONY: install-everything
install-everything:

.PHONY: install-base
install-base:

.PHONY: install-cmd
install-cmd:

.PHONY: install-doc
install-doc:


.PHONY: installcheck
installcheck:


.PHONY: uninstall
uninstall:



.PHONY: all
all: base

.PHONY: everything
everything: base

.PHONY: base
base: cmd


.PHONY: install
install: install-base

.PHONY: install-everything
install-everything: install-base

.PHONY: install-base
install-base: install-cmd



.PHONY: install-strip
install-strip: __STRIP = -s
install-strip: install

.PHONY: install-everything-strip
install-everything-strip: __STRIP = -s
install-everything-strip: install-everything

.PHONY: install-base-strip
install-base-strip: __STRIP = -s
install-base-strip: install-base

.PHONY: install-cmd-strip
install-cmd-strip: __STRIP = -s
install-cmd-strip: install-cmd


