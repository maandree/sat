# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file includes rules for automatically rebuilding the makefile. ===#


base: Makefile

Makefile: $(v)Makefile.in config.status $(v)configure $(v)mk/configure
	./config.status

