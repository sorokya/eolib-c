# EOLib

[![Build Status][actions-badge]][actions-url]
[![Docs][docs-badge]][docs-url]
[![License][mit-badge]][mit-url]

[docs-badge]: https://img.shields.io/github/actions/workflow/status/sorokya/eolib-c/docs.yml?label=docs
[docs-url]: https://sorokya.github.io/eolib-c/
[mit-badge]: https://img.shields.io/badge/license-MIT-blue.svg
[mit-url]: https://github.com/sorokya/eolib-c/blob/master/LICENSE
[actions-badge]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml/badge.svg
[actions-url]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml

A core C library for writing applications related to Endless Online

## Features

Read and write the following EO data structures:

- Client packets
- Server packets
- Endless Map Files (EMF)
- Endless Item Files (EIF)
- Endless NPC Files (ENF)
- Endless Spell Files (ESF)
- Endless Class Files (ECF)
- Endless Server Files (Inns, Drops, Shops, etc.)

Utilities:

- Data reader
- Data writer
- Number encoding
- String encoding
- Data encryption
- Packet sequencer

## Requirements

- CMake 3.16+
- C compiler
- libxml2
- json-c

### Install dependencies

macOS (Homebrew):

```bash
brew install libxml2 json-c
```

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y libxml2-dev libjson-c-dev ninja-build
```

Windows (MSVC):

- Use vcpkg with the provided manifest (`vcpkg.json`). CMake will pick it up if you configure with the toolchain file.

Windows (MinGW / MSYS2):

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-libxml2 mingw-w64-x86_64-json-c
```

## Build

```bash
cmake . -DCMAKE_BUILD_TYPE=Release
make
```

## Run tests

```bash
make test
```

## Install

```bash
make install
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
