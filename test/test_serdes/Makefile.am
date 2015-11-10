bin_PROGRAMS = test_serdes

AM_CPPFLAGS = -I$(includedir) 
test_serdes_LDADD = -L$(libdir) -lcd
test_serdes_SOURCES = test_serdes.cc

clean-tests:
	rm -rf debug_logs; rm -rf HDD; rm -rf SSD
