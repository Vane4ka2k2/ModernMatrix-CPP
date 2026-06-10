#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
cd build
cmake ..
cmake --build . --target matrix_tests
if [[ -x "./matrix_tests" ]]; then
  ./matrix_tests
else
  echo "Ошибка: исполняемый файл matrix_tests не найден."
  exit 1
fi
