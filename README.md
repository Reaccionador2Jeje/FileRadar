# FileRadar

**A cross-platform storage analyzer, duplicate finder and disk usage inspector written in C++.**

FileRadar helps you understand what is stored on a drive or inside a directory. Version 1.3.0 adds a wxWidgets desktop GUI with folder browsing, scan summaries, largest-file and duplicate views, and JSON export. The TUI and command-line tools remain available.

## Features

- Recursive directory scanning.
- Total file count and storage usage.
- Largest-file listing.
- File grouping by extension.
- Duplicate detection with a fast fingerprint followed by byte-for-byte verification.
- JSON report generation.
- Interactive TUI menu for scanning, largest files, duplicate detection and JSON export.
- Live scan counters in the TUI: entries, files, folders, bytes and errors.
- Optional desktop GUI built with wxWidgets for Windows, Linux and macOS.
- Cross-platform C++17 build targets for Windows, Linux and macOS.
- Read-only structural diagnostics for basic PDF, PNG, JPEG and ZIP markers.
- Read-only operation: scanned files are never deleted or modified.
- Symbolic links are not followed.
- The CLI/TUI core has no third-party runtime dependencies; the optional desktop GUI requires wxWidgets.

## Requirements

- A C++17 compiler (GCC 8+, Clang 7+, or a recent MSVC).
- CMake 3.16 or newer.

## Build on desktop (Windows, Linux and macOS)

FileRadar uses standard C++17 and CMake. The CI workflow builds and tests on Ubuntu, Windows and macOS. Install a C++17 compiler and CMake 3.16+ for your platform, then run:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel
    ctest --test-dir build -C Release --output-on-failure

On Windows, run these commands from a Developer PowerShell or a terminal where your C++ compiler is configured.

## Build the desktop GUI

Install wxWidgets development files before configuring CMake. Examples:

- Ubuntu/Debian: `sudo apt install libwxgtk3.2-dev`
- macOS with Homebrew: `brew install wxwidgets`
- Windows: install wxWidgets development libraries and make them discoverable by CMake (for example, through vcpkg).

Then configure and build as usual:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFILERADAR_BUILD_GUI=ON
    cmake --build build --config Release --parallel

When wxWidgets is detected, the graphical application is built as `fileradar-gui` (or `fileradar-gui.exe`). Without wxWidgets, CMake still builds the CLI/TUI normally.

## Build native installers

The repository includes a GitHub Actions workflow that builds native desktop packages on all three desktop platforms:

- Windows: NSIS setup installer (`.exe`).
- Linux: Debian package (`.deb`).
- macOS: drag-and-drop disk image (`.dmg`).

To generate them, open **Actions → FileRadar installers → Run workflow** in GitHub. The workflow installs wxWidgets, builds the graphical app and CLI/TUI, runs the tests, creates the native package with CPack, and uploads each installer as a workflow artifact. Windows uses a static wxWidgets build; the macOS app bundle collects its non-system runtime libraries; the Debian package records shared-library dependencies.

The workflow also runs when a version tag beginning with `v` is pushed. These artifacts are build outputs, not a published GitHub Release. Publish a Release only after all three platform jobs have succeeded.

## Build on Linux

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

The terminal executable is created at build/fileradar. If wxWidgets is installed, the desktop app is also created at build/fileradar-gui.

## Install on Android with Termux (recommended)

This installer uses Python's standard library for downloading and Clang for compilation. It does **not** require `curl`, `wget`, `git`, or CMake.

Install the requirements:

```sh
pkg install -y python clang
```

Then run the public installer:

```sh
python -c "import urllib.request; exec(compile(urllib.request.urlopen('https://raw.githubusercontent.com/Reaccionador2Jeje/FileRadar/main/scripts/install-termux.py').read(), 'install-termux.py', 'exec'))"
```

The installer downloads the source archive from GitHub using Python, compiles and runs the core tests, then installs `fileradar` into Termux's `bin` directory. It does not modify your scanned files.

After installation:

```sh
fileradar --help
```

To update, run the same installer command again. To uninstall, remove the installed command with:

```sh
rm -f "$PREFIX/bin/fileradar"
```

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

    fileradar              # Open interactive TUI
    fileradar tui          # Open interactive TUI
    fileradar scan <directory>
    fileradar large <directory> [limit]
    fileradar duplicates <directory>
    fileradar report <directory> [output.json]
    fileradar repair diagnose <file>
    fileradar repair scan <directory>
    fileradar tui
    fileradar --help
    fileradar --version

Examples:

    ./build/fileradar
    ./build/fileradar scan .
    ./build/fileradar large ~/storage/shared/Download 15
    ./build/fileradar duplicates ~/storage/shared/Documents
    ./build/fileradar report . report.json

If no output file is supplied to report, JSON is written to standard output.\n\n`repair diagnose` checks one file; `repair scan` checks supported files in a directory. The first diagnostic pass recognizes basic PDF, PNG, JPEG and ZIP structure markers. It is read-only and does not attempt repair yet. A marker check is not a full format validator.

## Safety and privacy

FileRadar 1.3.0 is an offline, read-only analyzer. It does not upload file names or file contents, and it does not remove, rename, or modify scanned files. Duplicate detection reads candidate files to compare their contents. Permission errors are counted and skipped where possible.

Always review a report before making manual storage changes.

## Project structure

    include/fileradar/   Public headers
    src/                 Application and implementation
    tests/               Automated tests

## License

FileRadar is distributed under the Apache License 2.0. See LICENSE.


### TUI diagnostics

The TUI includes options to diagnose an individual file and scan the selected folder for suspicious file structures. These checks are read-only and do not repair or modify files.
