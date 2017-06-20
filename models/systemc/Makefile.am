MAINTAINERCLEANFILES=  aclocal.m4 config.log config.status configure depcomp \
		      INSTALL install-sh Makefile.in missing

#SUBDIRS = DRAMSim2-2.2.2
manifold_systemclib_LIBRARIES = libmanifold_systemc.a
manifold_systemclibdir = $(libdir)/manifold
manifold_systemc_a_SOURCES = \
        manifold_systemc.cpp \
        manifold_systemc.h

pkginclude_manifold_systemcdir = $(includedir)/manifold/manifold_systemc

pkginclude_manifold_systemc_HEADERS = \
        manifold_systemc.h


libmanifold_systemc_a_CPPFLAGS = -I$(KERNEL_INC) -I$(SYSTEM_C)/include 


AM_CPPFLAGS = -I$(top_srcdir) $(FOO_CPPFLAGS)
