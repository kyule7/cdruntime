SUBDIRS = src

allTests:
	cd test; make;

allExamples:
	cd examples; make;


clean-local: clean-bin clean-lib

clean-lib:
	@if test $(prefix) != $(top_srcdir) ; then \
	  echo "Delete ./lib ./bin ./include" ; \
	  rm -rf ./lib ./bin ./include; \
	fi

clean-bin: clean-tests clean-examples

clean-tests:
	cd test; make clean;

clean-examples:
	cd examples; make clean;
