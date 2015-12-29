# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file defines variables for all used commands. ===#


# Part of GNU Coreutils:
BASENAME ?= basename
CHGRP ?= chgrp
CHMOD ?= chmod
CHOWN ?= chown
CP ?= cp
CPLIT ?= cplit
CUT ?= cut
DATE ?= date
DIRNAME ?= dirname
ECHO ?= echo
ENV ?= env
EXPAND ?= expand
EXPR ?= expr
FALS ?= false
FMT ?= fmt
FOLD ?= fold
HEAD ?= head
INSTALL ?= install
INSTALL_DATA ?= $(INSTALL) -m644
INSTALL_DIR ?= $(INSTALL) -dm755
INSTALL_PROGRAM ?= $(INSTALL) -m755
JOIN ?= join
LN ?= ln
MKDIR ?= mkdir
MKFIFO ?= mkfifo
MKNOD ?= mknod
MV ?= mv
NL ?= nl
NPROC ?= nproc
NUMFMT ?= numfmt
OD ?= od
PASTE ?= paste
PATHCHK ?= pathchk
PR ?= pr
PRINTF ?= printf
READLINK ?= readlink
REALPATH ?= realpath
RM ?= rm
RMDIR ?= rmdir
SEQ ?= seq
SLEEP ?= sleep
SORT ?= sort
SPLIT ?= split
STAT ?= stat
TAC ?= tac
TAIL ?= tail
TEE ?= tee
TEST ?= test
TOUCH ?= touch
TR ?= tr
TRUE ?= true
TRUNCATE ?= truncate
TSORT ?= tsort
UNAME ?= uname
UNEXPAND ?= unexpand
UNIQ ?= uniq
WC ?= wc
YES ?= yes

# Part of GNU help2man:
HELP2MAN ?= help2man

# Part of GNU tar:
TAR ?= tar

# Part of GNU Findutils:
FIND ?= find
XARGS ?= xargs

# Part of GNU Grep:
GREP ?= grep
EGREP ?= egrep
FGREP ?= fgrep

# Part of GNU Sed:
SED ?= sed

# Part of GNU Privacy Guard:
GPG ?= gpg

# Part of Texinfo:
MAKEINFO ?= makeinfo
MAKEINFO_HTML ?= $(MAKEINFO) --html
INSTALL_INFO ?= install-info

# Part of Texlive-plainextra:
TEXI2PDF ?= texi2pdf
TEXI2DVI ?= texi2dvi
TEXI2PS ?= texi2pdf --ps

# Part of Texlive-core:
PS2EPS ?= ps2eps

# Part of librsvg:
RSVG_CONVERT ?= rsvg-convert
SVG2PS ?= $(RSVG_CONVERT) --format=ps
SVG2PDF ?= $(RSVG_CONVERT) --format=pdf

# Part of GNU Compiler Collection:
CC ?= cc
CPP ?= cpp
CXX ?= c++

# Part of GNU Binutils:
AR ?= ar
LD ?= ld
RANLIB ?= ranlib

# Part of GNU Bison:
BISON ?= bison
YACC ?= yacc

# Part of Flex:
FLEX ?= FLEX
LEX ?= lex

# Part of GNU C Library:
LDCONFIG ?= ldconfig

# Part of GNU Gettext:
XGETTEXT ?= xgettext
MSGFMT ?= msgfmt
MSGMERGE ?= msgmerge
MSGINIT ?= msginit

# Part of gzip:
GZIP ?= gzip
GZIP_COMPRESS ?= $(GZIP) -k9

# Part of bzip2:
BZIP2 ?= bzip2
BZIP2_COMPRESS ?= $(BZIP2) -k9

# Part of xz:
XZ ?= xz
XZ_COMPRESS ?= $(XZ) -ke9

# Part of auto-auto-complete:
AUTO_AUTO_COMPLETE ?= auto-auto-complete


# Change to $(TRUE) to suppress the bold red and blue output.
ifndef PRINTF_INFO
PRINTF_INFO = $(PRINTF)
endif

# Change to $(TRUE) to suppress empty lines
ifndef ECHO_EMPTY
ECHO_EMPTY = $(ECHO)
endif

