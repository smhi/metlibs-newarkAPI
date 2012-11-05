# $Id: Makefile.am 2786 2009-09-23 11:39:02Z dages $

pkgname = newarkAPI

include $(top_srcdir)/metlibs.mk

lib_LTLIBRARIES = libnewarkAPI.la

qt_sources = \
	diParam.cc \
	diRoaddata.cc \
	diRoaddatathread.cc \
	diStation.cc

nodist_libnewarkAPI_la_SOURCES = \
	$(qt_sources:.cc=.moc.cc)

libnewarkAPI_la_SOURCES = \
	$(qt_sources)

libnewarkAPI_la_CPPFLAGS = \
	${PGCONPOOL_CPPFLAGS} \
	${QT4_CPPFLAGS}

if ENABLE_SHARED_LIBRARY
libnewarkAPI_la_LDFLAGS = \
	${PGCONPOOL_LDPLAGS} \
	${PUTOOLS_LDFLAGS} \
	${QT4_LDFLAGS}

libnewarkAPI_la_LIBADD = \
	${PGCONPOOL_LIBS} \
	${PUTOOLS_LIBS} \
	${QT4_LIBS}
endif
# Do we really want to do this?  Wouldn't it be better to include the
# generated files in the dist tarball, like we do with configure etc.?
CLEANFILES = $(qt_sources:.cc=.moc.cc)

pkg_includedir = ${includedir}/newarkAPI

pkg_include_HEADERS = \
    rdkESQLTypes.h \
	diParam.h \
	diRoaddata.h \
	diRoaddatathread.h \
	diStation.h