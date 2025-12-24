# Building yaml-glib

This document describes how to build yaml-glib from source, its dependencies, and available build targets.

## Dependencies

yaml-glib requires the following libraries:

| Library | Package (Fedora) | Package (Debian/Ubuntu) | Purpose |
|---------|------------------|-------------------------|---------|
| GLib 2.0 | glib2-devel | libglib2.0-dev | Core utilities and data structures |
| GObject 2.0 | (included with glib2) | (included with glib) | Object system |
| GIO 2.0 | (included with glib2) | (included with glib) | I/O operations |
| libyaml | libyaml-devel | libyaml-dev | YAML parsing and emitting |
| JSON-GLib 1.0 | json-glib-devel | libjson-glib-dev | JSON interoperability |

### Installing Dependencies

**Fedora:**
```bash
sudo dnf install gcc make glib2-devel libyaml-devel json-glib-devel
```

**Debian/Ubuntu:**
```bash
sudo apt install gcc make libglib2.0-dev libyaml-dev libjson-glib-dev
```

**Arch Linux:**
```bash
sudo pacman -S gcc make glib2 libyaml json-glib
```

### Verifying Dependencies

Use pkg-config to verify all dependencies are installed:

```bash
pkg-config --exists glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0 && echo "All dependencies found"
```

To see the compiler flags:
```bash
pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0
```

To see the linker flags:
```bash
pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0
```

## Build Configuration

yaml-glib uses GNU Make for building. The Makefile supports several configuration variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `CC` | gcc | C compiler to use |
| `PREFIX` | /usr/local | Installation prefix |
| `DESTDIR` | (empty) | Staging directory for packaging |

### Compiler Flags

The default CFLAGS include:
- `-std=gnu89` - GNU C89 standard
- `-Wall -Wextra` - Enable warnings
- `-g` - Debug symbols
- `-fPIC` - Position-independent code (required for shared library)
- `-I./src` - Include path for headers

## Building

### Basic Build

```bash
git clone https://gitlab.com/your-repo/yaml-glib.git
cd yaml-glib
make
```

This builds:
- `build/libyaml-glib.so.1.0.0` - Shared library
- `build/libyaml-glib.so.1` - Soname symlink
- `build/libyaml-glib.so` - Development symlink
- `build/libyaml-glib.a` - Static library

### Build Targets

| Target | Description |
|--------|-------------|
| `make` or `make all` | Build shared and static libraries |
| `make tests` | Build test executables |
| `make check` or `make run-tests` | Build and run all tests |
| `make clean` | Remove build artifacts |
| `make install` | Install to PREFIX |
| `make uninstall` | Remove installed files |

### Building Tests

```bash
make tests
```

This compiles all `test_*.c` files in the `tests/` directory into separate executables in `build/`.

### Running Tests

```bash
make check
```

Or equivalently:
```bash
make run-tests
```

This runs all test executables and reports pass/fail status:
```
Running build/test_builder...
Running build/test_generator...
Running build/test_mapping...
...
All tests passed!
```

## Installation

### System-Wide Installation

```bash
sudo make install
```

Default installation paths:
- Headers: `/usr/local/include/yaml-glib/`
- Libraries: `/usr/local/lib/`
- pkg-config: `/usr/local/lib/pkgconfig/`

After installation, run ldconfig to update the library cache:
```bash
sudo ldconfig
```

### Custom Installation Prefix

```bash
make PREFIX=/opt/yaml-glib install
```

### Staged Installation (for Packaging)

```bash
make DESTDIR=/tmp/yaml-glib-pkg install
```

This installs to `/tmp/yaml-glib-pkg/usr/local/...` which is useful for building packages.

### Uninstallation

```bash
sudo make uninstall
```

## Library Versioning

yaml-glib uses standard library versioning:

| Variable | Value | Description |
|----------|-------|-------------|
| `LIB_MAJOR` | 1 | Major version (ABI breaking changes) |
| `LIB_MINOR` | 0 | Minor version (new features) |
| `LIB_PATCH` | 0 | Patch version (bug fixes) |

The shared library is versioned as:
- `libyaml-glib.so.1.0.0` - Real name with full version
- `libyaml-glib.so.1` - Soname (for runtime linking)
- `libyaml-glib.so` - Linker name (for development)

## Using yaml-glib in Your Projects

### With Installed Library

**Compiler flags:**
```bash
gcc -I/usr/local/include/yaml-glib \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0` \
    -c myprogram.c
```

**Linker flags:**
```bash
gcc -o myprogram myprogram.o \
    -L/usr/local/lib -lyaml-glib \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`
```

### With Uninstalled Library (Development)

When developing against an uninstalled yaml-glib:

```bash
gcc -I/path/to/yaml-glib/src \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0` \
    -c myprogram.c

gcc -o myprogram myprogram.o \
    -L/path/to/yaml-glib/build -lyaml-glib \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`

# Set library path when running
LD_LIBRARY_PATH=/path/to/yaml-glib/build ./myprogram
```

### Example Makefile

```makefile
CC = gcc
CFLAGS = -std=gnu89 -Wall -Wextra -g \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`
LDFLAGS = -lyaml-glib \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`

myprogram: myprogram.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
```

## Static Linking

To link statically against yaml-glib:

```bash
gcc -o myprogram myprogram.c /usr/local/lib/libyaml-glib.a \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`
```

Note: Static linking still requires shared libraries for GLib, libyaml, and JSON-GLib unless you statically link those as well.

## Troubleshooting

### Library Not Found at Runtime

If you get "libyaml-glib.so: cannot open shared object file":

1. Ensure the library path is in the linker cache:
   ```bash
   sudo ldconfig /usr/local/lib
   ```

2. Or set LD_LIBRARY_PATH:
   ```bash
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   ```

### Header Not Found

If you get "yaml-glib.h: No such file or directory":

1. Check the include path in your CFLAGS
2. Verify installation:
   ```bash
   ls /usr/local/include/yaml-glib/
   ```

### pkg-config Not Finding Packages

Ensure PKG_CONFIG_PATH includes the right directories:
```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

## Development Build

For development with debugging enabled:

```bash
make CFLAGS="-std=gnu89 -Wall -Wextra -g -O0 -fPIC -I./src \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`"
```

For a release build with optimizations:

```bash
make CFLAGS="-std=gnu89 -Wall -Wextra -O2 -fPIC -I./src \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`"
```

## Source Files

The library source files in `src/`:

| File | Description |
|------|-------------|
| yaml-types.h | Type definitions, enums, error codes |
| yaml-node.{h,c} | YamlNode boxed type |
| yaml-mapping.{h,c} | YamlMapping boxed type |
| yaml-sequence.{h,c} | YamlSequence boxed type |
| yaml-document.{h,c} | YamlDocument GObject |
| yaml-parser.{h,c} | YamlParser GObject |
| yaml-builder.{h,c} | YamlBuilder GObject |
| yaml-generator.{h,c} | YamlGenerator GObject |
| yaml-serializable.{h,c} | YamlSerializable interface |
| yaml-gobject.{h,c} | GObject serialization utilities |
| yaml-schema.{h,c} | YamlSchema validation |
| yaml-private.h | Private implementation details |
| yaml-glib.h | Main include file |

## See Also

- [Getting Started](getting-started.md) - Quick start guide
- [API Reference](api/types.md) - Full API documentation
