all: build 

include ../global.mak

CERTS = EquifaxSecureGlobaleBusinessCA.pem

build clean:
	
install :
	install -d $(PURPLE_CACERTS_DIR)
	for cert in $(CERTS); do \
		install -m 0644 $$cert $(PURPLE_CACERTS_DIR)/$$cert; \
	done

uninstall :
	for cert in $(CERTS); do \
		rm -f $(PURPLE_CACERTS_DIR)/$$cert; \
	done
	
DISTFILES = $(CERTS) Makefile
