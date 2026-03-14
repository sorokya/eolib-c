# Win32 Demo Application

This demo targets the Windows MinGW x86 build of EOLib.

Download a release archive from the [releases page](https://github.com/sorokya/eolib-c/releases/).

## Prerequisites

Tested on Windows Server 2003 with MinGW 5.1.6 and CMake 2.8.2.

Place these items next to this README:

- `include/`
- `lib/`
- `libeolib.dll` (from the release archive `bin/` directory)

## Generate Packet Data

The Packet Browser in the demo uses generated capture data in `packets.inc`.

From this directory, run:

```bash
python gen_packets.py
```

The script reads capture/protocol files from `../../eo-captured-packets` and `../../eo-protocol`, then writes `packets.inc` in this directory.

## Build

From this directory, run:

```bash
cmake .
mingw32-make
```

This produces `eolib_win32_demo.exe`.
