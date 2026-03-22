# config.mk - yaml-glib Configuration
# GLib/GObject-based YAML parsing and generation library
#
# Copyright (C) 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# This file contains all configurable build options.
# Override any variable on the command line:
#   make DEBUG=1
#   make PREFIX=/usr/local

# ── Project info ──────────────────────────────────────────────────────
PROJECT_NAME := yaml-glib
VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_MICRO := 0
VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)
API_VERSION := 1.0

# ── Installation directories ─────────────────────────────────────────
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Auto-detect lib vs lib64 for 64-bit systems (Fedora, RHEL, SUSE, etc.)
# Override with: make LIBDIR=/usr/local/lib
LIBDIR_SUFFIX := $(shell if [ -d /usr/lib64 ]; then echo lib64; else echo lib; fi)
LIBDIR ?= $(PREFIX)/$(LIBDIR_SUFFIX)

INCLUDEDIR ?= $(PREFIX)/include
DATADIR ?= $(PREFIX)/share
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
GIRDIR ?= $(DATADIR)/gir-1.0
TYPELIBDIR ?= $(LIBDIR)/girepository-1.0

# ── Build directories ────────────────────────────────────────────────
BUILDDIR := build
OBJDIR_DEBUG := $(BUILDDIR)/debug/obj
OBJDIR_RELEASE := $(BUILDDIR)/release/obj
BINDIR_DEBUG := $(BUILDDIR)/debug
BINDIR_RELEASE := $(BUILDDIR)/release

# ── Build options (0 or 1) ───────────────────────────────────────────
DEBUG ?= 0
ASAN ?= 0
UBSAN ?= 0
BUILD_GIR ?= 0
BUILD_TESTS ?= 1
BUILD_EXAMPLES ?= 1

# Select build directories based on DEBUG
ifeq ($(DEBUG),1)
    OBJDIR := $(OBJDIR_DEBUG)
    OUTDIR := $(BINDIR_DEBUG)
    BUILD_TYPE := debug
else
    OBJDIR := $(OBJDIR_RELEASE)
    OUTDIR := $(BINDIR_RELEASE)
    BUILD_TYPE := release
endif

# ── Cross-Compilation ────────────────────────────────────────────────
# Usage:
#   make WINDOWS=1            # Cross-compile for Windows x64
#   make LINUX_ARM64=1        # Cross-compile for Linux ARM64
#   make CROSS=<prefix>       # Explicit cross-compiler prefix

WINDOWS ?= 0
LINUX_ARM64 ?= 0
CROSS ?=

# ARM64 sysroot location (Fedora's sysroot-aarch64-fc41-glibc package)
ARM64_SYSROOT ?= /usr/aarch64-redhat-linux/sys-root/fc41

# Set CROSS based on convenience variables
ifeq ($(WINDOWS),1)
    CROSS := x86_64-w64-mingw32
endif

ifeq ($(LINUX_ARM64),1)
    CROSS := aarch64-linux-gnu
endif

# ── Compiler and tools ───────────────────────────────────────────────
ifneq ($(CROSS),)
    CC := $(CROSS)-gcc
    AR := $(CROSS)-ar

    ifeq ($(CROSS),x86_64-w64-mingw32)
        PKG_CONFIG := $(CROSS)-pkg-config
        TARGET_PLATFORM := windows
        EXE_EXT := .exe
    else ifeq ($(CROSS),aarch64-linux-gnu)
        PKG_CONFIG_SYSROOT_DIR := $(ARM64_SYSROOT)
        PKG_CONFIG_PATH := $(ARM64_SYSROOT)/usr/lib64/pkgconfig:$(ARM64_SYSROOT)/usr/share/pkgconfig
        PKG_CONFIG := PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config
        SYSROOT_FLAGS := --sysroot=$(ARM64_SYSROOT)
        TARGET_PLATFORM := linux
        EXE_EXT :=
    else
        PKG_CONFIG := pkg-config
        TARGET_PLATFORM := linux
        EXE_EXT :=
    endif
else
    CC := gcc
    AR := ar
    PKG_CONFIG ?= pkg-config
    TARGET_PLATFORM := linux
    SYSROOT_FLAGS :=
    EXE_EXT :=
endif

GIR_SCANNER ?= g-ir-scanner
GIR_COMPILER ?= g-ir-compiler
INSTALL := install
INSTALL_PROGRAM := $(INSTALL) -m 755
INSTALL_DATA := $(INSTALL) -m 644
MKDIR_P := mkdir -p

# ── C standard and warnings ─────────────────────────────────────────
CSTD := -std=gnu89
WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wformat=2 -Wshadow

# ── Base compiler flags ─────────────────────────────────────────────
CFLAGS_BASE := $(CSTD) $(WARNINGS)
CFLAGS_BASE += -fPIC
CFLAGS_BASE += -DYAML_GLIB_VERSION=\"$(VERSION)\"
CFLAGS_BASE += -DYAML_GLIB_VERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS_BASE += -DYAML_GLIB_VERSION_MINOR=$(VERSION_MINOR)
CFLAGS_BASE += -DYAML_GLIB_VERSION_MICRO=$(VERSION_MICRO)
CFLAGS_BASE += -DG_LOG_DOMAIN=\"YamlGlib\"

# ── Debug/Release flags ─────────────────────────────────────────────
ifeq ($(DEBUG),1)
    CFLAGS_BUILD := -g -O0 -DDEBUG
else
    CFLAGS_BUILD := -O2 -DNDEBUG
endif

# AddressSanitizer
ifeq ($(ASAN),1)
    CFLAGS_BUILD += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS_ASAN := -fsanitize=address
endif

# UndefinedBehaviorSanitizer
ifeq ($(UBSAN),1)
    CFLAGS_BUILD += -fsanitize=undefined
    LDFLAGS_UBSAN := -fsanitize=undefined
endif

# ── Dependencies (pkg-config) ───────────────────────────────────────
DEPS_REQUIRED := glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0

# Check for required dependencies
define check_dep
$(if $(shell $(PKG_CONFIG) --exists $(1) && echo yes),,$(error Missing dependency: $(1)))
endef

# Get flags from pkg-config
CFLAGS_DEPS := $(shell $(PKG_CONFIG) --cflags $(DEPS_REQUIRED) 2>/dev/null)
LDFLAGS_DEPS := $(shell $(PKG_CONFIG) --libs $(DEPS_REQUIRED) 2>/dev/null)

# ── Include paths ────────────────────────────────────────────────────
CFLAGS_INC := -I. -Isrc $(SYSROOT_FLAGS)

# ── Combine all CFLAGS ──────────────────────────────────────────────
CFLAGS := $(CFLAGS_BASE) $(CFLAGS_BUILD) $(CFLAGS_INC) $(CFLAGS_DEPS)

# ── Linker flags ────────────────────────────────────────────────────
LDFLAGS := $(LDFLAGS_DEPS) $(LDFLAGS_ASAN) $(LDFLAGS_UBSAN)

# ── Platform-specific library configuration ─────────────────────────
LIB_NAME := yaml-glib-$(API_VERSION)

ifeq ($(TARGET_PLATFORM),windows)
    LIB_STATIC := lib$(LIB_NAME).a
    LIB_SHARED := $(LIB_NAME).dll
    LIB_IMPORT := lib$(LIB_NAME).dll.a
    LIB_SHARED_FULL := $(LIB_SHARED)
    LIB_SHARED_MAJOR := $(LIB_SHARED)
    LDFLAGS_SHARED := -shared -Wl,--out-implib,$(OUTDIR)/$(LIB_IMPORT)
else
    LIB_STATIC := lib$(LIB_NAME).a
    LIB_SHARED := lib$(LIB_NAME).so
    LIB_SHARED_FULL := lib$(LIB_NAME).so.$(VERSION)
    LIB_SHARED_MAJOR := lib$(LIB_NAME).so.$(VERSION_MAJOR)
    LDFLAGS_SHARED := -shared -Wl,-soname,$(LIB_SHARED_MAJOR)
endif

# ── GIR settings ────────────────────────────────────────────────────
GIR_NAMESPACE := YamlGlib
GIR_VERSION := $(API_VERSION)
GIR_FILE := $(GIR_NAMESPACE)-$(GIR_VERSION).gir
TYPELIB_FILE := $(GIR_NAMESPACE)-$(GIR_VERSION).typelib

# ── Test framework ───────────────────────────────────────────────────
TEST_CFLAGS := $(CFLAGS)
TEST_LDFLAGS := $(LDFLAGS) -L$(OUTDIR) -l$(LIB_NAME) -Wl,-rpath,$(OUTDIR)

# ── Print configuration ─────────────────────────────────────────────
.PHONY: show-config
show-config:
	@echo "yaml-glib Build Configuration"
	@echo "=============================="
	@echo "Version:        $(VERSION)"
	@echo "API Version:    $(API_VERSION)"
	@echo "Build type:     $(BUILD_TYPE)"
	@echo "Compiler:       $(CC)"
	@echo "CFLAGS:         $(CFLAGS)"
	@echo "LDFLAGS:        $(LDFLAGS)"
	@echo "PREFIX:         $(PREFIX)"
	@echo "LIBDIR:         $(LIBDIR)"
	@echo "DEBUG:          $(DEBUG)"
	@echo "ASAN:           $(ASAN)"
	@echo "UBSAN:          $(UBSAN)"
	@echo "BUILD_GIR:      $(BUILD_GIR)"
	@echo "BUILD_TESTS:    $(BUILD_TESTS)"
	@echo "BUILD_EXAMPLES: $(BUILD_EXAMPLES)"
ifneq ($(CROSS),)
	@echo "CROSS:          $(CROSS)"
	@echo "TARGET_PLATFORM:$(TARGET_PLATFORM)"
endif

# ── Package names for install-deps ───────────────────────────────────

# Fedora / RHEL / CentOS
FEDORA_DEPS_TOOLS := gcc make pkgconf-pkg-config
FEDORA_DEPS_REQUIRED := glib2-devel libyaml-devel json-glib-devel
FEDORA_DEPS_GIR := gobject-introspection-devel

# Ubuntu / Debian
UBUNTU_DEPS_TOOLS := gcc make pkg-config
UBUNTU_DEPS_REQUIRED := libglib2.0-dev libyaml-dev libjson-glib-dev
UBUNTU_DEPS_GIR := gobject-introspection libgirepository1.0-dev

# Arch Linux
ARCH_DEPS_TOOLS := gcc make pkgconf
ARCH_DEPS_REQUIRED := glib2 libyaml json-glib
ARCH_DEPS_GIR := gobject-introspection

# Detect distro and install
.PHONY: install-deps
install-deps:
	@if command -v dnf >/dev/null 2>&1; then \
		echo "Detected Fedora/RHEL (dnf)"; \
		sudo dnf install -y $(FEDORA_DEPS_TOOLS) $(FEDORA_DEPS_REQUIRED) \
			$(if $(filter 1,$(BUILD_GIR)),$(FEDORA_DEPS_GIR)); \
	elif command -v apt-get >/dev/null 2>&1; then \
		echo "Detected Ubuntu/Debian (apt)"; \
		sudo apt-get install -y $(UBUNTU_DEPS_TOOLS) $(UBUNTU_DEPS_REQUIRED) \
			$(if $(filter 1,$(BUILD_GIR)),$(UBUNTU_DEPS_GIR)); \
	elif command -v pacman >/dev/null 2>&1; then \
		echo "Detected Arch Linux (pacman)"; \
		sudo pacman -S --needed $(ARCH_DEPS_TOOLS) $(ARCH_DEPS_REQUIRED) \
			$(if $(filter 1,$(BUILD_GIR)),$(ARCH_DEPS_GIR)); \
	else \
		echo "Unknown distro. Required packages:"; \
		echo "  glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0"; \
		exit 1; \
	fi
