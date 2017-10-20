bin_PROGRAMS = test_simple_hierarchy
WRAPPER_PATH=$(prefix)/plugin/wrapper
PACKER_PATH=$(prefix)/src/persistence
AM_CPPFLAGS = -I$(includedir) -I$(WRAPPER_PATH) -I$(PACKER_PATH)
test_simple_hierarchy_LDADD = -L$(libdir) -lcd -L$(WRAPPER_PATH) -lwrapLibc -L$(PACKER_PATH) -lpacker
test_simple_hierarchy_SOURCES = test_simple_hierarchy.cc
