# FileRadar

**A cross-platform storage analyzer, duplicate finder and disk usage inspector written in C++.**

FileRadar helps you understand what is stored on a drive or inside a directory. Version 1.0.0 provides a lightweight command-line interface and JSON reports.

## Features

- Recursive directory scanning.
- Total file count and storage usage.
- Largest-file listing.
- File grouping by extension.
- Duplicate detection with a fast fingerprint followed by byte-for-byte verification.
- JSON report generation.
- Read-only operation: scanned files are never deleted or modified.
- Symbolic links are not followed.
- No third-party runtime dependencies.

## Requirements

- A C++17 compiler (GCC 8+, Clang 7+, or a recent MSVC).
- CMake 3.16 or newer.

## Build on Linux

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

The executable is created at build/fileradar.

## Install on Android with Termux (recommended)

For a one-command installation, open Termux and run:

```sh
pkg install -y curl
curl -fsSL https://raw.githubusercontent.com/Reaccionador2Jeje/FileRadar/main/scripts/install-termux.sh | bash
```

The installer installs the required build tools, downloads FileRadar, compiles it, runs its tests, and installs the `fileradar` command into Termux. Then use:

```sh
fileradar --help
```

To update, run the same installer command again. To uninstall the command, download and run `scripts/uninstall-termux.sh` from the repository. The uninstaller asks separately before deleting the downloaded source and build directory.

## Build on Android with Termux

Install the build tools:

    pkg update
    pkg install clang cmake make

Clone and build:

    git clone https://github.com/Reaccionador2Jeje/FileRadar.git
    cd FileRadar
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ./build/fileradar --help

On Android, grant Termux storage access using Android's normal permission flow if you need to scan shared storage. Only scan folders you are allowed to access.

## Commands

    fileradar scan <directory>
    fileradar large <directory> [limit]
    fileradar duplicates <directory>
    fileradar report <directory> [output.json]
    fileradar --help
    fileradar --version

Examples:

    ./build/fileradar scan .
    ./build/fileradar large ~/storage/shared/Download 15
    ./build/fileradar duplicates ~/storage/shared/Documents
    ./build/fileradar report . report.json

If no output file is supplied to report, JSON is written to standard output.

## Safety and privacy

FileRadar 1.0.0 is an offline, read-only analyzer. It does not upload file names or file contents, and it does not remove, rename, or modify scanned files. Duplicate detection reads candidate files to compare their contents. Permission errors are counted and skipped where possible.

Always review a report before making manual storage changes.

## Project structure

    include/fileradar/   Public headers
    src/                 Application and implementation
    tests/               Automated tests

## License

FileRadar is distributed under the Apache License 2.0. See LICENSE.
