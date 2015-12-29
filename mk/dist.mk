# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for making releases. ===#


# CONFIGURATIONS:

# Formats for packages to generate.
DIST_FORMATS = tar tar.xz tar.bz2 tar.gz

# Define DO_NOT_SIGN if you do not want sign the release.

# You can select checksums to generate:
#   Declaring NO_BSD_SUM will omit `sum -r`.
#   Declaring NO_SYSV_SUM will omit `sum -s`.
#   List all checksum programs, that do not require any flags, in DIST_CHECKSUMS.

# Any missing flags for GnuPG?
GPG_FLAGS =

# What key should be used for signing?
GPG_KEY ?=

# You should define _EVERYTHING to enumerate all files that belong in
# the release tarball, lest 'git archive' will be used as a fallback.



# HELP VARIALES:

# You may use these to help list all files in _EVERYTHING
__EVERYTHING_COMMON = $(_COPYING) $(_LICENSE) README
__EVERYTHING_SRC = $(foreach F,$(_SRC),src/$(F))
__EVERYTHING_LOCALE = $(foreach L,$(LOCALES),po/$(L).po)
__EVERYTHING_SHELL = $(foreach F,$(_AUTO_COMPLETE),src/$(F).auto-completion)  \
                     $(foreach F,$(_AUTO_COMPLETE),$(foreac F,$(_SHELL_LOCALES),src/$(F).$(L),auto-completion))
__EVERYTHING_MK_ = all clean copy dist empty i18n lang-c lowerpath man path prologue tags texinfo tools
__EVERYTHING_MK = $(foreach F,$(__EVERYTHING_MK_),mk/$(F).mk) mk/configure mk/README configure Makefile.in
__EVERYTHING_MAN = $(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach P,$(_MAN_$(S)),doc/man/$(P).$(S)))  \
                   $(foreach S,$(_MAN_PAGE_SECTIONS),$(foreach L,$(MAN_LOCALES),$(foreach P,$(_MAN_$(L)_$(S)),doc/man/$(P).$(L).$(S))))
__EVERYTHING_ALL_COMMON = $(__EVERYTHING_COMMON) $(__EVERYTHING_MK) $(__EVERYTHING_MAN)  \
                          $(__EVERYTHING_LOCALE) $(__EVERYTHING_SRC) $(__EVERYTHING_SHELL)
# This one (__todo) if you have a todo file, but do not want it to be greped.
__TO__ = TO
__DO__ = DO
__todo = $(__TO__)$(__DO__)


# The packages and there detached signatures (if any.)
ifndef DO_NOT_SIGN
__DIST_FILES = $(foreach F,$(DIST_FORMATS),$(_PROJECT)-$(_VERSION).$(F) $(_PROJECT)-$(_VERSION).$(F).sig)
endif
ifdef DO_NOT_SIGN
__DIST_FILES = $(foreach F,$(DIST_FORMATS),$(_PROJECT)-$(_VERSION).$(F))
endif

# All known checksum programs that are unambiguous.
__DIST_CHECKSUMS_ALL = cksum md2sum md4sum md5sum md6sum sha0sum sha1sum sha224sum  \
                       sha256sum sha384sum sha512sum sha512-224sum sha512-256sum \
                       sha3-224sum sha3-256sum sha3-384sum sha3-512sum shake256sum  \
                       shake512sum rawshake256sum rawshake512sum keccak-224sum  \
                       keccak-256sum keccak-384sum keccak-512sum keccaksum

# All installed checksum programs that are unambiguous.
DIST_CHECKSUMS = $(foreach C,$(__DIST_CHECKSUMS_ALL),$(shell if command -v $(C) >/dev/null; then $(ECHO) $(C); fi))



# MAKE RULES:

# Generate release files. (Tarballs, compressed and uncompressed; signatures; checksums.)
.PHONY: dist
dist: dist-balls dist-checksums



# HELP MAKE RULES:

# Generate tarballs, compressed and uncompressed,
.PHONY: dist-balls
dist-balls: $(__DIST_FILES)

ifndef DO_NOT_SIGN
# Generate checksums and signature of checksums.
.PHONY: dist-checksums
dist-checksums: $(_PROJECT)-$(_VERSION).checksums $(_PROJECT)-$(_VERSION).checksums.sig

# Generate uncompressed tarball and signature of it.
.PHONY: dist-tar
dist-tar: $(_PROJECT)-$(_VERSION).tar $(_PROJECT)-$(_VERSION).tar.sig

# Generate xz-copressed tarball and signature of it.
.PHONY: dist-xz
dist-xz: $(_PROJECT)-$(_VERSION).tar.xz $(_PROJECT)-$(_VERSION).tar.xz.sig

# Generate bz2-copressed tarball and signature of it.
.PHONY: dist-bzip2
dist-bz2: $(_PROJECT)-$(_VERSION).tar.bz2 $(_PROJECT)-$(_VERSION).tar.bz2.sig

# Generate gzip-copressed tarball and signature of it.
.PHONY: dist-gz
dist-gz: $(_PROJECT)-$(_VERSION).tar.gz $(_PROJECT)-$(_VERSION).tar.sig
endif
ifdef DO_NOT_SIGN
# Generate checksums, but no signature.
.PHONY: dist-checksums
dist-checksums: $(_PROJECT)-$(_VERSION).checksums

# Generate uncompressed tarball, but no signature.
.PHONY: dist-tar
dist-tar: $(_PROJECT)-$(_VERSION).tar

# Generate xz-compressed tarball, but no signature.
.PHONY: dist-xz
dist-xz: $(_PROJECT)-$(_VERSION).tar.xz

# Generate bzip2-compressed tarball, but no signature.
.PHONY: dist-bz2
dist-bz2: $(_PROJECT)-$(_VERSION).tar.bz2

# Generate gzip-compressed tarball, but no signature.
.PHONY: dist-gz
dist-gz: $(_PROJECT)-$(_VERSION).tar.gz
endif

# Generate the tarball for the release.
ifdef v
__GIT_DIR = --git-dir=$(v)
endif
$(_PROJECT)-$(_VERSION).tar: $(foreach F,$(_EVERYTHING),$(v)$(F))
	@$(PRINTF_INFO) '\e[00;01;31mTAR\e[34m %s\e[00m$A\n' "$@"
	@if $(TEST) -f $@; then $(RM) $@; fi
ifndef _EVERYTHING
	$(Q)git archive --prefix=$(_PROJECT)-$(_VERSION)/ --format=tar $(__GIT_DIR) $(_VERSION) -o $@ #$Z
endif
ifdef _EVERYTHING
	$(Q)$(LN) -s . $(_PROJECT)-$(_VERSION)
	$(Q)$(TAR) c -- $(foreach F,$(_EVERYTHING),$(v)$(_PROJECT)-$(_VERSION)/$(F)) > $@
	$(Q)$(RM) $(_PROJECT)-$(_VERSION)
endif
	@$(ECHO_EMPTY)

# Compression rule for xz-compression. Used on the tarball.
%.xz: %
	@$(PRINTF_INFO) '\e[00;01;31mXZ\e[34m %s\e[00m$A\n' "$@"
	@if $(TEST) -f $@; then $(RM) $@; fi
	$(Q)$(XZ_COMPRESS) $< #$Z
	@$(ECHO_EMPTY)

# Compression rule for bzip2-compression. Used on the tarball.
%.bz2: %
	@$(PRINTF_INFO) '\e[00;01;31mBZIP2\e[34m %s\e[00m$A\n' "$@"
	@if $(TEST) -f $@; then $(RM) $@; fi
	$(Q)$(BZIP2_COMPRESS) $< #$Z
	@$(ECHO_EMPTY)

# Compression rule for gzip-compression. Used on the tarball.
%.gz: %
	@$(PRINTF_INFO) '\e[00;01;31mGZIP\e[34m %s\e[00m$A\n' "$@"
	@if $(TEST) -f $@; then $(RM) $@; fi
	$(Q)$(GZIP_COMPRESS) $< #$Z
	@$(ECHO_EMPTY)

# Generate checksums of tarballs, compressed and uncompressed, and of their detached signatures.
$(_PROJECT)-$(_VERSION).checksums: $(__DIST_FILES)
	@$(PRINTF_INFO) '\e[00;01;31mCHECKSUM\e[34m %s\e[00m\n' "$@"
	@$(PRINTF) '' > $@
ifndef NO_BSD_SUM
	@if ! ($(ECHO) ':: sum -r ::' && sum -r $^ && $(ECHO)) >> $@ ; then $(PRINTF) '' > $@; fi
endif
ifndef NO_SYSV_SUM
	@if ! ($(ECHO) ':: sum -s ::' && sum -s $^ && $(ECHO)) >> $@ ; then $(PRINTF) '' > $@; fi
endif
	@$(foreach C,$(DIST_CHECKSUMS),$(ECHO) ':: $(C) ::' >> $@ && $(C) $^ | $(GREP) -v '^--' >> $@ && $(ECHO) >> $@ &&) $(TRUE)
	@$(ECHO_EMPTY)

# Signing rule. Used on the tarballs, compressed and uncompressed, and on the checksum file.
%.sig: %
	@$(PRINTF_INFO) '\e[00;01;31mSIG\e[34m %s\e[00m$A\n' "$@"
	@if $(TEST) -f $@; then $(RM) $@; fi
	$(Q)$(GPG) $(GPG_FLAGS) --local-user $(GPG_KEY) --detach-sign --armor --output $@ < $< #$Z
	@$(ECHO_EMPTY)

