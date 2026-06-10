#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
cd build
cmake ..
cmake --build . --target NewMatrix
if [[ -x "./NewMatrix" ]]; then
  ./NewMatrix
else
  echo "Ошибка: исполняемый файл NewMatrix не найден."
  exit 1
fi
