# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file includes all the other files in appropriate order. ===#


ifndef Q
A = \e[35m
Z = [m[D 
endif

include $(v)mk/path.mk
include .config.mk
include $(v)mk/path.mk
include $(v)mk/lowerpath.mk
include $(v)mk/empty.mk
include $(v)mk/tools.mk
include $(v)mk/copy.mk
include $(v)mk/lang-c.mk
include $(v)mk/texinfo.mk
include $(v)mk/man.mk
include $(v)mk/i18n.mk
include $(v)mk/clean.mk
include $(v)mk/dist.mk
include $(v)mk/tags.mk
include $(v)mk/prologue.mk

