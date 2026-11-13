#!/usr/bin/env python3
"""FileRadar installer for Termux. Uses Python urllib; no curl, wget, git or CMake."""
from __future__ import annotations

import io
import os
import pathlib
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request

REPOSITORY = "Reaccionador2Jeje/FileRadar"
ARCHIVE_URL = f"https://codeload.github.com/{REPOSITORY}/tar.gz/refs/heads/main"
BINARY_NAME = "fileradar"


def say(message: str) -> None:
    print(f"[FileRadar] {message}", flush=True)


def fail(message: str, code: int = 1) -> None:
    print(f"[FileRadar] ERROR: {message}", file=sys.stderr)
    raise SystemExit(code)


def run(command: list[str]) -> None:
    say("$ " + " ".join(command))
    try:
        subprocess.run(command, check=True)
    except FileNotFoundError:
        fail("No se encontró clang++. Instálalo con: pkg install clang")
    except subprocess.CalledProcessError as exc:
        fail(f"La compilación o prueba falló (código {exc.returncode}).")


def download_sources(destination: pathlib.Path) -> None:
    request = urllib.request.Request(
        ARCHIVE_URL,
        headers={"User-Agent": "FileRadar-Termux-Installer/1.0"},
    )
    say("Descargando el código fuente desde GitHub mediante Python...")
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            archive_data = response.read()
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        fail(f"No se pudo descargar FileRadar: {exc}")

    try:
        with tarfile.open(fileobj=io.BytesIO(archive_data), mode="r:gz") as archive:
            members = archive.getmembers()
            if not members:
                fail("GitHub devolvió un archivo vacío.")
            top = members[0].name.split("/", 1)[0]
            copied = 0
            for member in members:
                parts = pathlib.PurePosixPath(member.name).parts
                if len(parts) < 3 or parts[0] != top:
                    continue
                if parts[1] not in ("include", "src", "tests"):
                    continue
                if not member.isfile():
                    continue
                relative = pathlib.PurePosixPath(*parts[1:])
                if any(part in ("", ".", "..") for part in relative.parts):
                    continue
                if relative.suffix not in (".hpp", ".cpp"):
                    continue
                target = destination.joinpath(*relative.parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                source = archive.extractfile(member)
                if source is None:
                    continue
                with source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
                copied += 1
            if copied == 0 or not (destination / "src" / "main.cpp").is_file():
                fail("El archivo descargado no contiene el código esperado de FileRadar.")
    except (tarfile.TarError, OSError, ValueError) as exc:
        fail(f"No se pudo leer el archivo de GitHub: {exc}")


def main() -> None:
    prefix = os.environ.get("PREFIX")
    if not prefix or not pathlib.Path(prefix).is_dir() or "com.termux" not in prefix:
        fail("Este instalador está diseñado para Termux en Android.")

    compiler = shutil.which("clang++")
    if not compiler:
        fail("Falta Clang. Ejecuta: pkg install clang")

    install_dir = pathlib.Path(prefix) / "bin"
    if not install_dir.is_dir():
        fail(f"No existe el directorio de comandos: {install_dir}")

    with tempfile.TemporaryDirectory(prefix="fileradar-install-") as temporary:
        work = pathlib.Path(temporary)
        source = work / "source"
        source.mkdir()
        download_sources(source)

        core_sources = [
            str(source / "src" / name)
            for name in ("scanner.cpp", "analyzer.cpp", "duplicates.cpp", "reports.cpp", "tui.cpp")
        ]
        test_binary = work / "fileradar-tests"
        say("Compilando y ejecutando las pruebas...")
        run([
            compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
            "-I", str(source / "include"),
            *core_sources,
            str(source / "tests" / "test_core.cpp"),
            "-o", str(test_binary),
        ])
        run([str(test_binary)])

        say("Compilando FileRadar...")
        candidate = work / BINARY_NAME
        run([
            compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
            "-I", str(source / "include"),
            *core_sources,
            str(source / "src" / "main.cpp"),
            "-o", str(candidate),
        ])
        run([str(candidate), "--version"])

        destination = install_dir / BINARY_NAME
        staged = install_dir / (BINARY_NAME + ".new")
        shutil.copy2(candidate, staged)
        staged.chmod(0o755)
        os.replace(staged, destination)

    say("¡Instalación completada!")
    print("Ejecuta: fileradar --help")


if __name__ == "__main__":
    main()
