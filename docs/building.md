# Building yaml-glib

This document describes how to build yaml-glib from source, its dependencies, and available build targets.

## Build System Architecture

yaml-glib uses a three-file GNU Make build system:

| File | Purpose |
|------|---------|
| `config.mk` | All configurable options: project info, versioning, install directories, build options, compiler/tools, warning flags, dependencies, cross-compilation |
| `rules.mk` | All build rules: object compilation, library creation, GIR generation, generated files, directory creation, clean, install, uninstall |
| `Makefile` | Orchestration only: source/header lists, object mappings, phony targets, test runner |

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

The build system can auto-detect your distro and install dependencies:

```bash
make install-deps
```

Or install manually:

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

```bash
make check-deps
```

This checks each required pkg-config dependency and reports versions:

```
Checking dependencies...
  glib-2.0: OK (2.86.4)
  gobject-2.0: OK (2.86.4)
  gio-2.0: OK (2.86.4)
  yaml-0.1: OK (0.2.5)
  json-glib-1.0: OK (1.10.8)
```

## Building

### Basic Build

```bash
git clone https://gitlab.com/copyleft-games/yaml-glib.git
cd yaml-glib
make
```

This builds (in `build/release/`):
- `libyaml-glib-1.0.so.1.0.0` - Shared library
- `libyaml-glib-1.0.so.1` - Soname symlink
- `libyaml-glib-1.0.so` - Development symlink
- `libyaml-glib-1.0.a` - Static library
- `yaml-glib-1.0.pc` - pkg-config file
- `examples/` - Example binaries

### Debug Build

```bash
make DEBUG=1
```

Debug builds go to `build/debug/` with `-g -O0 -DDEBUG` flags.

### Build with Sanitizers

```bash
make DEBUG=1 ASAN=1       # AddressSanitizer
make DEBUG=1 UBSAN=1      # UndefinedBehaviorSanitizer
make DEBUG=1 ASAN=1 UBSAN=1  # Both
```

### View Configuration

```bash
make show-config
```

### Build Targets

| Target | Description |
|--------|-------------|
| `make` or `make all` | Build library and examples (default) |
| `make lib` | Build static and shared libraries only |
| `make test` | Build and run all tests |
| `make examples` | Build example programs |
| `make gir` | Generate GObject Introspection data |
| `make install` | Install to PREFIX |
| `make uninstall` | Remove installed files |
| `make clean` | Remove build artifacts for current build type |
| `make clean-all` | Remove all build directories |
| `make help` | Show all targets and options |

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `DEBUG` | 0 | Debug build (`-g -O0`) |
| `ASAN` | 0 | AddressSanitizer |
| `UBSAN` | 0 | UndefinedBehaviorSanitizer |
| `BUILD_GIR` | 0 | GObject Introspection generation |
| `BUILD_TESTS` | 1 | Build test executables |
| `BUILD_EXAMPLES` | 1 | Build example programs |
| `PREFIX` | /usr/local | Installation prefix |

## Testing

### Running Tests

```bash
make test
```

This compiles all `test-*.c` files in `tests/` and runs them:

```
Running tests...
  Running test-builder...
    PASS
  Running test-document...
    PASS
  Running test-gobject...
    PASS
  Running test-json...
    PASS
  Running test-node...
    PASS
  Running test-parser...
    PASS
  Running test-schema...
    PASS

Results: 7/7 passed
All tests passed
```

### Running Tests with Sanitizers

```bash
make DEBUG=1 ASAN=1 test
```

### Test Files

| Test File | Coverage |
|-----------|----------|
| `test-node.c` | YamlNode, YamlMapping, YamlSequence (scalar types, edge cases, copy, equal, hash, seal, foreach) |
| `test-parser.c` | YamlParser (loading, multi-document, immutable, reset, file I/O, error handling, null values) |
| `test-builder.c` | YamlBuilder + YamlGenerator (all scalar types, steal/dup root, document, file output, explicit markers, roundtrip) |
| `test-document.c` | YamlDocument (root management, seal, version directives, tag directives, JSON interop) |
| `test-gobject.c` | yaml-gobject + YamlSerializable (serialize/deserialize GObjects, from_data/to_data, roundtrip, error cases) |
| `test-json.c` | JSON-GLib interoperability (YAML-to-JSON, JSON-to-YAML, nested, document conversion) |
| `test-schema.c` | YamlSchema (type validation, required/optional properties, length, enum, pattern, numeric range, nested) |

## Installation

### System-Wide Installation

```bash
sudo make install
```

Default installation paths:
- Headers: `/usr/local/include/yaml-glib/`
- Libraries: `/usr/local/lib64/` (or `lib/` on non-64-bit systems)
- pkg-config: `/usr/local/lib64/pkgconfig/`

After installation, run ldconfig to update the library cache:
```bash
sudo ldconfig
```

### Granular Install Targets

| Target | Description |
|--------|-------------|
| `make install-lib` | Install static and shared libraries |
| `make install-headers` | Install public header files |
| `make install-pc` | Install pkg-config file |
| `make install-gir` | Install GIR and typelib files |

### Custom Installation Prefix

```bash
make PREFIX=/opt/yaml-glib install
```

### Staged Installation (for Packaging)

```bash
make DESTDIR=/tmp/yaml-glib-pkg install
```

### Uninstallation

```bash
sudo make uninstall
```

## Library Versioning

yaml-glib uses standard library versioning:

| Variable | Value | Description |
|----------|-------|-------------|
| `VERSION_MAJOR` | 1 | Major version (ABI breaking changes) |
| `VERSION_MINOR` | 0 | Minor version (new features) |
| `VERSION_MICRO` | 0 | Micro/patch version (bug fixes) |
| `API_VERSION` | 1.0 | API version (used in library and pkg-config naming) |

The shared library is versioned as:
- `libyaml-glib-1.0.so.1.0.0` - Real name with full version
- `libyaml-glib-1.0.so.1` - Soname (for runtime linking)
- `libyaml-glib-1.0.so` - Linker name (for development)

## Using yaml-glib in Your Projects

### With pkg-config (Recommended)

After installation, use the `yaml-glib-1.0` pkg-config module:

```bash
gcc -o myprogram myprogram.c `pkg-config --cflags --libs yaml-glib-1.0`
```

### Example Makefile

```makefile
CC = gcc
CFLAGS = -std=gnu89 -Wall -Wextra `pkg-config --cflags yaml-glib-1.0`
LDFLAGS = `pkg-config --libs yaml-glib-1.0`

myprogram: myprogram.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
```

### With Uninstalled Library (Development)

When developing against an uninstalled yaml-glib:

```bash
gcc -Ipath/to/yaml-glib/src \
    `pkg-config --cflags glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0` \
    -o myprogram myprogram.c \
    -Lpath/to/yaml-glib/build/release -lyaml-glib-1.0 \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`

LD_LIBRARY_PATH=path/to/yaml-glib/build/release ./myprogram
```

## Static Linking

To link statically against yaml-glib:

```bash
gcc -o myprogram myprogram.c path/to/libyaml-glib-1.0.a \
    `pkg-config --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0`
```

Note: Static linking still requires shared libraries for GLib, libyaml, and JSON-GLib unless you statically link those as well.

## Source Files

The library source files in `src/`:

| File | Description |
|------|-------------|
| yaml-glib.h | Main umbrella include file |
| yaml-types.h | Type definitions, enums, error codes |
| yaml-version.h.in | Version header template (generates yaml-version.h) |
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

## Cross-Compilation

yaml-glib supports cross-compilation for Windows (MinGW-w64) and Linux ARM64.

### Windows Cross-Compilation

**Dependencies (Fedora):**
```bash
sudo dnf install mingw64-gcc mingw64-glib2 mingw64-json-glib mingw64-libyaml
```

**Build:**
```bash
make WINDOWS=1
```

**Output:** `build/release/yaml-glib.dll`, `build/release/libyaml-glib-1.0.dll.a`, `build/release/libyaml-glib-1.0.a`

**Running tests with Wine:**
```bash
cd build/release
WINEPATH="/usr/x86_64-w64-mingw32/sys-root/mingw/bin" wine test-node.exe
```

### Linux ARM64 Cross-Compilation

**Dependencies (Fedora):**
```bash
sudo dnf install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu \
    sysroot-aarch64-fc41-glibc qemu-user-static cpio
sudo ./scripts/setup-arm64-sysroot.sh
```

**Build and test:**
```bash
make LINUX_ARM64=1
make LINUX_ARM64=1 test
```

Tests run automatically via QEMU user-mode emulation.

## Troubleshooting

### Library Not Found at Runtime

```bash
sudo ldconfig /usr/local/lib64
```

Or set LD_LIBRARY_PATH:
```bash
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
```

### Header Not Found

Verify installation:
```bash
ls /usr/local/include/yaml-glib/
```

### pkg-config Not Finding yaml-glib

```bash
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH
pkg-config --cflags --libs yaml-glib-1.0
```

## See Also

- [Getting Started](getting-started.md) - Quick start guide
- [API Reference](api/types.md) - Full API documentation
