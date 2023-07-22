EXTRA_DIST = ltversion.sh .vinfo oousb2k-config.in

bin_SCRIPTS = oousb2k-config

VINFO = $(top_srcdir)/ltversion.sh
LT_RELEASE = $(shell $(VINFO) --version)
LT_VINFO   = $(shell $(VINFO) --version-info)

LIBS = -lm -lusb -lreadline -lncurses -lhistory

noinst_HEADERS = \
 command.h
include_HEADERS      = \
 oousb2k.h

lib_LTLIBRARIES     = liboousb2k.la
liboousb2k_la_SOURCES = \
 oousb2k.c

liboousb2k_la_LDFLAGS= \
 -version-info $(LT_VINFO)\
 -release $(LT_RELEASE)

DEFS = \
 @DEFS@ 

noinst_PROGRAMS  = oou2k-test

bin_PROGRAMS  = oou2ksh

oou2k_test_SOURCES = oou2k-test.c
oou2k_test_LDADD   = $(lib_LTLIBRARIES)

oou2ksh_SOURCES = command.c shell.c
oou2ksh_LDADD   = $(lib_LTLIBRARIES)
