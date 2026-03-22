# yaml-glib

A GLib/GObject-based YAML library for C, providing a high-level API for parsing, building, and generating YAML documents.

## Features

- **Parse** YAML from files, strings, or streams (sync and async)
- **Build** YAML programmatically with a fluent builder API
- **Generate** YAML output with configurable formatting
- **Validate** YAML against schemas with type and constraint checking
- **Convert** bidirectionally between YAML and JSON-GLib
- **Serialize** GObjects to/from YAML with custom serialization support
- **Reference-counted** boxed types with GLib memory management patterns
- **Multi-document** YAML stream support
- **Anchors and aliases** for YAML references

## Requirements

Install build dependencies automatically:

```bash
make install-deps
```

Or install manually:

### Fedora

```bash
sudo dnf install gcc make glib2-devel libyaml-devel json-glib-devel
```

### Debian/Ubuntu

```bash
sudo apt install gcc make libglib2.0-dev libyaml-dev libjson-glib-dev
```

### Arch Linux

```bash
sudo pacman -S gcc make glib2 libyaml json-glib
```

## Quick Start

### Build

```bash
git clone <repository-url>
cd yaml-glib
make
```

### Build with Debug Symbols

```bash
make DEBUG=1
```

### Run Tests

```bash
make test
```

### Check Dependencies

```bash
make check-deps
```

### Install

```bash
sudo make install
sudo ldconfig
```

### Cross-Compile for Windows

```bash
make WINDOWS=1
```

### Cross-Compile for Linux ARM64

```bash
make LINUX_ARM64=1
make LINUX_ARM64=1 test
```

## Build System

yaml-glib uses a three-file GNU Make build system:

| File | Purpose |
|------|---------|
| `config.mk` | Configuration: project info, install dirs, build options, compiler flags, dependencies |
| `rules.mk` | Build rules: compilation, linking, GIR generation, install, clean |
| `Makefile` | Orchestration: source lists, object mappings, phony targets |

### Build Targets

| Target | Description |
|--------|-------------|
| `make` or `make all` | Build library and examples (release) |
| `make lib` | Build static and shared libraries only |
| `make test` | Build and run all tests |
| `make examples` | Build example programs |
| `make gir` | Generate GObject Introspection data (requires `BUILD_GIR=1`) |
| `make install` | Install to PREFIX |
| `make uninstall` | Remove installed files |
| `make clean` | Remove build artifacts for current build type |
| `make clean-all` | Remove all build artifacts |
| `make show-config` | Display build configuration |
| `make check-deps` | Check for required pkg-config dependencies |
| `make install-deps` | Install build dependencies (auto-detects distro) |
| `make help` | Show all available targets and options |

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `DEBUG=1` | 0 | Enable debug build (`-g -O0`) |
| `ASAN=1` | 0 | Enable AddressSanitizer |
| `UBSAN=1` | 0 | Enable UndefinedBehaviorSanitizer |
| `BUILD_GIR=1` | 0 | Enable GObject Introspection generation |
| `BUILD_TESTS=0` | 1 | Disable test building |
| `BUILD_EXAMPLES=0` | 1 | Disable example building |
| `PREFIX=path` | /usr/local | Set installation prefix |
| `WINDOWS=1` | 0 | Cross-compile for Windows x64 (MinGW) |
| `LINUX_ARM64=1` | 0 | Cross-compile for Linux ARM64 |

### Build Output

Release builds go to `build/release/`, debug builds to `build/debug/`:

```
build/release/
├── libyaml-glib-1.0.a           # Static library
├── libyaml-glib-1.0.so.1.0.0    # Shared library
├── libyaml-glib-1.0.so.1        # Soname symlink
├── libyaml-glib-1.0.so          # Development symlink
├── yaml-glib-1.0.pc             # pkg-config file
├── obj/                          # Object files and .d dependency files
├── examples/                     # Example binaries
└── test-*                        # Test binaries
```

## Quick Example

Parse a YAML file and read values:

```c
#include <yaml-glib/yaml-glib.h>

int
main(int argc, char *argv[])
{
    g_autoptr(YamlParser) parser = yaml_parser_new();
    g_autoptr(GError) error = NULL;

    if (!yaml_parser_load_from_file(parser, "config.yaml", &error))
    {
        g_printerr("Error: %s\n", error->message);
        return 1;
    }

    YamlNode *root = yaml_parser_get_root(parser);
    YamlMapping *config = yaml_node_get_mapping(root);

    const gchar *name = yaml_mapping_get_string_member(config, "name");
    gint64 port = yaml_mapping_get_int_member_with_default(config, "port", 8080);
    gboolean debug = yaml_mapping_get_boolean_member(config, "debug");

    g_print("Name: %s\n", name);
    g_print("Port: %" G_GINT64_FORMAT "\n", port);
    g_print("Debug: %s\n", debug ? "yes" : "no");

    return 0;
}
```

### Compile

```bash
gcc -o example example.c \
    `pkg-config --cflags --libs yaml-glib-1.0`
```

Or if yaml-glib is not installed system-wide:

```bash
gcc -o example example.c \
    `pkg-config --cflags --libs glib-2.0 gobject-2.0 gio-2.0 yaml-0.1 json-glib-1.0` \
    -Ipath/to/yaml-glib/src -Lpath/to/yaml-glib/build/release -lyaml-glib-1.0
```

## Documentation

Full documentation is available in the `docs/` directory:

- [Getting Started](docs/getting-started.md) - Complete tutorial with working example
- [Building](docs/building.md) - Build instructions and configuration
- [API Reference](docs/api/) - Complete API documentation
- [Guides](docs/guides/) - How-to guides for common tasks

## License

AGPL-3.0-or-later

## Author

Zach Podbielniak
