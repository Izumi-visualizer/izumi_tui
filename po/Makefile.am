# Translations, built with msgfmt and installed as
# $(localedir)/<lang>/LC_MESSAGES/$(PACKAGE).mo
#
# To add a language: create <lang>.po from $(PACKAGE).pot (e.g. with msginit)
# and add it to LINGUAS, POFILES and MOFILES below.
#
# After changing translatable strings in the sources, run `make update-po`
# (needs xgettext and msgmerge) to refresh the template and the catalogs.

DOMAIN = $(PACKAGE)
LINGUAS = ca es
POFILES = ca.po es.po
MOFILES = ca.mo es.mo

EXTRA_DIST = POTFILES $(DOMAIN).pot $(POFILES)

SUFFIXES = .po .mo

.po.mo:
	$(AM_V_GEN)$(MSGFMT) --check -o $@ $<

if BUILD_MO
all-local: $(MOFILES)

install-data-local: $(MOFILES)
	@for lang in $(LINGUAS); do \
	    dir="$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES"; \
	    echo " $(MKDIR_P) '$$dir'"; \
	    $(MKDIR_P) "$$dir" || exit 1; \
	    echo " $(INSTALL_DATA) $$lang.mo '$$dir/$(DOMAIN).mo'"; \
	    $(INSTALL_DATA) $$lang.mo "$$dir/$(DOMAIN).mo" || exit 1; \
	done

uninstall-local:
	@for lang in $(LINGUAS); do \
	    rm -f "$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo"; \
	done
endif

CLEANFILES = $(MOFILES)

.PHONY: update-pot update-po
update-pot:
	$(XGETTEXT) --directory=$(top_srcdir) --files-from=$(srcdir)/POTFILES \
	    --from-code=UTF-8 --keyword=_ --keyword=N_ --add-comments=TRANSLATORS: \
	    --package-name=$(PACKAGE) --package-version=$(VERSION) \
	    --msgid-bugs-address=https://github.com/Izumi-visualizer/izumi_tui/issues \
	    --output=$(srcdir)/$(DOMAIN).pot

update-po: update-pot
	@for lang in $(LINGUAS); do \
	    echo "$(MSGMERGE) --update --backup=none $(srcdir)/$$lang.po $(srcdir)/$(DOMAIN).pot"; \
	    $(MSGMERGE) --update --backup=none $(srcdir)/$$lang.po $(srcdir)/$(DOMAIN).pot || exit 1; \
	done
