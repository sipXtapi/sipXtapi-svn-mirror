## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../sipXcallLib/include

bin_PROGRAMS = basicImClient

basicImClient_LDADD = $(top_builddir)/src/libsipXtack.la @SIPXSDP_LIBS@ @SIPXPORT_LIBS@

basicImClient_SOURCES =  basicImClient.cpp
