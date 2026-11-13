#!/data/data/com.termux/files/usr/bin/bash
set -Eeuo pipefail

PREFIX_DIR="${PREFIX:-/data/data/com.termux/files/usr}"
BINARY="$PREFIX_DIR/bin/fileradar"
SOURCE_DIR="$PREFIX_DIR/opt/fileradar"

printf '[FileRadar] Se quitará el ejecutable instalado.\n'
if [ -f "$BINARY" ]; then
  rm -f "$BINARY"
fi

if [ -d "$SOURCE_DIR/.git" ]; then
  printf '[FileRadar] También se eliminarán el código fuente y los archivos de compilación en:\n%s\n' "$SOURCE_DIR"
  printf 'Escribe REMOVE para confirmar: '
  read -r answer
  if [ "$answer" = "REMOVE" ]; then
    rm -rf "$SOURCE_DIR"
  else
    printf 'Código fuente conservado.\n'
  fi
fi

printf '[FileRadar] Desinstalación terminada.\n'
