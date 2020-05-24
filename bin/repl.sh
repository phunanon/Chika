#!/bin/bash

while :
do
  echo "" > repl.chi
  while :
  do
    read -p "> " line
    if [ -z "$line" ]; then
      break
    fi
    echo $line >> repl.chi
  done
  ./chika repl.chi
  echo
done
