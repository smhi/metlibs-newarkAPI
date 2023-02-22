# $Id: Makefile.am 2786 2009-09-23 11:39:02Z dages $

pkgname = newarkAPI

#include $(top_srcdir)/metlibs.mk

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libnewarkAPI.la

libnewarkAPI_la_SOURCES = diParam.cc diStation.cc diRoaddata.cc diRoaddatathread.cc

libnewarkAPI_la_CPPFLAGS = \
	${PQXX_CPPFLAGS} \
	${PUTOOLS_CPPFLAGS} \
	${BOOST_CPPFLAGS}

if ENABLE_SHARED_LIBRARY
libnewarkAPI_la_LDFLAGS = \
	${PQXX_LDFLAGS} \
	${PUTOOLS_LDFLAGS} \
	${BOOST_LDFLAGS}

libnewarkAPI_la_LIBADD = \
	${PQXX_LIBS} \
	${PUTOOLS_LIBS} \
	${BOOST_LIBS} \
	${BOOST_THREAD_LIBS}
endif

pkgconfigdir = $(libdir)/pkgconfig

pkg_includedir = ${includedir}/newarkAPI

pkgconfig_DATA = newarkAPI.pc

pkg_include_HEADERS = \
    rdkESQLTypes.h \
	diParam.h \
	diRoaddata.h \
	diRoaddatathread.h \
	diStation.h
INSTALL=install -p # tells install to preserve timestamp
