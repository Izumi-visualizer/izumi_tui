include $(srcdir)/izumi_tui.mk


bin_PROGRAMS = izumi
izumi_SOURCES = $(izumi_c_sources) $(izumi_headers)

izumi_CPPFLAGS = -I$(srcdir)/include $(LIBIZUMI_CPPFLAGS) $(CURSES_CFLAGS) 
izumi_LDADD = $(CURSES_LIBS)

if SYSTEM_LIBIZUMI
izumi_LDADD += -lizumi
izumi_LDFLAGS = $(LIBIZUMI_LDFLAGS)
else
izumi_LDADD += $(LIBIZUMI_LA)
endif

check_PROGRAMS = test_empty_rows
test_empty_rows_SOURCES = test_empty_rows.c \
    command_tree.c commands.c configure.c files.c interact.c window.c utils.c
test_empty_rows_CPPFLAGS = $(izumi_CPPFLAGS)
test_empty_rows_LDADD = $(izumi_LDADD)
test_empty_rows_LDFLAGS = $(izumi_LDFLAGS)

check-local: test_empty_rows
	./test_empty_rows

ARGS ?=

.PHONY: run
run: $(builddir)/izumi
	cd $(top_builddir) && $(abs_builddir)/izumi $(ARGS)
