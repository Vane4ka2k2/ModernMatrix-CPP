#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v doxygen >/dev/null 2>&1; then
  echo "Ошибка: doxygen не установлен. Установите doxygen и повторите попытку."
  exit 1
fi
if [[ ! -f Doxyfile ]]; then
  echo "Ошибка: Doxyfile не найден в корне проекта."
  exit 1
fi
mkdir -p docs/doxygen
doxygen Doxyfile

echo "Документация сформирована в docs/doxygen/html"
