bin_PROGRAMS = test_vector

AM_CPPFLAGS = -I$(includedir) -I$(includedir)/../plugin/wrapper -I$(includedir)/../src/persistence
test_vector_LDADD = -L$(libdir) -lcd -L${YAML_PATH}/lib -lyaml
test_vector_SOURCES = test_vector.cc
