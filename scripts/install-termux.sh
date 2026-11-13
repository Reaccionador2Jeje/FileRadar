#!/data/data/com.termux/files/usr/bin/bash
set -Eeuo pipefail

REPO_URL="https://github.com/Reaccionador2Jeje/FileRadar.git"
INSTALL_DIR="${PREFIX:-/data/data/com.termux/files/usr}"
SOURCE_DIR="$INSTALL_DIR/opt/fileradar"
BUILD_DIR="$SOURCE_DIR/build"

say() { printf '\n[FileRadar] %s\n' "$*"; }
fail() { printf '\n[FileRadar] ERROR: %s\n' "$*" >&2; exit 1; }

if [ -z "${PREFIX:-}" ] || [ ! -d "$PREFIX" ] || ! command -v pkg >/dev/null 2>&1; then
  fail "Este instalador es para Termux en Android."
fi

say "Actualizando la lista de paquetes..."
pkg update -y

say "Instalando herramientas necesarias..."
pkg install -y git clang cmake make

mkdir -p "$INSTALL_DIR/opt"

if [ -d "$SOURCE_DIR/.git" ]; then
  say "Actualizando el código existente..."
  git -C "$SOURCE_DIR" pull --ff-only
elif [ -e "$SOURCE_DIR" ]; then
  fail "$SOURCE_DIR ya existe y no parece ser un repositorio Git. Muévelo o renómbralo antes de continuar."
else
  say "Descargando FileRadar..."
  git clone --depth 1 "$REPO_URL" "$SOURCE_DIR"
fi

say "Compilando FileRadar..."
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DFILERADAR_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --parallel 2

say "Ejecutando pruebas..."
ctest --test-dir "$BUILD_DIR" --output-on-failure

say "Instalando el comando..."
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

say "Instalación completada."
printf 'Prueba con: fileradar --help\n'
