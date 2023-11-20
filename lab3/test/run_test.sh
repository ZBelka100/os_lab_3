#!/bin/bash

generate_random_number() {
  echo $((RANDOM % 201 - 100))
}

generate_file() {
  local filename="$1"
  local count="$2"

  > "$filename"
  for ((i = 0; i < count; i++)); do
    echo -n "$(generate_random_number) " >> "$filename"
  done
}

run_program() {
  local filename="$1"
  result=$(../src/a.out "$filename")  # Обновленный путь к исполняемому файлу
  echo "$result"
}

compare_results() {
  local expected="$1"
  local actual="$2"

  if [ "$expected" -eq "$actual" ]; then
    echo "Результаты совпадают: $expected"
  else
    echo "Результаты не совпадают. Ожидалось: $expected, Фактический результат: $actual"
  fi
}

# Основная логика
filename="gen_file.txt"
count=100  # Количество случайных чисел

generate_file "$filename" "$count"
# Используем tr для замены пробелов на символ новой строки
expected_sum=$(tr ' ' '\n' < "$filename" | awk '{s+=$1} END {print s}')
run_program "$filename"
actual_sum=$(run_program "$filename")
compare_results "$expected_sum" "$actual_sum"
