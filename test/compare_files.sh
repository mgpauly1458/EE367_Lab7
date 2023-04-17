#!/bin/bash

parentdir="$(pwd)"

files=("haha.txt" "large.txt")
exit_code=0

echo "Comparison results:"
echo $parentdir

for file in "${files[@]}"
do
  if cmp -s "$parentdir/t0/$file" "$parentdir/t1/$file"; then
    printf "  t0/%-10s equal to t1/%s\n" "$file" "$file"
  else
    echo -e "\e[31mError:\e[0m $file is not equal" # Red color for fail message
    exit_code=1
  fi
done

if [ $exit_code -eq 0 ]; then
  echo -e "\e[32mAll files are equal\e[0m" # Green color for success message
fi

exit $exit_code

