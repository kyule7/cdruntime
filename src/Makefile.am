#lib_LIBRARIES = libcd.a
include_HEADERS = cd.h \
									cd_global.h \
									cd_features.h \
									cd_handle.h \
									node_id.h \
									sys_err_t.h \
									serdes.h \
									serializable.h \
									profiler_interface.h \
									error_injector.h

# Libtool-related
lib_LTLIBRARIES = libcd.la

libcd_la_SOURCES = \
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
  	                error_injector.cc \
										profiler_interface.cc \
  	                recover_object.cc \
										serdes.cc \
  	                sys_err_t.cc \
  	                unpacker.cc

#libcd_la_CPPFLAGS = $(cpp_flags)
#libcd_la_LIBADD = $(external_libs)
