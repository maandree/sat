# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file overrides values uppercase path variables with values lowercase path variables. ===#


ifdef srcdir
VPATH = $(srcdir)
endif

ifdef prefix
PREFIX = $(prefix)
endif

ifdef sysconfdir
SYSCONFDIR = $(sysconfdir)
endif

ifdef sharedstatedir
COMDIR = $(sharedstatedir)
endif

ifdef localstatedir
VARDIR = $(localstatedir)
endif

ifdef runstatedir
RUNDIR = $(runstatedir)
endif

ifdef tmpdir
TMPDIR = $(tmpdir)
endif

ifdef devdir
DEVDIR = $(devdir)
endif

ifdef sysdir
SYSDIR = $(sysdir)
endif

ifdef procdir
PROCDIR = $(procdir)
endif

ifdef exec_prefix
EXEC_PREFIX = $(exec_prefix)
endif

ifdef bindir
BINDIR = $(bindir)
endif

ifdef sbindir
SBINDIR = $(sbindir)
endif

ifdef libexecdir
LIBEXECDIR = $(libexecdir)
endif

ifdef libdir
LIBDIR = $(libdir)
endif

ifdef includedir
INCLUDEDIR = $(includedir)
endif

ifdef oldincludedir
OLDINCLUDEDIR = $(oldincludedir)
endif

ifdef datarootdir
DATADIR = $(datarootdir)
endif

ifdef datadir
RESDIR = $(datadir)
endif

ifdef libdatarootdir
SYSDEPDATADIR = $(libdatarootdir)
endif

ifdef libdatadir
SYSDEPRESDIR = $(libdatadir)
endif

ifdef lispdir
LISPDIR = $(lispdir)
endif

ifdef localedir
LOCALEDIR = $(localedir)
endif

ifdef licensedir
LICENSEDIR = $(licensedir)
endif

ifdef cachedir
CACHEDIR = $(cachedir)
endif

ifdef spooldir
SPOOLDIR = $(spooldir)
endif

ifdef emptydir
EMPTYDIR = $(emptydir)
endif

ifdef logdir
LOGDIR = $(logdir)
endif

ifdef statedir
STATEDIR = $(statedir)
endif

ifdef gamedir
GAMEDIR = $(gamedir)
endif

ifdef sharedcachedir
COMCACHEDIR = $(sharedcachedir)
endif

ifdef sharedpooldir
COMPOOLDIR = $(sharedpooldir)
endif

ifdef sharedlogdir
COMLOGDIR = $(sharedlogdir)
endif

ifdef sharedlogdir
COMSTATEDIR = $(sharedstatedir)
endif

ifdef sharedgamedir
COMGAMEDIR = $(sharedgamedir)
endif

ifdef localtmpdir
VARTMPDIR = $(localtmpdir)
endif

ifdef sharedtmpdir
COMTMPDIR = $(sharedtmpdir)
endif

ifdef lockdir
LOCKDIR = $(lockdir)
endif

ifdef skeldir
SKELDIR = $(skeldir)
endif

ifdef selfprocdir
SELFPROCDIR = $(selfprocdir)
endif

ifdef docdir
DOCDIR = $(docdir)
endif

ifdef infodir
INFODIR = $(infodir)
endif

ifdef dvidir
DVIDIR = $(dvidir)
endif

ifdef pdfdir
PDFDIR = $(pdfdir)
endif

ifdef psdir
PSDIR = $(psdir)
endif

ifdef htmldir
HTMLDIR = $(htmldir)
endif

ifdef mandir
MANDIR = $(mandir)
endif

ifdef man0
MAN0 = $(man0)
endif

ifdef man1
MAN1 = $(man1)
endif

ifdef man2
MAN2 = $(man2)
endif

ifdef man3
MAN3 = $(man3)
endif

ifdef man4
MAN4 = $(man4)
endif

ifdef man5
MAN5 = $(man5)
endif

ifdef man6
MAN6 = $(man6)
endif

ifdef man7
MAN7 = $(man7)
endif

ifdef man8
MAN8 = $(man8)
endif

ifdef man9
MAN9 = $(man9)
endif

ifdef man0ext
MAN0EXT = $(man0ext)
endif

ifdef man1ext
MAN1EXT = $(man1ext)
endif

ifdef man2ext
MAN2EXT = $(man2ext)
endif

ifdef man3ext
MAN3EXT = $(man3ext)
endif

ifdef man4ext
MAN4EXT = $(man4ext)
endif

ifdef man5ext
MAN5EXT = $(man5ext)
endif

ifdef man6ext
MAN6EXT = $(man6ext)
endif

ifdef man7ext
MAN7EXT = $(man7ext)
endif

ifdef man8ext
MAN8EXT = $(man8ext)
endif

ifdef man9ext
MAN9EXT = $(man9ext)
endif

