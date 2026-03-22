# Makefile - yaml-glib
# GLib/GObject-based YAML parsing and generation library
#
# Copyright (C) 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Usage:
#   make              - Build static and shared libraries
#   make lib          - Build static and shared libraries
#   make gir          - Generate GIR/typelib for introspection
#   make examples     - Build example programs
#   make test         - Build and run the test suite
#   make install      - Install to PREFIX
#   make clean        - Clean build artifacts for current build type
#   make clean-all    - Clean all build artifacts
#   make DEBUG=1      - Build with debug symbols
#   make ASAN=1       - Build with AddressSanitizer
#   make UBSAN=1      - Build with UndefinedBehaviorSanitizer

.DEFAULT_GOAL := all
.PHONY: all lib gir examples test check-deps help

# Include configuration
include config.mk

# Check dependencies before anything else (skip for targets that don't need them)
SKIP_DEP_CHECK_TARGETS := install-deps help check-deps show-config clean clean-all
ifeq ($(filter $(SKIP_DEP_CHECK_TARGETS),$(MAKECMDGOALS)),)
$(foreach dep,$(DEPS_REQUIRED),$(call check_dep,$(dep)))
endif

# ── Source files ─────────────────────────────────────────────────────

LIB_SRCS := \
	src/yaml-node.c \
	src/yaml-mapping.c \
	src/yaml-sequence.c \
	src/yaml-document.c \
	src/yaml-parser.c \
	src/yaml-builder.c \
	src/yaml-generator.c \
	src/yaml-serializable.c \
	src/yaml-gobject.c \
	src/yaml-schema.c

# ── Header files (for GIR scanner and installation) ──────────────────

LIB_HDRS_PUBLIC := \
	src/yaml-glib.h \
	src/yaml-types.h \
	src/yaml-node.h \
	src/yaml-mapping.h \
	src/yaml-sequence.h \
	src/yaml-document.h \
	src/yaml-parser.h \
	src/yaml-builder.h \
	src/yaml-generator.h \
	src/yaml-serializable.h \
	src/yaml-gobject.h \
	src/yaml-schema.h

LIB_HDRS_PRIVATE := \
	src/yaml-private.h

LIB_HDRS := $(LIB_HDRS_PUBLIC) $(LIB_HDRS_PRIVATE)

# ── Object file mappings ─────────────────────────────────────────────

LIB_OBJS := $(patsubst src/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))

# ── Test sources ─────────────────────────────────────────────────────

TEST_SRCS := $(wildcard tests/test-*.c)
TEST_OBJS := $(patsubst tests/%.c,$(OBJDIR)/tests/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst tests/%.c,$(OUTDIR)/%$(EXE_EXT),$(TEST_SRCS))

# ── Example sources ──────────────────────────────────────────────────

EXAMPLE_SRCS := \
	examples/yaml-print.c \
	examples/yaml-to-json.c \
	examples/json-to-yaml.c

EXAMPLE_BINS := $(patsubst examples/%.c,$(OUTDIR)/examples/%$(EXE_EXT),$(EXAMPLE_SRCS))

# Include build rules
include rules.mk

# ── Source dependencies on generated headers ─────────────────────────
$(LIB_OBJS): src/yaml-version.h

# ── Default target ───────────────────────────────────────────────────

all: src/yaml-version.h lib
ifeq ($(BUILD_GIR),1)
all: gir
endif
ifeq ($(BUILD_EXAMPLES),1)
all: examples
endif

# ── Build the library ────────────────────────────────────────────────

lib: src/yaml-version.h \
     $(OUTDIR)/$(LIB_STATIC) $(OUTDIR)/$(LIB_SHARED_FULL) \
     $(OUTDIR)/yaml-glib-$(API_VERSION).pc

# ── Build GIR/typelib ────────────────────────────────────────────────

gir: $(OUTDIR)/$(GIR_FILE) $(OUTDIR)/$(TYPELIB_FILE)

# ── Build examples ───────────────────────────────────────────────────

examples: lib $(OUTDIR)/examples $(EXAMPLE_BINS)

$(OUTDIR)/examples/%$(EXE_EXT): examples/%.c $(OUTDIR)/$(LIB_STATIC)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -o $@ $< $(OUTDIR)/$(LIB_STATIC) $(LDFLAGS)

# ── Build and run tests ──────────────────────────────────────────────

$(OUTDIR)/test-%$(EXE_EXT): $(OBJDIR)/tests/test-%.o $(OUTDIR)/$(LIB_SHARED_FULL)
	$(CC) -o $@ $< $(TEST_LDFLAGS)

test: lib $(TEST_BINS)
ifeq ($(TARGET_PLATFORM),windows)
	@echo "Cross-compiled tests cannot be run on Linux. Use Wine or copy to Windows."
	@echo "Test binaries built in $(OUTDIR)/"
else ifeq ($(CROSS),aarch64-linux-gnu)
	@echo "Running ARM64 tests via QEMU (requires qemu-user-static)..."
	@failed=0; total=0; passed=0; \
	for test in $(TEST_BINS); do \
		total=$$((total + 1)); \
		echo "  Running $$(basename $$test)..."; \
		if qemu-aarch64 -L $(ARM64_SYSROOT) $$test; then \
			echo "    PASS"; \
			passed=$$((passed + 1)); \
		else \
			echo "    FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$passed/$$total passed"; \
	if [ $$failed -gt 0 ]; then \
		echo "$$failed test(s) failed"; \
		exit 1; \
	else \
		echo "All tests passed"; \
	fi
else
	@echo "Running tests..."
	@failed=0; total=0; passed=0; \
	for test in $(TEST_BINS); do \
		total=$$((total + 1)); \
		echo "  Running $$(basename $$test)..."; \
		if LD_LIBRARY_PATH=$(OUTDIR) $$test; then \
			echo "    PASS"; \
			passed=$$((passed + 1)); \
		else \
			echo "    FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$passed/$$total passed"; \
	if [ $$failed -gt 0 ]; then \
		echo "$$failed test(s) failed"; \
		exit 1; \
	else \
		echo "All tests passed"; \
	fi
endif

# ── Check dependencies ───────────────────────────────────────────────

check-deps:
	@echo "Checking dependencies..."
	@for dep in $(DEPS_REQUIRED); do \
		if $(PKG_CONFIG) --exists $$dep; then \
			ver=$$($(PKG_CONFIG) --modversion $$dep 2>/dev/null); \
			echo "  $$dep: OK ($$ver)"; \
		else \
			echo "  $$dep: MISSING"; \
		fi; \
	done

# ── Help ──────────────────────────────────────────────────────────────

help:
	@echo "yaml-glib - GLib/GObject-based YAML library"
	@echo ""
	@echo "Build targets:"
	@echo "  all          - Build library and examples (default)"
	@echo "  lib          - Build static and shared libraries"
	@echo "  gir          - Generate GObject Introspection data"
	@echo "  examples     - Build example programs"
	@echo "  test         - Build and run the test suite"
	@echo "  install      - Install to PREFIX ($(PREFIX))"
	@echo "  uninstall    - Remove installed files"
	@echo "  clean        - Remove build artifacts for current build type"
	@echo "  clean-all    - Remove all build directories"
	@echo ""
	@echo "Build options (set on command line):"
	@echo "  DEBUG=1          - Enable debug build (-g -O0)"
	@echo "  ASAN=1           - Enable AddressSanitizer"
	@echo "  UBSAN=1          - Enable UndefinedBehaviorSanitizer"
	@echo "  BUILD_GIR=1      - Enable GObject Introspection generation"
	@echo "  BUILD_TESTS=0    - Disable test building"
	@echo "  BUILD_EXAMPLES=0 - Disable example building"
	@echo "  PREFIX=path      - Set installation prefix (default: /usr/local)"
	@echo ""
	@echo "Cross-compilation:"
	@echo "  WINDOWS=1        - Cross-compile for Windows x64 (MinGW)"
	@echo "  LINUX_ARM64=1    - Cross-compile for Linux ARM64"
	@echo "  CROSS=<prefix>   - Explicit cross-compiler prefix"
	@echo ""
	@echo "Utility targets:"
	@echo "  install-deps  - Install build dependencies (auto-detects distro)"
	@echo "  check-deps    - Check for required pkg-config dependencies"
	@echo "  show-config   - Show current build configuration"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Library:  lib$(LIB_NAME).so.$(VERSION)"

# ── Dependency tracking (incremental builds) ─────────────────────────
# Skip when cleaning to avoid regenerating headers that clean will delete

ALL_DEPS := $(LIB_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

ifeq ($(filter clean clean-all,$(MAKECMDGOALS)),)
-include $(ALL_DEPS)
endif
