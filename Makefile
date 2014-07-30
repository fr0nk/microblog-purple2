#
# Top-level makefile
#

include version.mak
include configure.mak

SUBDIRS = microblog $(BUILD_TWITGIN) certs
DISTFILES = COPYING global.mak Makefile mbpurple.nsi README.txt version.mak subversion.mak

.PHONY: all install clean build distdir

default: subversion build

build install uninstall clean : 
	for dir in $(SUBDIRS); do \
		$(MAKE) -C "$$dir" $@ || exit 1; \
	done

distdir: 
	rm -rf $(PACKAGE)-$(VERSION)$(SUBVERSION)
	mkdir $(PACKAGE)-$(VERSION)$(SUBVERSION)
	for dir in $(SUBDIRS); do \
		$(MAKE) -C "$$dir" dist; \
	done
	cp -f $(DISTFILES) $(PACKAGE)-$(VERSION)$(SUBVERSION)/

dist: distdir
	tar -zcvf $(PACKAGE)-$(VERSION)$(SUBVERSION).tar.gz $(PACKAGE)-$(VERSION)$(SUBVERSION)
	rm -rf $(PACKAGE)-$(VERSION)$(SUBVERSION)

windist: pidgin-microblog-$(VERSION)$(SUBVERSION).exe

pidgin-microblog.exe: build mbpurple.nsi
	makensis /DPRODUCT_VERSION=$(VERSION)$(SUBVERSION) mbpurple.nsi

pidgin-microblog-$(VERSION)$(SUBVERSION).exe: pidgin-microblog.exe
	mv -f $< $@
	
zipdist: pidgin-microblog-$(VERSION)$(SUBVERSION).zip

pidgin-microblog.zip: build
	rm -rf $(PACKAGE)-$(VERSION)$(SUBVERSION)
	mkdir $(PACKAGE)-$(VERSION)$(SUBVERSION)
	PURPLE_PLUGIN_DIR=$(PWD)/$(PACKAGE)-$(VERSION)$(SUBVERSION)/plugins && \
		PURPLE_INSTALL_DIR=$(PWD)/$(PACKAGE)-$(VERSION)$(SUBVERSION) && \
		mkdir -p $$PURPLE_PLUGIN_DIR && \
		for dir in $(SUBDIRS); do \
			$(MAKE) -C "$$dir" install PURPLE_INSTALL_DIR=$$PURPLE_INSTALL_DIR PURPLE_PLUGIN_DIR=$$PURPLE_PLUGIN_DIR; \
		done
	zip -r $@ $(PACKAGE)-$(VERSION)$(SUBVERSION)
	rm -rf $(PACKAGE)-$(VERSION)$(SUBVERSION)

pidgin-microblog-$(VERSION)$(SUBVERSION).zip: pidgin-microblog.zip
	mv -f $< $@

include subversion.mak
