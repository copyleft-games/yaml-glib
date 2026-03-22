# rules.mk - yaml-glib Build Rules
# Pattern rules and common build recipes
#
# Copyright (C) 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later

# ── Object file compilation ──────────────────────────────────────────

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# ── Test compilation ─────────────────────────────────────────────────
# .PRECIOUS prevents make from deleting test objects as intermediates
.PRECIOUS: $(OBJDIR)/tests/%.o

$(OBJDIR)/tests/%.o: tests/%.c | $(OBJDIR)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(TEST_CFLAGS) -MMD -MP -c $< -o $@

# ── Static library ───────────────────────────────────────────────────

$(OUTDIR)/$(LIB_STATIC): $(LIB_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(AR) rcs $@ $^

# ── Shared library ───────────────────────────────────────────────────

ifeq ($(TARGET_PLATFORM),windows)
$(OUTDIR)/$(LIB_SHARED_FULL): $(LIB_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(LIB_SHARED) and $(LIB_IMPORT)"
else
$(OUTDIR)/$(LIB_SHARED_FULL): $(LIB_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ $(LDFLAGS)
	cd $(OUTDIR) && ln -sf $(LIB_SHARED_FULL) $(LIB_SHARED_MAJOR)
	cd $(OUTDIR) && ln -sf $(LIB_SHARED_MAJOR) $(LIB_SHARED)
endif

# ── GIR generation ───────────────────────────────────────────────────

$(OUTDIR)/$(GIR_FILE): $(LIB_SRCS) $(LIB_HDRS) | $(OUTDIR)/$(LIB_SHARED_FULL)
	$(GIR_SCANNER) \
		--namespace=$(GIR_NAMESPACE) \
		--nsversion=$(GIR_VERSION) \
		--library=$(LIB_NAME) \
		--library-path=$(OUTDIR) \
		--include=GLib-2.0 \
		--include=GObject-2.0 \
		--include=Gio-2.0 \
		--pkg=glib-2.0 \
		--pkg=gobject-2.0 \
		--pkg=gio-2.0 \
		--pkg=yaml-0.1 \
		--pkg=json-glib-1.0 \
		--output=$@ \
		--warn-all \
		-Isrc \
		$(LIB_HDRS) $(LIB_SRCS)

$(OUTDIR)/$(TYPELIB_FILE): $(OUTDIR)/$(GIR_FILE)
	$(GIR_COMPILER) --output=$@ $<

# ── Generated files ──────────────────────────────────────────────────

# Version header
src/yaml-version.h: src/yaml-version.h.in
	sed \
		-e 's|@YAML_GLIB_VERSION_MAJOR@|$(VERSION_MAJOR)|g' \
		-e 's|@YAML_GLIB_VERSION_MINOR@|$(VERSION_MINOR)|g' \
		-e 's|@YAML_GLIB_VERSION_MICRO@|$(VERSION_MICRO)|g' \
		-e 's|@YAML_GLIB_VERSION@|$(VERSION)|g' \
		$< > $@

# pkg-config file
$(OUTDIR)/yaml-glib-$(API_VERSION).pc: yaml-glib-$(API_VERSION).pc.in | $(OUTDIR)
	sed \
		-e 's|@PREFIX@|$(PREFIX)|g' \
		-e 's|@LIBDIR@|$(LIBDIR)|g' \
		-e 's|@INCLUDEDIR@|$(INCLUDEDIR)|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		-e 's|@API_VERSION@|$(API_VERSION)|g' \
		$< > $@

# ── Directory creation ───────────────────────────────────────────────

$(BUILDDIR):
	@$(MKDIR_P) $(BUILDDIR)

$(OBJDIR): | $(BUILDDIR)
	@$(MKDIR_P) $(OBJDIR)
	@$(MKDIR_P) $(OBJDIR)/tests

$(OUTDIR):
	@$(MKDIR_P) $(OUTDIR)

$(OUTDIR)/examples:
	@$(MKDIR_P) $(OUTDIR)/examples

# ── Clean rules ──────────────────────────────────────────────────────

.PHONY: clean clean-all

clean:
	rm -rf $(BUILDDIR)/$(BUILD_TYPE)
	rm -f src/yaml-version.h

clean-all:
	rm -rf $(BUILDDIR)
	rm -f src/yaml-version.h

# ── Installation rules ───────────────────────────────────────────────

.PHONY: install install-lib install-headers install-pc install-gir

install: install-lib install-headers install-pc
ifeq ($(BUILD_GIR),1)
install: install-gir
endif

ifeq ($(TARGET_PLATFORM),windows)
install-lib:
	@echo "Install target is for native Linux builds only."
	@echo "For Windows, copy the DLL and headers manually."
else
install-lib: $(OUTDIR)/$(LIB_STATIC) $(OUTDIR)/$(LIB_SHARED_FULL)
	$(MKDIR_P) $(DESTDIR)$(LIBDIR)
	$(INSTALL_DATA) $(OUTDIR)/$(LIB_STATIC) $(DESTDIR)$(LIBDIR)/
	$(INSTALL_PROGRAM) $(OUTDIR)/$(LIB_SHARED_FULL) $(DESTDIR)$(LIBDIR)/
	cd $(DESTDIR)$(LIBDIR) && ln -sf $(LIB_SHARED_FULL) $(LIB_SHARED_MAJOR)
	cd $(DESTDIR)$(LIBDIR) && ln -sf $(LIB_SHARED_MAJOR) $(LIB_SHARED)
	@if [ -z "$(DESTDIR)" ] && command -v ldconfig >/dev/null 2>&1; then \
		echo "Updating shared library cache..."; \
		ldconfig; \
	fi
endif

install-headers:
	$(MKDIR_P) $(DESTDIR)$(INCLUDEDIR)/yaml-glib
	@for hdr in $(LIB_HDRS_PUBLIC); do \
		$(INSTALL_DATA) "$$hdr" $(DESTDIR)$(INCLUDEDIR)/yaml-glib/; \
	done
	@if [ -f src/yaml-version.h ]; then \
		$(INSTALL_DATA) src/yaml-version.h $(DESTDIR)$(INCLUDEDIR)/yaml-glib/; \
	fi

install-pc: $(OUTDIR)/yaml-glib-$(API_VERSION).pc
	$(MKDIR_P) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL_DATA) $(OUTDIR)/yaml-glib-$(API_VERSION).pc $(DESTDIR)$(PKGCONFIGDIR)/

install-gir: $(OUTDIR)/$(GIR_FILE) $(OUTDIR)/$(TYPELIB_FILE)
	$(MKDIR_P) $(DESTDIR)$(GIRDIR)
	$(MKDIR_P) $(DESTDIR)$(TYPELIBDIR)
	$(INSTALL_DATA) $(OUTDIR)/$(GIR_FILE) $(DESTDIR)$(GIRDIR)/
	$(INSTALL_DATA) $(OUTDIR)/$(TYPELIB_FILE) $(DESTDIR)$(TYPELIBDIR)/

# ── Uninstall ────────────────────────────────────────────────────────

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_STATIC)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SHARED_FULL)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SHARED_MAJOR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SHARED)
	rm -rf $(DESTDIR)$(INCLUDEDIR)/yaml-glib
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/yaml-glib-$(API_VERSION).pc
	rm -f $(DESTDIR)$(GIRDIR)/$(GIR_FILE)
	rm -f $(DESTDIR)$(TYPELIBDIR)/$(TYPELIB_FILE)
