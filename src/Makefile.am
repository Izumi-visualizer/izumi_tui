include $(srcdir)/izumi_tui.mk


bin_PROGRAMS = izumi
izumi_SOURCES = $(izumi_c_sources) $(izumi_headers)
noinst_LTLIBRARIES = libizumi_tui.la
libizumi_tui_la_SOURCES = $(izumi_core_sources) $(izumi_headers)
libizumi_tui_la_CPPFLAGS = $(izumi_CPPFLAGS)
MOSTLYCLEANFILES = *.gcda *.gcno *.gcov

izumi_CPPFLAGS = -I$(srcdir)/include $(LIBIZUMI_CPPFLAGS) $(CURSES_CFLAGS) \
    -DLOCALEDIR='"$(localedir)"'
izumi_LDADD = libizumi_tui.la $(CURSES_LIBS)

if SYSTEM_LIBIZUMI
izumi_LDADD += -lizumi
izumi_LDFLAGS = $(LIBIZUMI_LDFLAGS)
else
izumi_LDADD += $(LIBIZUMI_LA)
endif

ARGS ?=

.PHONY: run
run: $(builddir)/izumi
	cd $(top_builddir) && $(abs_builddir)/izumi $(ARGS)
