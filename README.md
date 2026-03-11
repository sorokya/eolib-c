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

## Error Handling

All fallible functions return an `EoResult` value. A return value of `EO_SUCCESS` (0)
means the operation completed without error. Any other value indicates a failure and
describes what went wrong.

| Code | Value | Meaning |
|------|-------|---------|
| `EO_SUCCESS` | 0 | Operation completed successfully. |
| `EO_NULL_PTR` | 1 | A required pointer argument was `NULL` (e.g. writer, reader, sequencer, or output buffer). |
| `EO_OVERFLOW` | 2 | An internal size calculation would overflow — growing the writer buffer would exceed `SIZE_MAX`. |
| `EO_ALLOC_FAILED` | 3 | `malloc` or `realloc` returned `NULL`; the system could not satisfy the allocation. |
| `EO_NUMBER_TOO_LARGE` | 4 | A value passed to an EO encode function exceeds the maximum encodable integer (`EO_INT_MAX`). |
| `EO_BUFFER_UNDERRUN` | 5 | The reader does not have enough bytes remaining to satisfy the requested read. |
| `EO_CHUNKED_MODE_DISABLED` | 6 | `eo_reader_next_chunk` was called but chunked reading mode is not enabled on the reader. |
| `EO_INVALID_SEQUENCE_RANGE` | 7 | The derived `seq1` bounds produce an empty range; the start value is outside the representable sequence space. |
| `EO_SEQUENCE_OUT_OF_RANGE` | 8 | A derived sequence byte value falls outside `[0, EO_CHAR_MAX - 1]` and cannot be encoded. |
| `EO_INVALID_DATA` | 9 | Deserialization failed because a sentinel value, file magic string, or required constant field did not match the expected value. |

### Example

```c
EoWriter writer = eo_writer_init_with_capacity(16);
EoResult result = eo_writer_add_int(&writer, 12345);
if (result != EO_SUCCESS) {
    fprintf(stderr, "write failed: %d\n", result);
}
free(writer.data);
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
