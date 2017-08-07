SUBDIRS := src

.PHONY: all
all:
	$(MAKE) $@ -C $(SUBDIRS)

.PHONY: clean
clean:
	$(MAKE) $@ -C $(SUBDIRS)

.PHONY: install
install: all
	$(MAKE) $@ -C $(SUBDIRS)

.PHONY: uninstall
uninstall:
	$(MAKE) $@ -C $(SUBDIRS)
