all: cd_profiler.cc cd_profiler.o

 
cd_profiler.o: cd_profiler.cc
	@echo "\nCD profiler is being compiled $<"
	$(CC) -shared -Wl,-soname,libprofiler.so -I.. ${SIGHT_CFLAGS} $(CFLAGS) $(CDS_FLAGS) $< -o $@ -L${SIGHT_ROOT} -lsight_structure ${SIGHT_LINKFLAGS} 

clean:
	rm cd_profiler.o
