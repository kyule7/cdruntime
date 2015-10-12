bin_PROGRAMS = test_prv_ref_remote

AM_CPPFLAGS = -I$(includedir) -I$(top_srcdir)/src 
# test_prv_ref_remote_LDADD = $(top_builddir)/src/libcds.a
test_prv_ref_remote_LDADD = -L$(libdir) -lcds
test_prv_ref_remote_SOURCES = test_prv_ref_remote.cc
