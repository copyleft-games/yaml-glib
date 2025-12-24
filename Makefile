# yaml-glib Makefile
#
# Copyright 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Build libyaml-glib.so and associated tests

CC = gcc
CFLAGS = -std=gnu89 -Wall -Wextra -g -fPIC -I./src \
	`pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`
LDFLAGS = `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`

# Library versioning
LIB_MAJOR = 1
LIB_MINOR = 0
LIB_PATCH = 0
LIB_VERSION = $(LIB_MAJOR).$(LIB_MINOR).$(LIB_PATCH)

# Directories
SRCDIR = src
TESTDIR = tests
BUILDDIR = build

# Source files
SOURCES = \
	$(SRCDIR)/yaml-node.c \
	$(SRCDIR)/yaml-mapping.c \
	$(SRCDIR)/yaml-sequence.c \
	$(SRCDIR)/yaml-document.c \
	$(SRCDIR)/yaml-parser.c \
	$(SRCDIR)/yaml-builder.c \
	$(SRCDIR)/yaml-generator.c \
	$(SRCDIR)/yaml-serializable.c \
	$(SRCDIR)/yaml-gobject.c \
	$(SRCDIR)/yaml-schema.c

# Header files
HEADERS = \
	$(SRCDIR)/yaml-glib.h \
	$(SRCDIR)/yaml-types.h \
	$(SRCDIR)/yaml-node.h \
	$(SRCDIR)/yaml-mapping.h \
	$(SRCDIR)/yaml-sequence.h \
	$(SRCDIR)/yaml-document.h \
	$(SRCDIR)/yaml-parser.h \
	$(SRCDIR)/yaml-builder.h \
	$(SRCDIR)/yaml-generator.h \
	$(SRCDIR)/yaml-serializable.h \
	$(SRCDIR)/yaml-gobject.h \
	$(SRCDIR)/yaml-schema.h \
	$(SRCDIR)/yaml-private.h

# Object files
OBJECTS = \
	$(BUILDDIR)/yaml-node.o \
	$(BUILDDIR)/yaml-mapping.o \
	$(BUILDDIR)/yaml-sequence.o \
	$(BUILDDIR)/yaml-document.o \
	$(BUILDDIR)/yaml-parser.o \
	$(BUILDDIR)/yaml-builder.o \
	$(BUILDDIR)/yaml-generator.o \
	$(BUILDDIR)/yaml-serializable.o \
	$(BUILDDIR)/yaml-gobject.o \
	$(BUILDDIR)/yaml-schema.o

# Library names
LIBNAME = libyaml-glib
SONAME = $(LIBNAME).so.$(LIB_MAJOR)
REALNAME = $(LIBNAME).so.$(LIB_VERSION)
STATICLIB = $(BUILDDIR)/$(LIBNAME).a
SHAREDLIB = $(BUILDDIR)/$(REALNAME)

# Default target
all: $(BUILDDIR) $(SHAREDLIB) $(STATICLIB)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Generic rule for object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Shared library with versioning
$(SHAREDLIB): $(OBJECTS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ $(LDFLAGS)
	ln -sf $(REALNAME) $(BUILDDIR)/$(SONAME)
	ln -sf $(SONAME) $(BUILDDIR)/$(LIBNAME).so

# Static library
$(STATICLIB): $(OBJECTS)
	ar rcs $@ $^

# Install target
PREFIX ?= /usr/local
INCLUDEDIR = $(PREFIX)/include/yaml-glib
LIBDIR = $(PREFIX)/lib
PKGCONFIGDIR = $(LIBDIR)/pkgconfig

install: $(SHAREDLIB) $(STATICLIB)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(PKGCONFIGDIR)
	install -m 644 $(SRCDIR)/yaml-glib.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-types.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-node.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-mapping.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-sequence.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-document.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-parser.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-builder.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-generator.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-serializable.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-gobject.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 644 $(SRCDIR)/yaml-schema.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 755 $(SHAREDLIB) $(DESTDIR)$(LIBDIR)/
	install -m 644 $(STATICLIB) $(DESTDIR)$(LIBDIR)/
	ln -sf $(REALNAME) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/$(LIBNAME).so
	ldconfig -n $(DESTDIR)$(LIBDIR) || true

# Uninstall target
uninstall:
	rm -rf $(DESTDIR)$(INCLUDEDIR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBNAME).so*
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBNAME).a
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/yaml-glib.pc

# Test targets
TEST_SOURCES = $(wildcard $(TESTDIR)/test_*.c)
TEST_OBJECTS = $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%.o,$(TEST_SOURCES))
TEST_BINARIES = $(patsubst $(TESTDIR)/%.c,$(BUILDDIR)/%,$(TEST_SOURCES))

tests: $(BUILDDIR) $(STATICLIB) $(TEST_BINARIES)

$(BUILDDIR)/test_%: $(BUILDDIR)/test_%.o $(STATICLIB)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/test_%.o: $(TESTDIR)/test_%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
check: tests
	@for test in $(BUILDDIR)/test_*; do \
		if [ -x "$$test" ]; then \
			echo "Running $$test..."; \
			$$test || exit 1; \
		fi \
	done
	@echo "All tests passed!"

run-tests: check

# Example targets
EXAMPLEDIR = examples
EXAMPLE_SOURCES = \
	$(EXAMPLEDIR)/yaml-print.c \
	$(EXAMPLEDIR)/yaml-to-json.c \
	$(EXAMPLEDIR)/json-to-yaml.c

EXAMPLE_BINARIES = \
	$(BUILDDIR)/examples/yaml-print \
	$(BUILDDIR)/examples/yaml-to-json \
	$(BUILDDIR)/examples/json-to-yaml

examples: $(BUILDDIR)/examples $(STATICLIB) $(EXAMPLE_BINARIES)

$(BUILDDIR)/examples:
	mkdir -p $(BUILDDIR)/examples

$(BUILDDIR)/examples/%: $(EXAMPLEDIR)/%.c $(STATICLIB)
	$(CC) $(CFLAGS) -o $@ $< $(STATICLIB) $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BUILDDIR)

# Dependency on all headers for safety
$(OBJECTS): $(HEADERS)

.PHONY: all clean install uninstall tests check run-tests examples
