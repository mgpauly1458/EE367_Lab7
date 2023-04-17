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
    echo "Error: $file is not equal"
    exit_code=1
  fi
done

if [ $exit_code -eq 0 ]; then
  echo "All files are equal"
fi

exit $exit_code

