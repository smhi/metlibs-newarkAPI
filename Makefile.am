# $Id: Makefile.am 2786 2009-09-23 11:39:02Z dages $

pkgname = newarkAPI

include $(top_srcdir)/metlibs.mk

lib_LTLIBRARIES = libnewarkAPI.la

libnewarkAPI_la_SOURCES = diParam.cc diStation.cc diRoaddata.cc diRoaddatathread.cc

libnewarkAPI_la_CPPFLAGS = \
	${PGCONPOOL_CPPFLAGS} \
	${BOOST_CPPFLAGS}

if ENABLE_SHARED_LIBRARY
libnewarkAPI_la_LDFLAGS = \
	${PGCONPOOL_LDPLAGS} \
	${PUTOOLS_LDFLAGS} \
	${BOOST_LDFLAGS}

libnewarkAPI_la_LIBADD = \
	${PGCONPOOL_LIBS} \
	${PUTOOLS_LIBS} \
	${BOOST_LIBS} \
	${BOOST_THREAD_LIBS}
endif

pkg_includedir = ${includedir}/newarkAPI

pkg_include_HEADERS = \
    rdkESQLTypes.h \
	diParam.h \
	diRoaddata.h \
	diRoaddatathread.h \
	diStation.h