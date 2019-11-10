#!/bin/bash
clear

OIFS="$IFS"
IFS=$'\n'

EXE="wfc_conv"

WIDTH=1024
HEIGHT=680


if [[ -d $1 ]]; then
  for file in `find "$1" -type f -name "*.wfc"`  
  do
   ./wfc2png "$file" $WIDTH $HEIGHT
  done
elif [[ -f $1 ]]; then
   ./wfc2png "$1" $WIDTH $HEIGHT
else
    echo "$1 not a file or folder!"
    exit 1
fi
