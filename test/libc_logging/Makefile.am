bin_PROGRAMS = test_libc_logging
WRAPPER_PATH=$(prefix)/plugin/wrapper
PACKER_PATH=$(prefix)/src/persistence
AM_CPPFLAGS = -I$(includedir) -I$(WRAPPER_PATH) -I$(PACKER_PATH)
test_libc_logging_LDADD = -L$(libdir) -lcd 
test_libc_logging_SOURCES = test_libc_logging.cc
