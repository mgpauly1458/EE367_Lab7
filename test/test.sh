#!/bin/bash

./test/compare_files.sh

if [ $? -eq 0 ]; then
  rmh
else
  exit 1
fi

