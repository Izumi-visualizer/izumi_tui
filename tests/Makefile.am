check_PROGRAMS = test_logic test_screen test_properties test_paths test_faults
TESTS = $(check_PROGRAMS) test_cli.sh
AM_CPPFLAGS = -I$(top_srcdir)/src/include -I$(top_builddir) $(LIBIZUMI_CPPFLAGS) $(CURSES_CFLAGS)
LDADD = $(top_builddir)/src/libizumi_tui.la $(CURSES_LIBS)
if SYSTEM_LIBIZUMI
LDADD += -lizumi
AM_LDFLAGS = $(LIBIZUMI_LDFLAGS)
else
LDADD += $(LIBIZUMI_LA)
endif
test_paths_LDFLAGS = $(AM_LDFLAGS) -Wl,--wrap=getpwnam
test_faults_LDFLAGS = $(AM_LDFLAGS) -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=strdup
EXTRA_DIST = test.h test_cli.sh test_cli.py check_coverage.py
TESTS_ENVIRONMENT = IZUMI_BINARY='$(abs_top_builddir)/src/izumi' PYTHON3='$(PYTHON3)'
MOSTLYCLEANFILES = *.gcda *.gcno *.gcov
