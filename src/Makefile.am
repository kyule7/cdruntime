#lib_LIBRARIES = libcds.a
include_HEADERS = cds.h \
									cd_global.h \
									cd_features.h \
									cd_handle.h \
									node_id.h \
									sys_err_t.h \
									serializable.h \
									profiler_interface.h \
									error_injector.h

# Libtool-related
lib_LTLIBRARIES = libcds.la

libcds_la_SOURCES = \
  	                cd.cc \
  	                cd_comm_internal.cc \
  	                cd_comm_log.cc \
  	                cd_entry.cc \
  	                cd_event.cc \
  	                cd_handle.cc \
  	                cd_id.cc \
  	                cd_log_handle.cc \
  	                cd_malloc.cc \
  	                cd_mpiwrapper.cc \
  	                cd_name_t.cc \
  	                cd_pfs.cc \
  	                cd_profiler.cc \
  	                data_handle.cc \
  	                event_handler.cc \
  	                libc_rn.cc \
  	                node_id.cc \
  	                packer.cc \
										profiler_interface.cc \
  	                recover_object.cc \
  	                sys_err_t.cc \
  	                unpacker.cc

#libcds_la_CPPFLAGS = $(cpp_flags)
#libcds_la_LIBADD = $(external_libs)
