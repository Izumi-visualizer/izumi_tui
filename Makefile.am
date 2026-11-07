SUBDIRS = $(subdirs_extra) src po tests

EXTRA_DIST = LICENSE

ACLOCAL_AMFLAGS = -I m4

.PHONY: run
run: all
	$(MAKE) -C $(builddir)/src run
