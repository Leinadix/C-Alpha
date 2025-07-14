# C-Alpha Compiler

![CI Status](https://github.com/Leinadix/C-Alpha/actions/workflows/ci.yml/badge.svg)
![Coverage](https://codecov.io/gh/Leinadix/C-Alpha/branch/main/graph/badge.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

A modern compiler implementation for the C-Alpha programming language, featuring a complete toolchain with lexical analysis, parsing, semantic analysis, and code generation capabilities.

## Features

- **Complete Compiler Pipeline**: Lexer, parser, semantic analyzer, and code generator
- **Language Server Protocol (LSP)**: Full IDE support with syntax highlighting and error detection
- **VS Code Extension**: Integrated development environment support
- **Comprehensive Testing**: Unit and integration tests for all components
- **Modern C++20**: Built with modern C++ standards and best practices

## Prerequisites

- **C++ Compiler**: GCC 11+ or Clang 18+ with C++20 support
- **CMake**: Version 3.20 or higher
- **LLVM**: Version 18+ (for advanced features)
- **Git**: For version control and submodules

### Platform Support

- Windows 10/11 (MSVC 2019+)
- macOS 10.15+ (Xcode 12+)
- Ubuntu 20.04+ / Debian 11+

## Build Instructions

### Out-of-Source Build (Recommended)

```bash
# Clone the repository
git clone https://github.com/Leinadix/C-Alpha.git
cd C-Alpha

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
make -j$(nproc)
```

### Debug Build

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Installation

```bash
# From build directory
make install

# Or specify custom prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make install
```

## Running Tests

```bash
# From build directory
ctest --verbose

# Or run individual test suites
./test_compiler
./test_semantic
./test_codegen
./test_syscall
```

## Usage

### Compiler

```bash
# Compile an C-Alpha source file
./alpha_c input.calpha -o output

# With debug symbols
./alpha_c input.calpha -o output -g

# Show help
./alpha_c --help
```

### Language Server

```bash
# Start the LSP server
./alpha_lsp --stdio
```

### VS Code Extension

1. Install the extension from `vscode-calpha/`
2. Open an `.calpha` file
3. Enjoy syntax highlighting and error detection

## Project Structure

```
C-Alpha/
├── src/                    # Core implementation
│   ├── lexer.cpp          # Lexical analyzer
│   ├── parser.cpp         # Syntax parser
│   ├── semantic.cpp       # Semantic analyzer
│   ├── codegen.cpp        # Code generator
│   └── preprocessor.cpp   # Preprocessor
├── include/               # Public headers
├── tests/                 # Test suites
├── lsp/                   # Language Server Protocol
├── vscode-calpha/         # VS Code extension
├── alpha/                 # Example programs
│   └── lib/std/          # Standard library
├── external/              # Third-party dependencies
└── docs/                  # Generated documentation
```

## Code Style and Contributing

This project follows the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html).

### Before Contributing

1. **Format your code**: `clang-format -i src/**/*.cpp include/**/*.hpp`
2. **Run static analysis**: `clang-tidy src/**/*.cpp`
3. **Run tests**: `ctest`
4. **Update documentation**: Add Doxygen comments for public APIs

### Contribution Guidelines

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes following the code style
4. Add tests for new functionality
5. Commit your changes: `git commit -m 'Add amazing feature'`
6. Push to the branch: `git push origin feature/amazing-feature`
7. Open a Pull Request

For detailed contribution guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

## Documentation

- **API Documentation**: Generated with Doxygen at `docs/html/index.html`
- **Language Reference**: See `docs/language-reference.md`
- **Developer Guide**: See `docs/developer-guide.md`

## Continuous Integration

The project uses GitHub Actions for:
- **Multi-platform builds**: Ubuntu, macOS, Windows
- **Multiple compilers**: GCC, Clang, MSVC
- **Code formatting**: clang-format validation
- **Static analysis**: clang-tidy checks
- **Test execution**: Complete test suite
- **Coverage reporting**: Code coverage analysis

## Versioning

This project follows [Semantic Versioning 2.0.0](https://semver.org/):
- **Major**: Breaking changes
- **Minor**: New features (backward compatible)
- **Patch**: Bug fixes (backward compatible)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- LLVM Project for compiler infrastructure inspiration
- nlohmann/json for JSON parsing in the LSP server
- The C++ community for modern language features

## Support

- **Issues**: Report bugs and feature requests on [GitHub Issues](https://github.com/Leinadix/C-Alpha/issues)
- **Discussions**: Join the conversation on [GitHub Discussions](https://github.com/Leinadix/C-Alpha/discussions)
- **Documentation**: Visit the [project wiki](https://github.com/Leinadix/C-Alpha/wiki)
